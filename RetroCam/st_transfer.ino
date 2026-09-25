/*
 * ============================================================================
 *  Retro Cam — st_transfer.ino : DURUM 3, Aktarım (Wi-Fi + QR + Web)
 * ============================================================================
 *      KISA basis : (islev yok)
 *      UZUN basis : Vizor'e don
 *
 *  AKIS
 *  ----
 *  1. Kamera KAPATILIR (esp_camera_deinit) — guc tasarrufu ve kamera ile
 *     Wi-Fi'yi ayni anda uyandirmamak icin (dev-log'daki brownout bulgusu).
 *  2. Access Point acilir: "Retro_Cam" (sifresiz, internet YOK).
 *  3. Ekranda QR uretilir: WIFI:T:nopass;S:Retro_Cam;;
 *     Telefon okutunca "bu aga baglan" teklifi cikar.
 *  4. Captive portal galeri sayfasini acar.
 *  5. Ayarlar da bu sayfadadir — tek butonla menu yonetmek zor oldugu icin
 *     ayarlar cihazda degil, burada.
 *
 *  ANDROID NOTU: varsayilan kamera uygulamasi WIFI: formatli QR'lari genelde
 *  TANIMAZ. Dogru yol: Ayarlar > Wi-Fi > QR ile baglan. Ekranda yaziyor.
 * ============================================================================
 */

// ===========================================================================
//  QR — ESP32 cekirdegindeki qrcode bileseni (ayri kutuphane gerekmez)
//  API geri cagirma tabanli oldugu icin cizim ayri fonksiyonda.
// ===========================================================================
static void qrDisplay(esp_qrcode_handle_t qr) {
  int n = esp_qrcode_get_size(qr);
  const int quiet = 4;                    // sessiz bolge — OKUMA ICIN SART
  const int total = n + quiet * 2;
  const int avail = 176;                  // QR'a ayrilan alan (sol taraf)
  int scale = avail / total;
  if (scale < 1) scale = 1;
  const int side = total * scale;
  const int ox = 8;
  const int oy = IMG_Y + (IMG_H - side) / 2;

  tft.fillRect(ox, oy, side, side, TFT_WHITE);    // sessiz bolge dahil zemin
  for (int y = 0; y < n; y++)
    for (int x = 0; x < n; x++)
      if (esp_qrcode_get_module(qr, x, y))
        tft.fillRect(ox + (x + quiet) * scale, oy + (y + quiet) * scale,
                     scale, scale, TFT_BLACK);
}

static void drawQR() {
  esp_qrcode_config_t c = {};
  c.display_func       = qrDisplay;
  c.max_qrcode_version = 5;
  c.qrcode_ecc_level   = ESP_QRCODE_ECC_LOW;

  if (esp_qrcode_generate(&c, QR_PAYLOAD) != ESP_OK) {
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(16, IMG_Y + 70);
    tft.print("QR YOK");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(16, IMG_Y + 100);
    tft.print(AP_SSID);
  }
}

// ===========================================================================
//  EKRAN
// ===========================================================================
static void trDrawScreen() {
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(12, 4);
  tft.print("AKTARIM");
  tft.setTextColor(wifi_on ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.setCursor(SCR_W - 96, 4);
  tft.print(wifi_on ? "WIFI ON" : "WIFI --");

  drawQR();

  // Yonergeler (sag taraf)
  const int TX = 196;
  int ty = IMG_Y + 8;
  tft.setTextSize(1);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(TX, ty); tft.print("ANDROID:");        ty += 12;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(TX, ty); tft.print("Ayarlar > Wi-Fi"); ty += 10;
  tft.setCursor(TX, ty); tft.print("> QR ile baglan"); ty += 18;

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(TX, ty); tft.print("IPHONE:");         ty += 12;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(TX, ty); tft.print("Kamerayi tut");    ty += 20;

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(TX, ty); tft.print("ELLE:");           ty += 12;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(TX, ty); tft.print(AP_SSID);           ty += 10;
  tft.setCursor(TX, ty); tft.print("192.168.4.1");     ty += 16;

  // Surum ve saat
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(TX, ty); tft.print("v" FW_VERSION);     ty += 10;

  char ts[24];
  timeStamp(ts, sizeof(ts));
  tft.setTextColor(time_set ? TFT_GREEN : TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(TX, ty); tft.print(ts);

  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(6, SCR_H - BAND_H + 5);
  tft.print("\"Internet yok\" uyarisi normaldir -");
  tft.setCursor(6, SCR_H - BAND_H + 15);
  tft.print("bildirime dokunun, galeri acilir.");

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(SCR_W - 76, SCR_H - BAND_H + 10);
  tft.print("UZUN=CIK");
}

// ===========================================================================
//  WEB — ortak stil
// ===========================================================================
static const char CSS[] PROGMEM =
  "<style>*{box-sizing:border-box}"
  "body{font-family:-apple-system,system-ui,sans-serif;background:#0b0b0c;"
  "color:#e8e8ea;margin:0;padding:0 0 32px}"
  "header{position:sticky;top:0;background:rgba(11,11,12,.94);"
  "backdrop-filter:blur(8px);border-bottom:1px solid #232326;padding:13px 16px;z-index:9}"
  ".t{font-size:15px;font-weight:700;letter-spacing:.14em}"
  ".s{font-size:12px;color:#8a8a92;margin-top:3px}"
  ".dot{display:inline-block;width:7px;height:7px;border-radius:50%;"
  "background:#e5484d;margin-right:7px;vertical-align:middle}"
  "nav{display:flex;gap:8px;padding:12px 16px 0}"
  "nav a{flex:1;text-align:center;padding:9px;border-radius:9px;"
  "background:#151517;border:1px solid #232326;color:#b9b9c0;"
  "text-decoration:none;font-size:13px}"
  "nav a.on{background:#e8e8ea;color:#0b0b0c;font-weight:600}"
  ".g{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));"
  "gap:12px;padding:16px}"
  ".c{background:#151517;border:1px solid #232326;border-radius:12px;overflow:hidden}"
  ".c img{width:100%;display:block;aspect-ratio:4/3;object-fit:cover;background:#0f0f10}"
  ".m{display:flex;align-items:center;justify-content:space-between;padding:8px 10px;gap:8px}"
  ".n{font-size:12px;color:#b9b9c0;font-family:ui-monospace,monospace}"
  ".d{font-size:12px;color:#0b0b0c;background:#e8e8ea;text-decoration:none;"
  "padding:5px 11px;border-radius:7px;font-weight:600}"
  ".e{text-align:center;color:#6d6d76;padding:56px 20px;font-size:14px}"
  "form{padding:16px}"
  ".f{background:#151517;border:1px solid #232326;border-radius:12px;"
  "padding:14px;margin-bottom:12px}"
  ".f label{display:block;font-size:13px;color:#b9b9c0;margin-bottom:8px}"
  ".f input[type=range]{width:100%}"
  ".v{float:right;color:#e8e8ea;font-family:ui-monospace,monospace}"
  ".row{display:flex;align-items:center;gap:10px;margin-bottom:10px}"
  "button{width:100%;padding:12px;border:0;border-radius:10px;"
  "background:#e8e8ea;color:#0b0b0c;font-size:14px;font-weight:600}"
  ".sec{background:#151517;color:#b9b9c0;border:1px solid #232326;margin-top:8px}"
  ".x{width:auto;margin-left:6px;padding:5px 10px;border-radius:7px;"
  "background:#2a1416;color:#ff6369;border:1px solid #4d1f23;"
  "font-size:12px;font-weight:600}"
  ".x.w{background:#e5484d;color:#fff;border-color:#e5484d}"
  ".act{display:flex;align-items:center}"
  "</style>";

static void sendHeader(const char *sub, int active) {
  server.sendContent(F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Retro Cam</title>"));
  server.sendContent_P(CSS);
  server.sendContent(F("</head><body><header><div class='t'>"
                       "<span class='dot'></span>RETRO CAM</div><div class='s'>"));
  server.sendContent(sub);
  server.sendContent(F(" &middot; v" FW_VERSION));
  server.sendContent(F("</div></header><nav>"));
  server.sendContent(active == 0 ? F("<a class='on' href='/'>Fotograflar</a>")
                                 : F("<a href='/'>Fotograflar</a>"));
  server.sendContent(active == 1 ? F("<a class='on' href='/ayar'>Ayarlar</a>")
                                 : F("<a href='/ayar'>Ayarlar</a>"));
  server.sendContent(active == 2 ? F("<a class='on' href='" OTA_PATH "'>Guncelle</a>")
                                 : F("<a href='" OTA_PATH "'>Guncelle</a>"));
  // Tarayicidan saat al. RTC yok, AP modunda internet de yok — ama sayfayi
  // acan telefon saati biliyor. Yerel saate cevrilip gonderilir; FAT dosya
  // damgalari da yerel saat bekler. Bu sayede fotograflar gercek tarih alir.
  server.sendContent(F("</nav>"
    "<script>(function(){var d=new Date();"
    "var t=Math.floor(d.getTime()/1000)-d.getTimezoneOffset()*60;"
    "var x=new XMLHttpRequest();x.open('GET','/saat?t='+t);x.send();})();"
    "</script>"));
}

// ---------------------------------------------------------------------------
//  Galeri sayfasi
// ---------------------------------------------------------------------------
//  Liste artik dizin sirasina (FAT ic sirasi) gore degil, scanPhotos()'un
//  sirali listesine gore uretilir — yeniden eskiye. Boylece web galerisi ile
//  cihaz galerisi ayni sirayi gosterir.
//
//  Her kartta iki asamali "Sil" butonu var: ilk tikta "Emin?" olur,
//  ikinci tikta siler. Silme geri alinamaz, o yuzden tek tikla olmamali;
//  tarayicinin confirm() kutusu ise bazi telefonlarda sayfayi dondurur.
// ---------------------------------------------------------------------------
static const char DEL_JS[] PROGMEM =
  "<script>"
  "function pad(i){return ('000'+i).slice(-4);}"
  "function del(b,i){"
  "if(b.dataset.c!='1'){"
  "b.dataset.c='1';b.textContent='Emin?';b.className='x w';"
  "setTimeout(function(){if(b.dataset.c=='1'){"
  "b.dataset.c='';b.textContent='Sil';b.className='x';}},4000);return;}"
  "b.textContent='...';b.disabled=true;"
  "var x=new XMLHttpRequest();x.open('POST','/sil');"
  "x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');"
  "x.onload=function(){"
  "if(x.status==200){var c=document.getElementById('c'+pad(i));if(c)c.remove();"
  "var g=document.querySelector('.g');"
  "if(g&&!g.children.length)location.reload();}"
  "else{b.textContent='Hata';b.disabled=false;b.dataset.c='';b.className='x';}};"
  "x.onerror=function(){b.textContent='Hata';b.disabled=false;"
  "b.dataset.c='';b.className='x';};"
  "x.send('f=IMG_'+pad(i)+'.jpg');}"
  "</script>";

static void handleIndex() {
  timeStore();          // son bilinen zamani koru
  if (sd_ok) scanPhotos();

  uint32_t total = 0;
  char path[32];
  for (int i = 0; i < photo_n; i++) {
    imgPath(path, sizeof(path), photo_list[i]);
    File f = SD_MMC.open(path, FILE_READ);
    if (f) { total += f.size(); f.close(); }
  }

  char sub[64];
  snprintf(sub, sizeof(sub), "%d fotograf &middot; %u KB", photo_n,
           (unsigned)(total / 1024));

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  sendHeader(sub, 0);

  if (photo_n == 0) {
    server.sendContent(F("<div class='e'>Kartta henuz fotograf yok.<br>"
                         "Vizore donup deklansore basin.</div>"
                         "</body></html>"));
    server.sendContent("");
    return;
  }

  server.sendContent(F("<div class='g'>"));

  char b[512];
  for (int i = photo_n - 1; i >= 0; i--) {        // yeniden eskiye
    int idx = photo_list[i];
    snprintf(b, sizeof(b),
      "<div class='c' id='c%04d'>"
      "<a href='/dl?f=IMG_%04d.jpg' target='_blank'>"
      "<img loading='lazy' src='/dl?f=IMG_%04d.jpg'></a>"
      "<div class='m'><span class='n'>IMG_%04d.jpg</span>"
      "<span class='act'><a class='d' href='/dl?f=IMG_%04d.jpg&d=1'>Indir</a>"
      "<button class='x' onclick='del(this,%d)'>Sil</button></span>"
      "</div></div>",
      idx, idx, idx, idx, idx, idx);
    server.sendContent(b);
  }

  server.sendContent(F("</div>"));
  server.sendContent_P(DEL_JS);
  server.sendContent(F("</body></html>"));
  server.sendContent("");
}

// ---------------------------------------------------------------------------
//  Saat kurulumu  (/saat?t=...)
// ---------------------------------------------------------------------------
//  Cihazda RTC yok ve AP modunda internet olmadigi icin NTP de yok.
//  Sayfayi acan tarayici kendi saatini gonderir. Deger NVS'e yazilir,
//  boylece guc kesilse bile acilista bir taban degeri olur.
// ---------------------------------------------------------------------------
static void handleTime() {
  if (!server.hasArg("t")) { server.send(400, "text/plain", String("eksik")); return; }

  long t = server.arg("t").toInt();
  if (t < 1700000000L) { server.send(400, "text/plain", String("gecersiz")); return; }

  struct timeval tv;
  tv.tv_sec  = (time_t)t;
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);

  time_set = true;
  timeStore();

  server.send(200, "text/plain", String("ok"));
}

// ---------------------------------------------------------------------------
//  Fotograf silme  (/sil)
// ---------------------------------------------------------------------------
//  Dosya adi KATI sekilde dogrulanir: yalnizca IMG_####.jpg silinebilir.
//  Aksi halde ".." ya da baska bir ad gonderip kartta ne varsa sildirmek
//  mumkun olurdu — firmware.bak dahil.
// ---------------------------------------------------------------------------
static void handleDelete() {
  if (!sd_ok)           { server.send(503, "text/plain", String("kart yok"));   return; }
  if (!server.hasArg("f")) { server.send(400, "text/plain", String("eksik")); return; }

  String n = server.arg("f");
  if (n.startsWith("/")) n = n.substring(1);

  bool valid = (n.length() == 12) && n.startsWith("IMG_");
  if (valid) for (int i = 4; i < 8; i++)
    if (n[i] < '0' || n[i] > '9') valid = false;
  if (valid) {
    String ext = n.substring(8);
    ext.toLowerCase();
    if (ext != ".jpg") valid = false;
  }
  if (!valid) { server.send(400, "text/plain", String("gecersiz ad")); return; }

  String path = "/" + n;
  if (!SD_MMC.exists(path)) { server.send(404, "text/plain", String("yok")); return; }
  if (!SD_MMC.remove(path)) { server.send(500, "text/plain", String("silinemedi")); return; }

  scanPhotos();          // liste ve next_idx guncellensin
  server.send(200, "text/plain", String("silindi"));
}

// ---------------------------------------------------------------------------
//  Ayarlar sayfasi
// ---------------------------------------------------------------------------
static void slider(const char *name, const char *label, int val,
                   int lo, int hi) {
  char b[320];
  snprintf(b, sizeof(b),
    "<div class='f'><label>%s<span class='v'>%d</span></label>"
    "<input type='range' name='%s' min='%d' max='%d' value='%d'></div>",
    label, val, name, lo, hi, val);
  server.sendContent(b);
}

static void handleSettings() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  sendHeader("Kamera ayarlari", 1);

  server.sendContent(F("<form method='POST' action='/ayar'>"));
  slider("ae",  "Pozlama (karanlik/aydinlik)", cfg.ae_level,   -2, 2);
  slider("sat", "Doygunluk (renk canliligi)",  cfg.saturation, -2, 2);
  slider("bri", "Parlaklik",                   cfg.brightness, -2, 2);
  slider("con", "Kontrast",                    cfg.contrast,   -2, 2);
  slider("q",   "Fotograf kalitesi (dusuk=iyi)", cfg.cap_quality, 8, 20);

  char b[420];
  snprintf(b, sizeof(b),
    "<div class='f'><div class='row'><input type='checkbox' name='vf' %s>"
    "<label style='margin:0'>Dikey cevir</label></div>"
    "<div class='row' style='margin:0'><input type='checkbox' name='hm' %s>"
    "<label style='margin:0'>Yatay ayna</label></div></div>"
    "<button type='submit'>Kaydet</button></form>"
    "<form method='POST' action='/sifirla' style='padding:0 16px'>"
    "<button class='sec' type='submit'>Varsayilanlara don</button></form>",
    cfg.vflip ? "checked" : "", cfg.hmirror ? "checked" : "");
  server.sendContent(b);

  server.sendContent(F("<div class='e' style='padding:20px 16px;font-size:12px'>"
    "Ayarlar cihazda kalici olarak saklanir.<br>"
    "Pozlamayi artirmak karanlik ortamda yardimci olur."
    "</div></body></html>"));
  server.sendContent("");
}

static int argInt(const char *n, int def) {
  if (!server.hasArg(n)) return def;
  return server.arg(n).toInt();
}

static void handleSettingsPost() {
  cfg.ae_level    = constrain(argInt("ae",  cfg.ae_level),   -2, 2);
  cfg.saturation  = constrain(argInt("sat", cfg.saturation), -2, 2);
  cfg.brightness  = constrain(argInt("bri", cfg.brightness), -2, 2);
  cfg.contrast    = constrain(argInt("con", cfg.contrast),   -2, 2);
  cfg.cap_quality = constrain(argInt("q",   cfg.cap_quality), 8, 20);
  cfg.vflip       = server.hasArg("vf") ? 1 : 0;
  cfg.hmirror     = server.hasArg("hm") ? 1 : 0;

  settingsSave();
  // Kamera bu modda kapali; ayarlar vizore donunce uygulanir.

  server.sendHeader("Location", "/ayar", true);
  server.send(303, "text/plain", "");
}

static void handleReset() {
  settingsReset();
  server.sendHeader("Location", "/ayar", true);
  server.send(303, "text/plain", "");
}

// ---------------------------------------------------------------------------
static void handleDownload() {
  if (!server.hasArg("f")) { server.send(400, "text/plain", "eksik"); return; }
  String p = server.arg("f");
  if (!p.startsWith("/")) p = "/" + p;
  File f = SD_MMC.open(p, FILE_READ);
  if (!f || f.isDirectory()) {
    server.send(404, "text/plain", "yok");
    if (f) f.close();
    return;
  }
  if (server.hasArg("d"))
    server.sendHeader("Content-Disposition",
                      "attachment; filename=" + p.substring(1));
  server.streamFile(f, "image/jpeg");
  f.close();
}

static void handleCaptive() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

// ===========================================================================
//  KABLOSUZ GUNCELLEME  (/guncelle)
// ===========================================================================
//  Bu cihazda seri port yok (GPIO 1 = ekran CS, GPIO 3 = buton), yani her
//  kablolu yukleme lehim/sokme demek. Aktarim modu OTA icin zaten hazir:
//  kamera kapali (RAM bos), AP ayakta, HTTP sunucusu calisiyor. Bu yuzden
//  ArduinoOTA + mDNS yerine mevcut sunucuya bir sayfa ekliyoruz.
//
//  GUVENLIK: yukleme basladigi anda ota_active = true olur ve buton tamamen
//  devre disi kalir. Aksi halde yanlislikla uzun basis trExit()'i cagirir,
//  Wi-Fi kapanir, flash yarim kalir ve cihaz bir daha acilmaz.
// ===========================================================================
static bool   ota_ok  = false;
static String ota_msg;
static uint32_t ota_len = 0;      // Content-Length (multipart yuku dahil)

static const char OTA_PAGE[] PROGMEM =
  "<div class='f'>"
  "<label>Firmware dosyasi (.bin)</label>"
  "<input type='file' id='fw' accept='.bin'>"
  "</div>"
  "<div style='padding:0 16px'><button id='go'>Yukle</button></div>"
  "<div class='f' id='st' style='display:none;margin:12px 16px'>"
  "<div style='height:8px;background:#232326;border-radius:4px;overflow:hidden'>"
  "<div id='pf' style='height:100%;width:0;background:#e8e8ea;transition:width .2s'></div>"
  "</div><div class='n' id='tx' style='margin-top:9px'>0%</div></div>"
  "<div class='e' style='padding:18px 16px;font-size:12px'>"
  "Arduino IDE &gt; Sketch &gt; Export Compiled Binary ile uretilen .bin"
  " dosyasini secin.<br><br>"
  "Yukleme sirasinda cihazin butonuna dokunmayin ve gucu kesmeyin.<br>"
  "Islem bitince cihaz kendiliginden yeniden baslar ve bu ag kapanir."
  "</div>"
  "<script>"
  "go.onclick=function(){"
  "var f=fw.files[0];"
  "if(!f){tx.textContent='Once dosya secin';st.style.display='block';return;}"
  "var d=new FormData();d.append('fw',f,f.name);"
  "var x=new XMLHttpRequest();"
  "st.style.display='block';go.disabled=true;"
  "x.upload.onprogress=function(e){"
  "if(!e.lengthComputable)return;"
  "var p=Math.round(e.loaded*100/e.total);"
  "pf.style.width=p+'%';tx.textContent=p+'%';};"
  "x.onload=function(){tx.textContent=x.responseText||'Bitti';};"
  "x.onerror=function(){tx.textContent='Baglanti koptu';go.disabled=false;};"
  "x.open('POST','" OTA_PATH "');x.send(d);};"
  "</script></body></html>";

static void handleUpdatePage() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  sendHeader("Kablosuz guncelleme", 2);
  server.sendContent_P(OTA_PAGE);
  server.sendContent("");
}

// Yukleme surerken parca parca cagrilir
static void handleUpdateUpload() {
  HTTPUpload &up = server.upload();

  if (up.status == UPLOAD_FILE_START) {
    ota_active = true;               // BUTON KILIDI
    ota_ok     = false;
    ota_msg    = "";
    ota_len    = server.header("Content-Length").toInt();

    uiOtaBegin("GUNCELLEME");

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      ota_msg = "baslatilamadi";
    }
  }
  else if (up.status == UPLOAD_FILE_WRITE) {
    if (!Update.hasError()) {
      if (Update.write(up.buf, up.currentSize) != up.currentSize)
        ota_msg = "yazma hatasi";
    }
    uiOtaProgress(up.totalSize, ota_len);
  }
  else if (up.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      ota_ok  = true;
      ota_msg = "tamam";
    } else {
      ota_msg = String("hata: ") + Update.errorString();
    }
  }
  else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    ota_msg = "iptal edildi";
  }
}

// Yukleme bitince (basarili ya da degil) cagrilir
static void handleUpdateDone() {
  if (ota_ok) {
    // String ile gonderiyoruz: send()'in const char* / String asiri
    // yuklemeleri arasinda belirsizlik kalmasin.
    server.send(200, "text/plain",
                String("Guncelleme tamam - cihaz yeniden baslatiliyor"));
    uiOtaDone(true, "YENIDEN BASLATILIYOR");
    delay(1200);
    ESP.restart();                   // buradan donus yok
  }

  String emsg = ota_msg.length() ? ota_msg : String("Guncelleme basarisiz");
  server.send(500, "text/plain", emsg);
  uiOtaDone(false, emsg.c_str());
  delay(2500);

  ota_active = false;                // buton kilidi kalksin
  trDrawScreen();                    // aktarim ekranina geri don
}

// ===========================================================================
//  DURUM GIRIS / CIKIS
// ===========================================================================
void trEnter() {
  // --- Gecis ekrani: siyah ekran yerine ne oldugunu goster ---
  ui_spin_active = true;
  uiTransition("AKTARIM", "hazirlaniyor");
  uiTransitionNote("kamera kapatiliyor", TFT_DARKGREY);
  uiSpinDelay(250);

  // Kamerayi kapat: guc tasarrufu + kamera ile Wi-Fi'yi ayni anda
  // uyandirmama (brownout onlemi)
  if (cam_ok) { esp_camera_deinit(); cam_ok = false; }

  uiTransitionNote("Wi-Fi aciliyor", TFT_YELLOW);
  uiSpinDelay(200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  IPAddress ip = WiFi.softAPIP();
  wifi_on = true;

  uiTransitionNote("QR hazirlaniyor", TFT_DARKGREY);
  uiSpinDelay(200);

  dns.start(DNS_PORT, "*", ip);

  // Rotalar YALNIZCA BIR KEZ kaydedilir.
  // server.stop() isleyici listesini temizlemez; her aktarim girisinde
  // yeniden kaydetseydik liste her turda buyur ve bellek sizardi.
  static bool routes_done = false;
  if (!routes_done) {
    routes_done = true;

  // OTA ilerleme cubugu icin gerekli: bu basligi acikca istemezsek
    // server.header("Content-Length") bos doner.
    static const char *ota_hdrs[] = { "Content-Length" };
    server.collectHeaders(ota_hdrs, 1);

    server.on("/",        HTTP_GET,  handleIndex);
    server.on("/ayar",    HTTP_GET,  handleSettings);
    server.on("/ayar",    HTTP_POST, handleSettingsPost);
    server.on("/sifirla", HTTP_POST, handleReset);
    server.on("/dl",      HTTP_GET,  handleDownload);
    server.on("/sil",     HTTP_POST, handleDelete);
    server.on("/saat",    HTTP_GET,  handleTime);
    server.on(OTA_PATH,   HTTP_GET,  handleUpdatePage);
    server.on(OTA_PATH,   HTTP_POST, handleUpdateDone, handleUpdateUpload);
    server.on("/generate_204",        handleCaptive);   // Android
    server.on("/hotspot-detect.html", handleCaptive);   // iOS / macOS
    server.on("/ncsi.txt",            handleCaptive);   // Windows
    server.onNotFound(handleCaptive);
  }

  server.begin();

  ui_spin_active = false;
  trDrawScreen();
}

void trExit() {
  ui_spin_active = true;
  uiTransition("VIZOR", "geri donuluyor");

  uiTransitionNote("Wi-Fi kapatiliyor", TFT_DARKGREY);
  server.stop();
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  wifi_on = false;
  uiSpinDelay(250);

  // Kamerayi yeniden baslat — web'den degistirilen ayarlar burada uygulanir
  uiTransitionNote("kamera aciliyor", TFT_CYAN);
  cam_ok = cameraInit();          // icindeki bekleme animasyonlu olur

  if (sd_ok) scanPhotos();
  // ui_spin_active BILEREK true birakiliyor: hemen ardindan vfEnter()
  // cagrilacak ve gecis ekranini ikinci kez cizmesin. vfEnter kapatir.
}

// ---------------------------------------------------------------------------
void trLoop() {
  dns.processNextRequest();
  server.handleClient();

  // Tarayici saati kurdugu anda ekrandaki tarihi guncelle (bir kez)
  static bool drawn_time = false;
  if (time_set && !drawn_time) { drawn_time = true; trDrawScreen(); }
  if (!time_set) drawn_time = false;
}

void trShort() {
  // Aktarim sayfasinda kisa basisin islevi yok.
  // (Kullanici yanlislikla bastiginda hicbir sey olmamasi kasitli.)
}
