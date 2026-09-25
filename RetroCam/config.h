/*
 * ============================================================================
 *  Retro Cam — config.h : pinler, sabitler, tipler
 * ============================================================================
 *  Burada YALNIZCA tanimlar bulunur; degisken tanimi yoktur.
 *  Paylasilan degiskenler RetroCam.ino icinde tanimlidir (ilk derlenen dosya).
 * ============================================================================
 */
#pragma once

// ---------------------------------------------------------------------------
//  PİNLER  (docs/04-pin-haritasi.md — kesinlesmis)
// ---------------------------------------------------------------------------
//  Ekran (TFT_eSPI User_Setup.h icinde de ayni degerler olmali):
//     SCL -> 13   SDA -> 4   DC -> 12 (+4.7k pulldown)   CS -> 1   RST -> 3.3V
//  SD_MMC 1-bit: 14 (CLK) / 15 (CMD) / 2 (D0)   — kutuphane yonetir
//  GPIO 16 KULLANILAMAZ (PSRAM'e ait)
//
//  !!! SERI PORT YOK: GPIO 1 = ekran CS. Serial.begin() CAGRILMAZ.
//      Cagrilirsa UART pini ele gecirir ve ekran bozulur.
// ---------------------------------------------------------------------------
#define BTN_PIN         3        // tek buton: bir ucu GPIO 3, digeri GND

// ---------------------------------------------------------------------------
//  EKRAN YERLEŞİMİ  (yatay montaj — docs/02-mekanik.md)
// ---------------------------------------------------------------------------
#define ROTATION        1        // goruntu bas asagi ise 3 yap
#define SCR_W         320
#define SCR_H         240
#define BAND_H         24        // ust/alt sabit bilgi seritleri
#define IMG_Y      BAND_H        // goruntu alani baslangici
#define IMG_H   (SCR_H - 2 * BAND_H)   // 192

// ---------------------------------------------------------------------------
//  BUTON ZAMANLAMASI
// ---------------------------------------------------------------------------
#define DEBOUNCE_MS    40
#define LONG_MS       800        // uzun basis esigi (durum degistirir)
#define HINT_MS       500        // bu sureden sonra ekranda ipucu belirir

// ---------------------------------------------------------------------------
//  KAMERA  ("Yol 2" mimarisi — docs/05-dev-log.md)
//  Kamera UXGA+JPEG olarak acilir, HIC KAPATILMAZ (aktarim modu haric).
//  Vizor icin kucultulur, cekim icin buyutulur.
// ---------------------------------------------------------------------------
// Vizor JPEG kalitesi (DUSUK SAYI = IYI KALITE).
// 18 cok agresifti: QVGA zaten kucuk oldugu icin 8x8 JPEG bloklari ekranda
// gozle secilir hale geliyordu. 12'de gorunur sekilde temiz, kare hizindaki
// kayip ihmal edilebilir. Yavasladigini hissedersen 14-16'ya cik.
#define Q_VIEWFINDER   12

// Boyut degisiminden sonra sensorun otomatik pozlama (AEC) ve beyaz ayari
// (AWB) donguleri BASTAN yakinsar. 300 ms bunun icin kisaydi: cekim,
// sensor daha oturmadan yapiliyor ve fotograf soluk/yanlis pozlu cikiyordu.
// Vizor iyi gorunup fotografin kotu cikmasinin ana sebebi buydu.
#define CAP_SETTLE_MS 600        // boyut degisimi sonrasi bekleme
#define CAP_DISCARD     5        // boyut degisimi sonrasi atilacak kare

// ---------------------------------------------------------------------------
//  Wi-Fi / AKTARIM
// ---------------------------------------------------------------------------
#define AP_SSID     "Retro_Cam"
#define DNS_PORT       53
// Telefon QR'i okutunca aga OTOMATIK baglanma teklifi cikar (sifresiz ag):
#define QR_PAYLOAD  "WIFI:T:nopass;S:Retro_Cam;;"

// ---------------------------------------------------------------------------
//  DOSYA
// ---------------------------------------------------------------------------
#define IMG_PREFIX  "/IMG_"
#define IMG_SUFFIX  ".jpg"
#define MAX_IMG     9999
#define SD_LOG_PATH "/log.txt"
#define SD_LOG_MAX  32768        // gunluk bu boyutu asinca bastan yazilir

// Kart bu yuzdenin altina inince uyarilir / cekim durdurulur
#define SD_WARN_PCT     10
#define SD_STOP_PCT      2

// ---------------------------------------------------------------------------
//  FIRMWARE SURUMU
//  OTA'dan sonra "hangi surum yuklu" sorusunun tek cevabi budur.
//  Her yeni .bin uretmeden once ARTTIR.
// ---------------------------------------------------------------------------
#define FW_VERSION  "1.3.1"
#define FW_BUILD    __DATE__ " " __TIME__

// ---------------------------------------------------------------------------
//  KABLOSUZ GUNCELLEME (OTA)
//  Aktarim modundaki web sunucusuna ucuncu bir sekme olarak eklenir.
//  SD yolu, OTA olmese bile kurtarma imkani birakir.
// ---------------------------------------------------------------------------
#define OTA_PATH     "/guncelle"
#define SD_FW_PATH   "/firmware.bin"
#define SD_FW_BAK    "/firmware.bak"

// ---------------------------------------------------------------------------
//  DURUMLAR
//  Uzun basis dongusel olarak ilerletir:  VIZOR -> GALERI -> AKTARIM -> VIZOR
// ---------------------------------------------------------------------------
enum AppState {
  ST_VIEWFINDER = 0,
  ST_GALLERY    = 1,
  ST_TRANSFER   = 2,
  ST_COUNT      = 3
};

// ---------------------------------------------------------------------------
//  KALICI AYARLAR  (Preferences ile saklanir, web arayuzunden degistirilir)
//  Tek butonla menu yonetmek zor oldugu icin ayarlar CIHAZDA DEGIL,
//  aktarim modundaki web sayfasindadir.
// ---------------------------------------------------------------------------
struct Settings {
  int8_t  ae_level;      // pozlama  -2..+2
  int8_t  saturation;    // doygunluk -2..+2
  int8_t  brightness;    // parlaklik -2..+2
  int8_t  contrast;      // kontrast  -2..+2
  uint8_t cap_quality;   // cekim JPEG kalitesi 8..20 (dusuk = kaliteli)
  uint8_t vflip;         // dikey cevir  0/1
  uint8_t hmirror;       // yatay ayna   0/1
};

// Ayar surumu. Bu sayi degisince kayitli ayarlar otomatik olarak yeni
// varsayilanlara doner. Aksi halde NVS'teki eski degerler kalir ve
// varsayilan degisikligi hicbir ise yaramaz.
#define SETTINGS_REV     3

// Varsayilanlar — ilk acilista, surum degisiminde ve "sifirla" komutunda
//
// SOLUK FOTOGRAF DUZELTMESI (rev 2):
//   ae_level   +2 -> 0 : +2 pozlamayi surekli yukari itiyordu. Asiri pozlama
//                        parlak alanlari yakar, renkler yikanmis gorunur.
//                        "Renksiz" sikayetinin birinci sebebi buydu.
//   brightness +1 -> 0 : ayni etkiyi ustune bindiriyordu.
//   contrast   +1       : ONCE +2 denendi, GERI ALINDI. Kontrast artisi
//                        algilanan doygunlugu artiriyor ama vizordeki
//                        JPEG blok artefaktlarini da belirginlestiriyordu.
//                        Sikistirma zaten sinirdayken kontrast bedava degil.
//   saturation +2       : zaten tavanda, degismedi.
#define DEF_AE_LEVEL     0
#define DEF_SATURATION   2
#define DEF_BRIGHTNESS   0
#define DEF_CONTRAST     1
#define DEF_CAP_QUALITY 10
#define DEF_VFLIP        1
#define DEF_HMIRROR      0

// ---------------------------------------------------------------------------
//  FOTOGRAF LISTESI
//  Silme eklendigi icin numaralar artik kesintisiz DEGIL. Galeri, dizinden
//  taranan gercek numara listesi uzerinde gezer.
// ---------------------------------------------------------------------------
#define PHOTO_CAP    4000        // listede tutulacak azami fotograf
