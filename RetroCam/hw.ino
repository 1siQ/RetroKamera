/*
 * ============================================================================
 *  Retro Cam — hw.ino : kamera, SD kart, dosya isimlendirme
 * ============================================================================
 */

// --- AI Thinker ESP32-CAM kamera pinleri (sabit, degistirilemez) -----------
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM   0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27
#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM     5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

// ===========================================================================
//  KAMERA
// ===========================================================================

// Kalici ayarlari sensore uygula. Ayar degistiginde de cagrilir.
void cameraApplySettings() {
  sensor_t *s = esp_camera_sensor_get();
  if (!s) return;
  s->set_vflip(s, cfg.vflip);
  s->set_hmirror(s, cfg.hmirror);
  s->set_ae_level(s, cfg.ae_level);
  s->set_saturation(s, cfg.saturation);
  s->set_brightness(s, cfg.brightness);
  s->set_contrast(s, cfg.contrast);
}

bool cameraInit() {
  camera_config_t c = {};      // TUM alanlari sifirla (yoksa stack copu kalir)
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0 = Y2_GPIO_NUM;  c.pin_d1 = Y3_GPIO_NUM;
  c.pin_d2 = Y4_GPIO_NUM;  c.pin_d3 = Y5_GPIO_NUM;
  c.pin_d4 = Y6_GPIO_NUM;  c.pin_d5 = Y7_GPIO_NUM;
  c.pin_d6 = Y8_GPIO_NUM;  c.pin_d7 = Y9_GPIO_NUM;
  c.pin_xclk = XCLK_GPIO_NUM;   c.pin_pclk  = PCLK_GPIO_NUM;
  c.pin_vsync = VSYNC_GPIO_NUM; c.pin_href  = HREF_GPIO_NUM;
  c.pin_sccb_sda = SIOD_GPIO_NUM; c.pin_sccb_scl = SIOC_GPIO_NUM;
  c.pin_pwdn  = PWDN_GPIO_NUM;  c.pin_reset = RESET_GPIO_NUM;
  c.xclk_freq_hz = 20000000;

  // KRITIK: EN BUYUK boyutta baslat — tampon buna gore ayrilir.
  // Sonradan kucultmek serbest; baslangictan buyugune cikmak guvenilir degil.
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size   = FRAMESIZE_UXGA;
  c.jpeg_quality = cfg.cap_quality;
  c.fb_count     = 2;
  c.fb_location  = CAMERA_FB_IN_PSRAM;
  c.grab_mode    = CAMERA_GRAB_LATEST;

  if (esp_camera_init(&c) != ESP_OK) return false;

  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_special_effect(s, 0);      // 2 = siyah-beyaz olurdu, kapali kalsin
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_wb_mode(s, 0);
    s->set_exposure_ctrl(s, 1);
    s->set_aec2(s, 1);
    s->set_gain_ctrl(s, 1);
    s->set_gainceiling(s, GAINCEILING_32X);  // pozlamayi uzatmadan parlaklik
    s->set_raw_gma(s, 1);
    s->set_lenc(s, 1);
  }
  cameraApplySettings();
  cameraToViewfinder();
  return true;
}

// Sensorun oturmasini bekle. Gecis ekrani acikken animasyonlu bekler,
// aksi halde duz delay yapar.
static void settleWait(uint32_t ms) {
  if (ui_spin_active) uiSpinDelay(ms);
  else                delay(ms);
}

// Vizor moduna gec: kucuk boyut + hizli (cok sikistirilmis) kalite
void cameraToViewfinder() {
  sensor_t *s = esp_camera_sensor_get();
  if (!s) return;
  s->set_framesize(s, FRAMESIZE_QVGA);
  s->set_quality(s, Q_VIEWFINDER);
  settleWait(CAP_SETTLE_MS);
  cameraFlush(CAP_DISCARD);
}

// Cekim moduna gec: en buyuk boyut + en iyi kalite
void cameraToCapture() {
  sensor_t *s = esp_camera_sensor_get();
  if (!s) return;
  s->set_framesize(s, FRAMESIZE_UXGA);
  s->set_quality(s, cfg.cap_quality);
  settleWait(CAP_SETTLE_MS);
  cameraFlush(CAP_DISCARD);
}

// Boyut/kalite degisiminden sonraki ilk kareler BOZUK gelir — atilmali.
void cameraFlush(int n) {
  for (int i = 0; i < n; i++) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) esp_camera_fb_return(fb);
  }
}

// ===========================================================================
//  SD KARTTAN KURTARMA
// ===========================================================================
//  Acilista, her seyden once calisir. Kartta /firmware.bin varsa flash'a
//  yazar ve yeniden baslatir.
//
//  NEDEN VAR: bu cihazda seri port yok (GPIO 1 = ekran CS, GPIO 3 = buton).
//  Bozuk bir OTA yuklenirse geriye tek kurtarma yolu budur. Bu yuzden
//  setup()'in ILK islerinden biridir; kamera ya da Wi-Fi patlasa bile
//  buraya kadar gelinir.
//
//  Yukleme bitince dosya firmware.bak olarak yeniden adlandirilir,
//  boylece her acilista tekrar tekrar yazilmaz.
// ===========================================================================
bool sdCheckFirmware() {
  if (!SD_MMC.begin("/sdcard", true)) return false;   // 1-bit mod

  if (!SD_MMC.exists(SD_FW_PATH)) { SD_MMC.end(); return false; }

  File f = SD_MMC.open(SD_FW_PATH, FILE_READ);
  if (!f || f.isDirectory()) { if (f) f.close(); SD_MMC.end(); return false; }

  size_t sz = f.size();

  // Gecerli bir ESP32 uygulama imaji mi? Ilk bayt 0xE9 olmali.
  uint8_t magic = 0;
  f.read(&magic, 1);
  f.seek(0);

  if (sz < 100000 || magic != 0xE9) {
    f.close();
    SD_MMC.end();
    uiOtaBegin("SD FIRMWARE");
    uiOtaDone(false, "gecersiz dosya");
    delay(2500);
    return false;
  }

  uiOtaBegin("SD FIRMWARE");

  if (!Update.begin(sz)) {
    f.close(); SD_MMC.end();
    uiOtaDone(false, "yer yok");
    delay(2500);
    return false;
  }

  // writeStream tek hamlede yazar; ilerleme icin elle parca parca okuyoruz.
  // static: setup() yigini 8 KB, 4 KB'lik tamponu yigina koymak riskli
  const size_t CH = 2048;
  static uint8_t buf[CH];
  size_t done = 0;
  bool err = false;
  while (done < sz) {
    size_t n = f.read(buf, CH);
    if (n == 0) { err = true; break; }
    if (Update.write(buf, n) != n) { err = true; break; }
    done += n;
    uiOtaProgress(done, sz);
  }
  f.close();

  if (err || !Update.end(true)) {
    Update.abort();
    SD_MMC.end();
    uiOtaDone(false, "yazma hatasi");
    delay(2500);
    return false;
  }

  // Tekrar tekrar yuklememesi icin adini degistir
  SD_MMC.remove(SD_FW_BAK);
  SD_MMC.rename(SD_FW_PATH, SD_FW_BAK);
  SD_MMC.end();

  uiOtaDone(true, "YENIDEN BASLATILIYOR");
  delay(1500);
  ESP.restart();
  return true;      // buraya hic gelinmez
}

// ===========================================================================
//  SD KART
// ===========================================================================
bool sdInit() {
  // NOT: Burada eskiden tft.init() cagriliyordu ("SD surucusu ekranin
  // pinlerini calabilir" onlemi olarak). KALDIRILDI cunku:
  //   1. Entegrasyon testinde SD_MMC'nin 4/12/13'u CALMADIGI dogrulandi.
  //   2. tft.init() panelin init dizisini bastan calistiriyor; bu sirada
  //      panel RAM'i cop icerik gosteriyor -> ACILIS EKRANI TURUNCUYA
  //      DONUYORDU. Ekran sonradan temizlenmedigi icin de oyle kaliyordu.
  return SD_MMC.begin("/sdcard", true);       // true = 1-bit mod
}

bool sdPresent() {
  File r = SD_MMC.open("/");
  if (!r) return false;
  bool ok = r.isDirectory();
  r.close();
  return ok;
}

// ===========================================================================
//  KART DOLULUK ORANI
// ===========================================================================
//  usedBytes() FAT'i tarar, pahali olabilir — onbellege alinir.
//  force=true cekimden hemen sonra gercek degeri almak icin.
// ===========================================================================
static int      sd_free_pct = -1;
static uint32_t sd_free_t   = 0;

int sdFreePct(bool force) {
  if (!sd_ok) return -1;
  uint32_t now = millis();
  if (!force && sd_free_pct >= 0 && (now - sd_free_t) < 15000) return sd_free_pct;

  uint64_t tot = SD_MMC.totalBytes();
  uint64_t use = SD_MMC.usedBytes();
  sd_free_t = now;
  if (tot == 0) { sd_free_pct = -1; return -1; }

  sd_free_pct = (int)(100 - (use * 100 / tot));
  if (sd_free_pct < 0)   sd_free_pct = 0;
  if (sd_free_pct > 100) sd_free_pct = 100;
  return sd_free_pct;
}

// ===========================================================================
//  ACILIS GUNLUGU
// ===========================================================================
//  Bu cihazda seri port YOK. Kilitlenme, brownout ya da watchdog sifirlamasi
//  oldugunda geriye hicbir iz kalmiyordu. Her acilista bir satir yazilir;
//  sorun yasadiginda karti takip /log.txt'ye bakmak yeterli.
// ===========================================================================
static const char *resetName(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "guc";
    case ESP_RST_EXT:       return "harici-reset";
    case ESP_RST_SW:        return "yazilim";
    case ESP_RST_PANIC:     return "PANIK";
    case ESP_RST_INT_WDT:   return "INT-WDT";
    case ESP_RST_TASK_WDT:  return "TASK-WDT";
    case ESP_RST_WDT:       return "WDT";
    case ESP_RST_DEEPSLEEP: return "uyku";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    default:                return "bilinmiyor";
  }
}

// Onceki acilis anormal bittiyse ekranda gosterilecek uyari, yoksa NULL
const char *resetWarning() {
  switch (esp_reset_reason()) {
    case ESP_RST_BROWNOUT: return "BESLEME DUSTU";
    case ESP_RST_PANIC:    return "COKME OLDU";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:      return "KILITLENDI";
    default:               return NULL;
  }
}

void logBoot() {
  if (!sd_ok) return;

  // Gunluk sismesin: sinira gelince bastan yazilir
  File chk = SD_MMC.open(SD_LOG_PATH, FILE_READ);
  bool wipe = false;
  if (chk) { wipe = (chk.size() > SD_LOG_MAX); chk.close(); }

  File f = SD_MMC.open(SD_LOG_PATH, wipe ? FILE_WRITE : FILE_APPEND);
  if (!f) return;

  char ts[24];
  timeStamp(ts, sizeof(ts));

  f.printf("%s  v%s  reset=%s  heap=%u  psram=%u  foto=%d  bos=%%%d\n",
           ts, FW_VERSION, resetName(esp_reset_reason()),
           (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram(),
           photo_n, sdFreePct(true));
  f.close();
}

// ---------------------------------------------------------------------------
//  FOTOGRAF TARAMASI
// ---------------------------------------------------------------------------
//  ESKI DAVRANIS: 1'den baslayip ilk bos numarada duruyordu. Bu, "hicbir
//  fotograf silinmez" varsayimina dayaniyordu. Web arayuzune silme
//  eklendigi an o varsayim coktu:
//    - ortadaki bir dosya silinirse tarama orada dururdu
//    - sonraki fotograflar cihaz galerisinde gorunmez olurdu
//    - daha kotusu, yeni cekimler mevcut dosyalarin USTUNE yazardi
//
//  YENI DAVRANIS: dizin bir kez taranir, gercek numaralar listeye alinir,
//  siralanir. next_idx = en buyuk + 1 (numaralar geri kullanilmaz).
// ---------------------------------------------------------------------------
static int cmpInt(const void *a, const void *b) {
  return (*(const int *)a) - (*(const int *)b);
}

// "IMG_0042.jpg" -> 42 ; uymuyorsa -1
static int parseImgName(const char *name) {
  const char *b = strrchr(name, '/');
  b = b ? b + 1 : name;

  if (strncmp(b, "IMG_", 4) != 0) return -1;
  for (int i = 4; i < 8; i++) if (b[i] < '0' || b[i] > '9') return -1;

  // Uzanti karsilastirmasi buyuk/kucuk harf duyarsiz. strcasecmp yerine elle
  // yapiliyor; o fonksiyon <strings.h>'de ve her kurulumda gelmeyebiliyor.
  const char *e = b + 8;
  const char *w = IMG_SUFFIX;
  while (*w) {
    char c = *e;
    if (c >= 'A' && c <= 'Z') c += 32;
    if (c != *w) return -1;
    e++; w++;
  }
  if (*e != '\0') return -1;

  return atoi(b + 4);
}

int scanPhotos() {
  photo_n = 0;
  int maxi = 0;

  if (!sd_ok || !photo_list) { next_idx = 1; return 0; }

  File root = SD_MMC.open("/");
  if (!root || !root.isDirectory()) { next_idx = 1; return 0; }

  File f = root.openNextFile();
  while (f) {
    if (!f.isDirectory()) {
      int idx = parseImgName(f.name());
      if (idx > 0 && idx <= MAX_IMG) {
        if (photo_n < PHOTO_CAP) photo_list[photo_n++] = idx;
        if (idx > maxi) maxi = idx;
      }
    }
    f = root.openNextFile();
  }
  root.close();

  if (photo_n > 1) qsort(photo_list, photo_n, sizeof(int), cmpInt);

  next_idx = maxi + 1;
  if (next_idx > MAX_IMG) next_idx = MAX_IMG;
  return photo_n;
}

// Listedeki konumdan dosya numarasi (0 = en eski)
int photoAt(int pos) {
  if (pos < 0 || pos >= photo_n) return 0;
  return photo_list[pos];
}

// Bir dosya numarasinin listedeki konumu; yoksa -1
int photoPos(int idx) {
  for (int i = 0; i < photo_n; i++) if (photo_list[i] == idx) return i;
  return -1;
}

void imgPath(char *out, size_t n, int idx) {
  snprintf(out, n, IMG_PREFIX "%04d" IMG_SUFFIX, idx);
}

// Kart takildi/cikarildi mi? Durum degisirse true doner (ekran tazelenmeli).
bool sdWatchChanged() {
  static uint32_t poll_t = 0;
  uint32_t now = millis();
  if (now - poll_t < 1500) return false;
  poll_t = now;

  if (sd_ok) {
    if (!sdPresent()) {                 // cikarildi
      SD_MMC.end();
      sd_ok = false;
      return true;
    }
  } else {
    SD_MMC.end();
    if (SD_MMC.begin("/sdcard", true)) { // takildi
      sd_ok = true;
      scanPhotos();
      // tft.init() BURADA DA CAGRILMIYOR (yukaridaki sebeple) — sadece
      // ekrani temizleyip cagirana HUD'u tazeletmek yeterli.
      tft.fillScreen(TFT_BLACK);
      return true;
    }
  }
  return false;
}
