/*
 * ============================================================================
 *  Retro Cam — ui.ino : ortak cizim yardimcilari
 * ============================================================================
 *  Ekran uc bolgeye ayrilir:
 *      ust serit  (0 .. BAND_H)          — sabit bilgi
 *      goruntu    (IMG_Y .. IMG_Y+IMG_H) — canli goruntu / fotograf / QR
 *      alt serit  (SCR_H-BAND_H .. SCR_H) — sabit bilgi
 *
 *  Seritlere goruntu BASILMAZ. Boylece oradaki yazilar her karede silinmez
 *  ve titremez — bkz. docs/05-dev-log.md.
 * ============================================================================
 */

// ---------------------------------------------------------------------------
//  ACILIS EKRANI — logo + ilerleme cubugu
// ---------------------------------------------------------------------------
//  Onceki surumde duz metin listesi vardi. Artik marka + ilerleme cubugu +
//  adim etiketi gosteriliyor; her alt sistem hazir olunca cubuk ilerliyor.
// ---------------------------------------------------------------------------
static const int BAR_X = 40;
static const int BAR_Y = 150;
static const int BAR_W = SCR_W - 80;
static const int BAR_H = 10;

// Marka yazisi + bos ilerleme cubugu
void uiBootBegin() {
  tft.fillScreen(TFT_BLACK);

  // --- marka ---
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(74, 66);            // ISIK 4 harf: RETRO'dan (5 harf) dar
  tft.print("ISIK");                // GLCD fontunda S-cedilli yok, ASCII sart
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(176, 66);
  tft.print("CAM");

  // kirmizi REC noktasi — vizordeki HUD ile ayni dil
  tft.fillCircle(38, 78, 7, TFT_RED);

  // surum — OTA sonrasi hangi surumun yuklu oldugunu gormenin tek yolu
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(56, 96);
  tft.print("v" FW_VERSION);

  // ince ayrac
  tft.drawFastHLine(56, 112, 208, TFT_DARKGREY);

  // --- bos cubuk ---
  tft.drawRect(BAR_X, BAR_Y, BAR_W, BAR_H, TFT_DARKGREY);
}

// Bir adim tamamlandi: cubugu ilerlet, etiketi yaz.
//   pct : 0-100
//   ok  : true = yesil tik, false = kirmizi carpi
void uiBootStep(const char *label, int pct, bool ok) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;

  // cubugu doldur
  int w = (BAR_W - 2) * pct / 100;
  tft.fillRect(BAR_X + 1, BAR_Y + 1, w, BAR_H - 2,
               ok ? TFT_CYAN : TFT_RED);

  // etiket satiri (cubugun altinda, tek satir — her seferinde temizlenir)
  tft.fillRect(0, BAR_Y + 22, SCR_W, 20, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ok ? TFT_WHITE : TFT_RED, TFT_BLACK);
  tft.setCursor(BAR_X, BAR_Y + 26);
  tft.print(label);

  // sagda durum isareti
  tft.setCursor(BAR_X + BAR_W - 18, BAR_Y + 26);
  tft.print(ok ? "OK" : "!!");
}

// Acilis bitti — kisa bir "hazir" mesaji
void uiBootDone(const char *msg) {
  tft.fillRect(0, BAR_Y + 22, SCR_W, 24, TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  int x = (SCR_W - (int)strlen(msg) * 12) / 2;
  tft.setCursor(x < 0 ? 0 : x, BAR_Y + 24);
  tft.print(msg);
}

// ---------------------------------------------------------------------------
//  GUNCELLEME EKRANI  (hem kablosuz OTA hem SD kurtarma icin ortak)
// ---------------------------------------------------------------------------
//  Yukleme sirasinda kullanici ekrana bakiyor olacak. Siyah ekran birakmak
//  "kilitlendi mi?" hissi verir ve gucu kesmeye iter — ki bu tam da cihazi
//  olduren seydir. Bu yuzden ilerleme gorunur olmali.
// ---------------------------------------------------------------------------
static const int OTA_BX = 30;
static const int OTA_BY = 150;
static const int OTA_BW = SCR_W - 60;
static const int OTA_BH = 14;
static int ota_last_pct = -1;

void uiOtaBegin(const char *title) {
  ota_last_pct = -1;
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  int tx = (SCR_W - (int)strlen(title) * 12) / 2;
  tft.setCursor(tx < 0 ? 0 : tx, 66);
  tft.print(title);

  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  const char *w = "GUCU KESME - BUTONA DOKUNMA";
  int wx = (SCR_W - (int)strlen(w) * 6) / 2;
  tft.setCursor(wx < 0 ? 0 : wx, 100);
  tft.print(w);

  tft.drawRect(OTA_BX, OTA_BY, OTA_BW, OTA_BH, TFT_DARKGREY);
}

// done / total bayt. total bilinmiyorsa (0) yalnizca KB gosterilir.
void uiOtaProgress(uint32_t done, uint32_t total) {
  int pct = (total > 1000) ? (int)((uint64_t)done * 100 / total) : -1;
  if (pct > 100) pct = 100;

  // Her cagride cizim yapma — SPI'yi bosuna mesgul eder, yuklemeyi yavaslatir
  if (pct >= 0 && pct == ota_last_pct) { delay(0); return; }
  ota_last_pct = pct;

  if (pct >= 0) {
    int w = (OTA_BW - 2) * pct / 100;
    tft.fillRect(OTA_BX + 1, OTA_BY + 1, w, OTA_BH - 2, TFT_CYAN);
  }

  tft.fillRect(0, OTA_BY + 22, SCR_W, 16, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(OTA_BX, OTA_BY + 24);
  if (pct >= 0) tft.printf("%%%d   %u KB", pct, (unsigned)(done / 1024));
  else          tft.printf("%u KB", (unsigned)(done / 1024));

  delay(0);          // bosta kalan gorevlere nefes aldir (watchdog)
}

void uiOtaDone(bool ok, const char *msg) {
  uint16_t col = ok ? TFT_GREEN : TFT_RED;

  if (ok) tft.fillRect(OTA_BX + 1, OTA_BY + 1, OTA_BW - 2, OTA_BH - 2, TFT_GREEN);

  tft.fillRect(0, OTA_BY + 22, SCR_W, 40, TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(col, TFT_BLACK);
  const char *head = ok ? "TAMAM" : "BASARISIZ";
  int hx = (SCR_W - (int)strlen(head) * 12) / 2;
  tft.setCursor(hx < 0 ? 0 : hx, OTA_BY + 26);
  tft.print(head);

  if (msg && *msg) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    int mx = (SCR_W - (int)strlen(msg) * 6) / 2;
    tft.setCursor(mx < 0 ? 0 : mx, OTA_BY + 48);
    tft.print(msg);
  }
}

// ---------------------------------------------------------------------------
//  GECIS EKRANI — durumlar arasi, siyah ekran yerine
// ---------------------------------------------------------------------------
//  Kamera/Wi-Fi baslatma islemleri isemciyi bloke eder; o anlarda animasyon
//  donemez. Ancak bu islemlerin cevresindeki bekleme sureleri (sensor
//  oturmasi, kare atma) uiSpinDelay() ile animasyonlu hale getirilir.
// ---------------------------------------------------------------------------
static uint8_t spin_step = 0;
static int     spin_cx = SCR_W / 2;
static int     spin_cy = IMG_Y + IMG_H / 2 + 16;

// Gecis ekranini kur: baslik + alt aciklama + bos spinner alani
void uiTransition(const char *title, const char *sub) {
  tft.fillScreen(TFT_BLACK);

  // baslik
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  int tx = (SCR_W - (int)strlen(title) * 12) / 2;
  tft.setCursor(tx < 0 ? 0 : tx, 60);
  tft.print(title);

  // alt aciklama
  if (sub) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    int sx = (SCR_W - (int)strlen(sub) * 6) / 2;
    tft.setCursor(sx < 0 ? 0 : sx, 88);
    tft.print(sub);
  }
  spin_step = 0;
}

// Spinner'i bir adim ilerlet — 8 nokta, biri parlak, donuyor
void uiSpinTick() {
  const int R = 22;
  for (int i = 0; i < 8; i++) {
    float a = i * 0.7853981f;            // 2*PI/8
    int x = spin_cx + (int)(cosf(a) * R);
    int y = spin_cy + (int)(sinf(a) * R);
    // aktif noktadan geriye dogru sonen kuyruk
    int d = (spin_step - i + 8) % 8;
    uint16_t c;
    if      (d == 0) c = TFT_WHITE;
    else if (d == 1) c = TFT_CYAN;
    else if (d == 2) c = TFT_DARKCYAN;
    else             c = 0x2104;         // cok koyu gri
    tft.fillCircle(x, y, 4, c);
  }
  spin_step = (spin_step + 1) % 8;
}

// delay() yerine: beklerken spinner doner
void uiSpinDelay(uint32_t ms) {
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    uiSpinTick();
    delay(55);
  }
}

// Gecis ekranina durum satiri yaz (spinner'in altina)
void uiTransitionNote(const char *msg, uint16_t color) {
  tft.fillRect(0, spin_cy + 34, SCR_W, 18, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(color, TFT_BLACK);
  int x = (SCR_W - (int)strlen(msg) * 6) / 2;
  tft.setCursor(x < 0 ? 0 : x, spin_cy + 38);
  tft.print(msg);
}

// ---------------------------------------------------------------------------
//  Seritler
// ---------------------------------------------------------------------------
void uiClearBands() {
  tft.fillRect(0, 0, SCR_W, BAND_H, TFT_BLACK);
  tft.fillRect(0, SCR_H - BAND_H, SCR_W, BAND_H, TFT_BLACK);
}

void uiClearImage() {
  tft.fillRect(0, IMG_Y, SCR_W, IMG_H, TFT_BLACK);
}

// Kose ayraclari — seritlerin ICINDE kalir, goruntu tarafindan silinmez
void uiBrackets(uint16_t col) {
  tft.drawFastHLine(4, 2, 26, col);
  tft.drawFastVLine(4, 2, 14, col);
  tft.drawFastHLine(SCR_W - 30, 2, 26, col);
  tft.drawFastVLine(SCR_W - 5, 2, 14, col);
  tft.drawFastHLine(4, SCR_H - 3, 26, col);
  tft.drawFastVLine(4, SCR_H - 17, 14, col);
  tft.drawFastHLine(SCR_W - 30, SCR_H - 3, 26, col);
  tft.drawFastVLine(SCR_W - 5, SCR_H - 17, 14, col);
}

// Depolama gostergesi — pil ikonunun yerini aldi.
//
// Eski pil ikonu her zaman dolu uc cubuk ciziyordu: hicbir sey olcmuyor,
// sadece pil gibi gorunuyordu. Bos ADC pini olmadigi icin (8 GPIO'nun hepsi
// dolu) gercek voltaj olcumu GPIO 33'e lehim ister. Yalan soyleyen bir
// gosterge yerine ayni yere GERCEKTEN olculebilen bir sey konuldu:
// kartta kalan yer.
//
// pct < 0 -> kart yok: kutu bos ve kirmizi
void uiStorage(int x, int y, int pct) {
  uint16_t col = TFT_WHITE;
  if      (pct < 0)             col = TFT_RED;
  else if (pct <= SD_STOP_PCT)  col = TFT_RED;
  else if (pct <= SD_WARN_PCT)  col = TFT_YELLOW;

  tft.drawRect(x, y, 26, 11, col);
  if (pct > 0) {
    int w = (24 * pct) / 100;
    if (w < 1) w = 1;
    tft.fillRect(x + 1, y + 1, w, 9, col);
  }
}

// ---------------------------------------------------------------------------
//  Mesaj kutulari (goruntu alaninin ortasinda)
// ---------------------------------------------------------------------------
void uiMessage(const char *l1, const char *l2, uint16_t color) {
  const int bw = SCR_W - 80, bh = 62;
  const int bx = 40, by = IMG_Y + (IMG_H - bh) / 2;
  tft.fillRect(bx, by, bw, bh, TFT_BLACK);
  tft.drawRect(bx, by, bw, bh, color);
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextSize(2);
  if (l1) { tft.setCursor(bx + 14, by + 12); tft.print(l1); }
  if (l2) {
    tft.setTextSize(1);
    tft.setCursor(bx + 14, by + 40);
    tft.print(l2);
  }
}

// Uzun basis ipucu — kullanici birakmadan once ne olacagini gorsun
void uiHint(const char *text) {
  ui_hint_active = true;          // vizor bu bayrak acikken tamponu basmaz
  const int bw = 190, bh = 28;
  const int bx = (SCR_W - bw) / 2, by = IMG_Y + IMG_H - bh - 6;
  tft.fillRect(bx, by, bw, bh, TFT_BLACK);
  tft.drawRect(bx, by, bw, bh, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(bx + 12, by + 6);
  tft.print(text);
}

void uiHintClear() {
  ui_hint_active = false;
  const int bw = 190, bh = 28;
  const int bx = (SCR_W - bw) / 2, by = IMG_Y + IMG_H - bh - 6;
  tft.fillRect(bx, by, bw, bh, TFT_BLACK);
}

// ---------------------------------------------------------------------------
//  JPEG cozucu geri cagirmalari
// ---------------------------------------------------------------------------

// (A) TAMPONA yaz — vizor icin. Kare tamamlaninca tek seferde basilir;
//     boylece ekran "supurulerek" boyanmaz ve harekette dalgalanma olmaz.
bool jpegToBuffer(int16_t x, int16_t y, uint16_t w, uint16_t h,
                  uint16_t *bitmap) {
  if (!fbuf) return 0;
  if (y >= IMG_Y + IMG_H) return 0;          // alt siniri gectik, cozmeyi kes

  for (uint16_t r = 0; r < h; r++) {
    int sy = y + r;
    if (sy < IMG_Y || sy >= IMG_Y + IMG_H) continue;   // serit disi
    int by = sy - IMG_Y;

    int sx = x, cw = w;
    uint16_t *src = bitmap + (size_t)r * w;
    if (sx < 0) { cw += sx; src -= sx; sx = 0; }
    if (sx + cw > SCR_W) cw = SCR_W - sx;
    if (cw <= 0) continue;

    memcpy(fbuf + (size_t)by * SCR_W + sx, src, (size_t)cw * 2);
  }
  return 1;
}

// (B) DOGRUDAN ekrana yaz — galeri icin. Tek kare gosterildigi ve hareket
//     olmadigi icin tamponlamaya gerek yok.
bool jpegToScreen(int16_t x, int16_t y, uint16_t w, uint16_t h,
                  uint16_t *bitmap) {
  if (y >= SCR_H) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// Tamponu ekrana bas (vizor)
void uiPushBuffer() {
  if (!fbuf) return;
  tft.setSwapBytes(false);   // veri cozucu tarafindan zaten cevrildi
  tft.pushImage(0, IMG_Y, SCR_W, IMG_H, fbuf);
}
