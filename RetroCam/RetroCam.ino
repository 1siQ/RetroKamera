/*
 * ============================================================================
 *  RETRO CAM — Nihai Firmware
 * ============================================================================
 *
 *  ESP32-CAM tabanli kompakt dijital fotograf makinesi.
 *  Tek buton, uc durum, yatay ekran.
 *
 *      VIZOR  --uzun-->  GALERI  --uzun-->  AKTARIM  --uzun-->  VIZOR
 *        |                  |                   |
 *   kisa: cek        kisa: onceki foto     kisa: (yok)
 *
 *  Ayarlar cihazda DEGIL, aktarim modundaki web sayfasindadir — tek butonla
 *  menu yonetmek zor oldugu icin. Boylece hicbir yerde cift basis gerekmez
 *  ve deklansor anlik kalir.
 *
 *  ============================  KURULUM  =================================
 *  Kutuphaneler (Library Manager):
 *      * TFT_eSPI      (Bodmer)  -> User_Setup.h'i bu klasordekiyle degistir
 *      * TJpg_Decoder  (Bodmer)
 *    QR icin kutuphane GEREKMEZ (ESP32 cekirdegindeki qrcode bileseni).
 *    DIKKAT: ricmoo/QRCode kuruluysa KALDIR — ayni dosya adiyla cakisir.
 *
 *  Arduino IDE:
 *      Kart  : AI Thinker ESP32-CAM
 *      PSRAM : Enabled            (ZORUNLU)
 *
 *  KABLO:
 *      Ekran SCL->13  SDA->4  DC->12(+4.7k pulldown)  CS->1  RST->3.3V
 *      Buton -> bir ucu GPIO 3, digeri GND
 *      SD kart yuvada (FAT32, maks 32 GB)
 *
 *  !!! YUKLEME SONRASI FTDI'IN TX KABLOSUNU CIKAR !!!
 *      Buton GPIO 3'te (UART RX). FTDI TX bagli kalirsa hatti yuksek surer
 *      ve buton okunamaz. 5V/GND besleme icin bagli kalabilir.
 *
 *  !!! SERI PORT YOK — GPIO 1 ekran CS'i. Serial.begin() CAGIRMA.
 *      Tum geri bildirim EKRANDAN gelir.
 * ============================================================================
 */

#include "config.h"

#include <math.h>       // uiSpinTick: cosf / sinf
#include <string.h>     // uiBootDone / uiTransition: strlen
#include <stdlib.h>     // scanPhotos: qsort / atoi
#include <TFT_eSPI.h>
#include <SPI.h>
#include <TJpg_Decoder.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Update.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_camera.h"
#include "esp_heap_caps.h"
#include "qrcode.h"
#include "FS.h"
#include "SD_MMC.h"

// ===========================================================================
//  PAYLASILAN DEGISKENLER
//  Bu dosya ilk derlendigi icin buradaki tanimlar diger .ino dosyalarindan
//  gorulebilir. Duruma ozel degiskenler kendi dosyalarinda static'tir.
// ===========================================================================
TFT_eSPI   tft = TFT_eSPI();
WebServer  server(80);
DNSServer  dns;
Preferences prefs;

AppState state = ST_VIEWFINDER;

bool  sd_ok    = false;      // SD kart takili ve calisiyor mu
bool  cam_ok   = false;      // kamera baslatildi mi
bool  wifi_on  = false;      // AP acik mi
int   next_idx = 1;          // sirada kullanilacak fotograf numarasi
int   photo_cnt = 0;         // bu oturumda cekilen

// Karttaki fotograf numaralari, kucukten buyuge. Silme eklendigi icin
// numaralar kesintisiz degil; galeri bu liste uzerinde gezer.
// PSRAM'de tutulur (4000 x 4 bayt = 16 KB).
int  *photo_list = nullptr;
int   photo_n    = 0;

Settings cfg;                // kalici ayarlar

// Vizor kare tamponu (PSRAM). Cift tamponlama, dalgalanmayi onler —
// bkz. docs/05-dev-log.md "Cift tamponlama zorunlu".
uint16_t *fbuf = nullptr;

// Gecis ekrani acikken true. hw.ino'daki bekleme sureleri bu bayrak aciksa
// duz delay() yerine animasyonlu uiSpinDelay() kullanir — boylece sensorun
// oturmasini beklerken ekran donmus gorunmez.
bool ui_spin_active = false;

// Kablosuz guncelleme surerken true. Bu bayrak acikken buton TAMAMEN
// devre disidir: yukleme ortasinda uzun basis trExit()'i cagirir, Wi-Fi
// kapanir, flash yarim kalir ve cihaz acilmaz hale gelir.
bool ota_active = false;

// Uzun basis ipucu ekranda duruyor. Vizorde bu bayrak acikken kare tamponu
// EKRANA BASILMAZ; aksi halde ipucu kutusu bir sonraki karede siliniyordu
// ve kullanici basili tutarken hicbir sey goremiyordu.
bool ui_hint_active = false;

// Saat kuruldu mu (aktarim sayfasini acan tarayicidan alinir)
bool time_set = false;

// ===========================================================================
//  AYARLAR — kalici saklama
// ===========================================================================
void settingsLoad() {
  prefs.begin("retrocam", true);              // salt okunur

  // Ayar surumu: varsayilanlar degistiginde NVS'teki eski degerler
  // kaliyordu ve degisiklik hicbir ise yaramiyordu. Surum uyusmuyorsa
  // ayarlar bir kereye mahsus yeni varsayilanlara doner.
  uint8_t rev = prefs.getUChar("rev", 0);
  if (rev != SETTINGS_REV) {
    prefs.end();
    settingsReset();
    return;
  }

  cfg.ae_level    = prefs.getChar ("ae",   DEF_AE_LEVEL);
  cfg.saturation  = prefs.getChar ("sat",  DEF_SATURATION);
  cfg.brightness  = prefs.getChar ("bri",  DEF_BRIGHTNESS);
  cfg.contrast    = prefs.getChar ("con",  DEF_CONTRAST);
  cfg.cap_quality = prefs.getUChar("q",    DEF_CAP_QUALITY);
  cfg.vflip       = prefs.getUChar("vf",   DEF_VFLIP);
  cfg.hmirror     = prefs.getUChar("hm",   DEF_HMIRROR);
  prefs.end();
}

void settingsSave() {
  prefs.begin("retrocam", false);
  prefs.putChar ("ae",  cfg.ae_level);
  prefs.putChar ("sat", cfg.saturation);
  prefs.putChar ("bri", cfg.brightness);
  prefs.putChar ("con", cfg.contrast);
  prefs.putUChar("q",   cfg.cap_quality);
  prefs.putUChar("vf",  cfg.vflip);
  prefs.putUChar("hm",  cfg.hmirror);
  prefs.putUChar("rev", SETTINGS_REV);
  prefs.end();
}

// ===========================================================================
//  SAAT
// ===========================================================================
//  RTC yok, AP modunda internet yok. Saati aktarim sayfasini acan tarayici
//  veriyor (bkz. handleTime). Son bilinen zaman NVS'e yazilir; acilista
//  taban olarak geri yuklenir. Boylece dosyalar 1970 yerine "en azindan
//  son bagladigin an" damgasini alir — kabaca dogru, kesin degil.
// ===========================================================================
void timeLoad() {
  prefs.begin("retrocam", true);
  uint32_t t0 = prefs.getULong("t", 0);
  prefs.end();

  if (t0 > 1700000000UL) {
    struct timeval tv;
    tv.tv_sec  = (time_t)t0;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
  }
}

void timeStore() {
  time_t now = time(NULL);
  if (now < 1700000000L) return;
  prefs.begin("retrocam", false);
  prefs.putULong("t", (uint32_t)now);
  prefs.end();
}

// "2026-09-16 14:32" — saat kurulmadiysa "----------------"
void timeStamp(char *out, size_t n) {
  time_t now = time(NULL);
  if (now < 1700000000L) { snprintf(out, n, "saat yok"); return; }
  struct tm t;
  localtime_r(&now, &t);
  strftime(out, n, "%Y-%m-%d %H:%M", &t);
}

void settingsReset() {
  cfg.ae_level    = DEF_AE_LEVEL;
  cfg.saturation  = DEF_SATURATION;
  cfg.brightness  = DEF_BRIGHTNESS;
  cfg.contrast    = DEF_CONTRAST;
  cfg.cap_quality = DEF_CAP_QUALITY;
  cfg.vflip       = DEF_VFLIP;
  cfg.hmirror     = DEF_HMIRROR;
  settingsSave();
}

// ===========================================================================
//  DURUM GECISLERI
// ===========================================================================
static void stateEnter(AppState s) {
  switch (s) {
    case ST_VIEWFINDER: vfEnter();  break;
    case ST_GALLERY:    galEnter(); break;
    case ST_TRANSFER:   trEnter();  break;
    default: break;
  }
  state = s;
}

static void stateExit(AppState s) {
  switch (s) {
    case ST_VIEWFINDER: vfExit();  break;
    case ST_GALLERY:    galExit(); break;
    case ST_TRANSFER:   trExit();  break;
    default: break;
  }
}

// Uzun basis: bir sonraki duruma dongusel gecis
static void nextState() {
  // Ipucu bayragi burada MUTLAKA dusmeli. Uzun basiste buttonTask'in
  // birakma dalindaki uiHintClear() calismiyor (long_fired true oldugu
  // icin), bayrak acik kaliyordu ve vizor kareyi ekrana hic basmiyordu:
  // kamera calisiyor gorunmuyordu.
  ui_hint_active = false;

  AppState from = state;
  AppState to   = (AppState)((state + 1) % ST_COUNT);
  stateExit(from);
  stateEnter(to);
}

// ===========================================================================
//  BUTON — kisa / uzun basis
//  Cift basis YOKTUR: cift basisi algilamak icin kisa basisi geciktirmek
//  gerekirdi ve bu deklansoru hantal hissettirirdi.
// ===========================================================================
static bool     btn_down   = false;
static uint32_t btn_t      = 0;
static bool     long_fired = false;
static bool     hint_shown = false;

static void buttonTask() {
  // Guncelleme sirasinda buton yok sayilir — bkz. ota_active tanimi.
  if (ota_active) return;

  uint32_t now = millis();
  bool raw = (digitalRead(BTN_PIN) == LOW);   // pull-up: basili = LOW

  if (raw && !btn_down) {                     // --- basildi ---
    btn_down = true;
    btn_t = now;
    long_fired = false;
    hint_shown = false;
  }
  else if (raw && btn_down) {                 // --- basili tutuluyor ---
    uint32_t held = now - btn_t;

    // Kullanici birakmadan once ne olacagini gorsun — yanlislikla
    // durum degistirmeyi onler.
    if (!hint_shown && !long_fired && held > HINT_MS) {
      hint_shown = true;
      uiHint(stateNextName());
    }
    if (!long_fired && held >= LONG_MS) {
      long_fired = true;
      nextState();
    }
  }
  else if (!raw && btn_down) {                // --- birakildi ---
    uint32_t held = now - btn_t;
    btn_down = false;
    if (!long_fired && held > DEBOUNCE_MS) {
      switch (state) {
        case ST_VIEWFINDER: vfShort();  break;
        case ST_GALLERY:    galShort(); break;
        case ST_TRANSFER:   trShort();  break;
        default: break;
      }
    }
    // Ipucu gosterildiyse temizle (durum degismediyse).
    // Durum degistiyse ekran zaten bastan cizildi; sadece bayragi dusur.
    if (hint_shown && !long_fired) uiHintClear();
    else                           ui_hint_active = false;
    hint_shown = false;
  }
}

// Bir sonraki durumun adi — ipucu kutusunda gosterilir
const char *stateNextName() {
  switch (state) {
    case ST_VIEWFINDER: return "-> GALERI";
    case ST_GALLERY:    return "-> AKTARIM";
    case ST_TRANSFER:   return "-> VIZOR";
    default:            return "->";
  }
}

// ===========================================================================
void setup() {
  // SERI YOK — GPIO 1 ekran CS'i.
  pinMode(BTN_PIN, INPUT_PULLUP);

  // --- Ekran (marka + bos ilerleme cubugu) ---
  tft.init();
  tft.setRotation(ROTATION);

  // ST7789 uyku cikisindan (SLPOUT) sonra kisa bir yerlesme suresi ister.
  delay(120);

  // Paneli HEMEN temizle. tft.init() panel RAM'ini sifirlamaz; icindeki
  // rastgele icerik ST7789'da genellikle duz kirmizi/turuncu olarak okunur.
  // Eskiden bu cop yalnizca uiBootBegin()'e kadar (120 ms) gorunurdu;
  // araya sdCheckFirmware() girince SD_MMC.begin()'in suresi kadar
  // uzadi ve acilista kirmizi bir flas olarak fark edilir hale geldi.
  tft.fillScreen(TFT_BLACK);

  // --- SD karttan kurtarma (HER SEYDEN ONCE) ---
  // Kartta firmware.bin varsa yazar ve yeniden baslatir; buradan donerse
  // guncelleme yok demektir. Kamera/Wi-Fi henuz baslatilmadi, yani bozuk
  // bir surum onlarda patlasa bile bu satira kadar gelinir.
  sdCheckFirmware();

  uiBootBegin();
  uiBootStep("Ekran hazirlaniyor", 15, true);
  delay(180);

  // --- Ayarlar + saat ---
  settingsLoad();
  timeLoad();
  uiBootStep("Ayarlar okundu", 30, true);
  delay(180);

  // --- Kare tamponu (PSRAM) ---
  fbuf = (uint16_t *)heap_caps_malloc((size_t)SCR_W * IMG_H * 2,
                                      MALLOC_CAP_SPIRAM);
  if (fbuf) memset(fbuf, 0, (size_t)SCR_W * IMG_H * 2);
  uiBootStep(fbuf ? "Kare tamponu ayrildi" : "PSRAM YOK - kontrol et",
             45, fbuf != nullptr);
  delay(180);

  // --- Fotograf listesi (PSRAM) ---
  photo_list = (int *)heap_caps_malloc(sizeof(int) * PHOTO_CAP,
                                       MALLOC_CAP_SPIRAM);

  // --- SD ---
  uiBootStep("SD kart taraniyor", 55, true);
  sd_ok = sdInit();
  if (sd_ok) scanPhotos();
  char sdmsg[32];
  if (sd_ok) snprintf(sdmsg, sizeof(sdmsg), "SD hazir - %d fotograf", photo_n);
  else       snprintf(sdmsg, sizeof(sdmsg), "SD kart yok");
  uiBootStep(sdmsg, 70, sd_ok);
  delay(180);

  // --- Acilis gunlugu (seri port olmadigi icin tek teshis kanali) ---
  if (sd_ok) logBoot();

  // --- JPEG cozucu ---
  TJpgDec.setSwapBytes(true);
  uiBootStep("Cozucu hazir", 80, true);

  // --- Kamera (en uzun adim) ---
  uiBootStep("Kamera baslatiliyor", 85, true);
  cam_ok = cameraInit();
  uiBootStep(cam_ok ? "Kamera hazir" : "Kamera baslatilamadi",
             100, cam_ok);
  delay(400);

  // Onceki acilis normal bitmedi mi? Seri port olmadigi icin bunu
  // gormenin baska yolu yok. Brownout = batarya cokuyor demektir.
  const char *rw = resetWarning();
  if (rw) {
    uiBootDone(rw);
    delay(2500);
  } else {
    uiBootDone(cam_ok ? "HAZIR" : "HATA VAR");
    delay(cam_ok ? 900 : 2500);
  }

  stateEnter(ST_VIEWFINDER);
}

void loop() {
  switch (state) {
    case ST_VIEWFINDER: vfLoop();  break;
    case ST_GALLERY:    galLoop(); break;
    case ST_TRANSFER:   trLoop();  break;
    default: break;
  }
  buttonTask();
}
