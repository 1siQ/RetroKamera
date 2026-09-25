/*
 * ============================================================================
 *  Retro Cam — st_gallery.ino : DURUM 2, Galeri
 * ============================================================================
 *  SD karttaki fotograflari cihaz ekraninda gosterir.
 *      KISA basis : bir ONCEKI (daha eski) fotograf; en eskiden sonra basa doner
 *      UZUN basis : Aktarim'a gec
 *
 *  SILME artik VAR (web arayuzunden). Bu yuzden dosya numaralari kesintisiz
 *  DEGIL. Galeri, scanPhotos() ile dizinden cikarilan gercek numara listesi
 *  uzerinde konum (pos) ile gezer; dosya numarasiyla degil.
 *
 *  Fotograflar UXGA (1600x1200). Ekrana sigdirmak icin cozucu 1/8 olcekte
 *  calisir: 1600/8 = 200, 1200/8 = 150 -> 200x150, goruntu alanina oturur.
 * ============================================================================
 */

static int  gal_pos      = 0;     // listedeki konum (0 = en eski)
static bool gal_need_draw = true;

// ---------------------------------------------------------------------------
static void galDrawBands() {
  uiClearBands();
  uiBrackets(TFT_DARKGREY);

  const int TY = 4;
  const int BY = SCR_H - BAND_H + 5;
  int total = photo_n;

  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(16, TY);
  tft.print("GALERI");

  // Kacinci / toplam — konum tabanli, dosya numarasi degil
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(SCR_W - 110, TY);
  if (total > 0) tft.printf("%3d/%-3d", gal_pos + 1, total);
  else           tft.print("  0/0  ");

  // Alt: dosya adi + ipucu
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(8, BY + 4);
  if (total > 0) tft.printf("IMG_%04d.jpg", photoAt(gal_pos));
  else           tft.print("fotograf yok");

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(SCR_W - 130, BY + 4);
  tft.print("KISA=ONCEKI  UZUN=CIK");
}

// Bir fotografi ekrana ciz. Dosyayi PSRAM'e okuyup cozeriz; TJpg_Decoder'in
// dosya API'si kutuphane surumune gore degistigi icin bu yol daha guvenli.
static bool galShowPhoto(int idx) {
  uiClearImage();

  if (idx < 1) {
    uiMessage("FOTOGRAF YOK", "once cekim yapin", TFT_DARKGREY);
    return false;
  }

  char path[32];
  imgPath(path, sizeof(path), idx);

  File f = SD_MMC.open(path, FILE_READ);
  if (!f) {
    uiMessage("ACILAMADI", path, TFT_RED);
    return false;
  }
  size_t len = f.size();
  if (len == 0 || len > 1024UL * 1024UL) {   // makul olmayan boyut
    f.close();
    uiMessage("BOZUK DOSYA", path, TFT_RED);
    return false;
  }

  uint8_t *buf = (uint8_t *)heap_caps_malloc(len, MALLOC_CAP_SPIRAM);
  if (!buf) {
    f.close();
    uiMessage("BELLEK YETERSIZ", NULL, TFT_RED);
    return false;
  }
  size_t rd = f.read(buf, len);
  f.close();

  bool ok = false;
  if (rd == len) {
    // 1/8 olcek: 1600x1200 -> 200x150
    TJpgDec.setJpgScale(8);
    TJpgDec.setCallback(jpegToScreen);
    tft.setSwapBytes(false);   // cozucu zaten cevirdi; ortuk duruma guvenme

    uint16_t jw = 0, jh = 0;
    TJpgDec.getJpgSize(&jw, &jh, buf, len);
    int ow = jw / 8, oh = jh / 8;
    int ox = (SCR_W - ow) / 2;
    int oy = IMG_Y + (IMG_H - oh) / 2;
    if (ox < 0) ox = 0;
    if (oy < IMG_Y) oy = IMG_Y;

    // Seritlere tasmasin diye kirp
    tft.setViewport(0, IMG_Y, SCR_W, IMG_H, false);
    ok = (TJpgDec.drawJpg(ox, oy, buf, len) == JDR_OK);
    tft.resetViewport();
  }
  free(buf);

  if (!ok) uiMessage("COZULEMEDI", path, TFT_RED);
  return ok;
}

// ---------------------------------------------------------------------------
void galEnter() {
  // Kart taramasi fotograf sayisi arttikca uzayabilir — bu yuzden
  // kisa da olsa gecis ekrani gosteriliyor.
  ui_spin_active = true;
  uiTransition("GALERI", "kart taraniyor");
  uiSpinTick();

  if (sd_ok) scanPhotos();
  gal_pos = photo_n - 1;           // EN YENI fotograftan basla

  int total = photo_n;
  char note[32];
  snprintf(note, sizeof(note), "%d fotograf bulundu", total);
  uiTransitionNote(total > 0 ? note : "fotograf yok", TFT_DARKGREY);
  uiSpinDelay(350);

  ui_spin_active = false;
  tft.fillScreen(TFT_BLACK);
  gal_need_draw = true;
}

void galExit() {
  // Galeriden cikarken cozucuyu vizor ayarina birakmak vfEnter'in isi
}

// ---------------------------------------------------------------------------
void galLoop() {
  if (sdWatchChanged()) {
    if (sd_ok) scanPhotos();
    gal_pos = photo_n - 1;
    gal_need_draw = true;
  }
  if (gal_need_draw) {
    galShowPhoto(photoAt(gal_pos));
    galDrawBands();
    gal_need_draw = false;
  }
  delay(20);       // galeride surekli cizim yok, islemciyi bosuna yorma
}

// ---------------------------------------------------------------------------
//  KISA BASIS — bir onceki (daha eski) fotograf
// ---------------------------------------------------------------------------
void galShort() {
  if (photo_n <= 0) return;

  gal_pos--;
  if (gal_pos < 0) gal_pos = photo_n - 1;   // en eskiden sonra en yeniye don
  gal_need_draw = true;
}
