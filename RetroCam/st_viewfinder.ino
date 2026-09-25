/*
 * ============================================================================
 *  Retro Cam — st_viewfinder.ino : DURUM 1, Vizör
 * ============================================================================
 *  Canli goruntu + camcorder tarzi sabit HUD.
 *      KISA basis : fotograf cek (UXGA)
 *      UZUN basis : Galeri'ye gec
 *
 *  HUD her karede DEGIL, yalnizca durum degistiginde cizilir (hud_dirty).
 *  Goruntu ust/alt seritlere basilmadigi icin HUD silinmez.
 * ============================================================================
 */

static bool vf_hud_dirty = true;

// ---------------------------------------------------------------------------
static void vfDrawHUD() {
  uiClearBands();
  uiBrackets(TFT_WHITE);

  const int TY = 4;
  const int BY = SCR_H - BAND_H + 5;

  tft.setTextSize(2);

  // Ust sol — cihaz adi / kart uyarisi
  tft.setTextColor(sd_ok ? TFT_WHITE : TFT_RED, TFT_BLACK);
  tft.setCursor(16, TY);
  tft.print(sd_ok ? "RETRO CAM" : "KART YOK!");

  // Ust sag — REC gostergesi (dekoratif)
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(SCR_W - 84, TY);
  tft.print("REC");
  tft.fillCircle(SCR_W - 22, TY + 8, 6, TFT_RED);

  // Alt sol — kartta kalan yer (gercek olcum) + cekilen sayisi
  uiStorage(12, SCR_H - BAND_H + 7, sdFreePct(false));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(48, BY);
  tft.printf("%d", photo_cnt);

  // Alt sag — sonraki dosya no / SD uyarisi
  tft.setTextColor(sd_ok ? TFT_WHITE : TFT_RED, TFT_BLACK);
  tft.setCursor(SCR_W - 110, BY);
  if (sd_ok) tft.printf("IMG %04d", next_idx);
  else       tft.print("NO SD");
}

// ---------------------------------------------------------------------------
void vfEnter() {
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(jpegToBuffer);     // vizor -> tampona (titremesin)

  if (cam_ok) {
    // Aktarimdan gelirken trExit() gecis ekranini zaten acmisti
    // (ui_spin_active true birakir) — ikinci kez cizmeyelim.
    if (!ui_spin_active) {
      ui_spin_active = true;
      uiTransition("VIZOR", "kamera hazirlaniyor");
    }
    cameraToViewfinder();                // icindeki bekleme animasyonlu
  }
  ui_spin_active  = false;               // gecis bitti
  ui_hint_active  = false;               // savunma: bayrak asili kalmasin

  tft.fillScreen(TFT_BLACK);
  vf_hud_dirty = true;
}

void vfExit() {
  // Vizorden cikarken yapilacak ozel bir sey yok
}

// ---------------------------------------------------------------------------
void vfLoop() {
  if (!cam_ok) {
    uiMessage("KAMERA HATASI", "PSRAM? Besleme?", TFT_RED);
    delay(400);
    return;
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (fb) {
    TJpgDec.drawJpg(0, 0, fb->buf, fb->len);   // tampona coz
    esp_camera_fb_return(fb);

    // Uzun basis ipucu ekrandayken tamponu basmak onu hemen siliyordu.
    // Ipucu acikken goruntu donar; zaten mod degistirmek uzeresin.
    if (!ui_hint_active) uiPushBuffer();       // tek seferde ekrana
  }

  if (sdWatchChanged()) vf_hud_dirty = true;

  // Depolama gostergesi donuk kalmasin — 15 sn'de bir HUD tazelenir
  static uint32_t hud_t = 0;
  if (millis() - hud_t > 15000) { hud_t = millis(); vf_hud_dirty = true; }

  if (vf_hud_dirty) { vfDrawHUD(); vf_hud_dirty = false; }
}

// ---------------------------------------------------------------------------
//  KISA BASIS — fotograf cek
// ---------------------------------------------------------------------------
void vfShort() {
  if (!sd_ok) {
    uiMessage("SD KART YOK", "karti takin", TFT_RED);
    delay(1200);
    vf_hud_dirty = true;
    return;
  }

  // Kart dolmak uzereyse cekme. Eskiden f.write() eksik yaziyor ve geriye
  // "CEKIM BASARISIZ" ile birlikte bozuk bir dosya kaliyordu.
  int freep = sdFreePct(true);
  if (freep >= 0 && freep <= SD_STOP_PCT) {
    uiMessage("KART DOLU", "web arayuzunden silin", TFT_RED);
    delay(1800);
    vf_hud_dirty = true;
    return;
  }

  // Deklansor efekti
  tft.fillRect(0, IMG_Y, SCR_W, IMG_H, TFT_WHITE);
  delay(40);
  uiClearImage();
  uiMessage("CEKILIYOR", NULL, TFT_YELLOW);

  cameraToCapture();                 // UXGA + en iyi kalite

  bool ok = false;
  uint32_t len = 0;
  int w = 0, h = 0;

  camera_fb_t *fb = esp_camera_fb_get();
  if (fb) {
    // JPEG imzasi dogrula — bozuk kare kaydetmeyelim
    bool sig = fb->len > 4 && fb->buf[0] == 0xFF && fb->buf[1] == 0xD8;
    if (sig) {
      char path[32];
      imgPath(path, sizeof(path), next_idx);
      File f = SD_MMC.open(path, FILE_WRITE);
      if (f) {
        // Sensorun kendi JPEG'i — yeniden sikistirma YOK
        size_t written = f.write(fb->buf, fb->len);
        f.close();
        if (written == fb->len) {
          ok  = true;
          len = fb->len;
          w   = fb->width;
          h   = fb->height;
          photo_cnt++;
          // Yeni fotograf listenin SONUNA eklenir (numaralar artan sirada,
          // next_idx her zaman en buyukten buyuk) — yeniden taramaya gerek yok
          if (photo_list && photo_n < PHOTO_CAP) photo_list[photo_n++] = next_idx;
          next_idx++;
        }
      } else {
        sd_ok = false;               // kart cikmis olabilir
      }
    }
    esp_camera_fb_return(fb);
  }

  cameraToViewfinder();              // vizore don

  if (ok) {
    int fp = sdFreePct(true);
    char l1[24], l2[40];
    snprintf(l1, sizeof(l1), "IMG_%04d", next_idx - 1);
    snprintf(l2, sizeof(l2), "%dx%d  %u KB", w, h, (unsigned)(len / 1024));
    uiMessage(l1, l2, (fp >= 0 && fp <= SD_WARN_PCT) ? TFT_YELLOW : TFT_GREEN);
    if (fp >= 0 && fp <= SD_WARN_PCT) {
      tft.setTextSize(1);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setCursor(54, IMG_Y + IMG_H / 2 + 34);
      tft.printf("kartta %%%d yer kaldi", fp);
    }
  } else {
    uiMessage("CEKIM BASARISIZ", sd_ok ? "tekrar deneyin" : "SD kart yok",
              TFT_RED);
  }
  delay(1200);
  vf_hud_dirty = true;
}
