# Retro Cam — İnceleme ve Kablosuz Güncelleme Planı

Tarih: 2026-09-16
Kapsam: Desktop/Camera klasöründeki 8 dosyalık sürüm (son hali değil, ara kayıt)

---

## 1. Mevcut durum

| Dosya | Satır/Boyut | Rol | Değerlendirme |
|---|---|---|---|
| `RetroCam.ino` | 9.0 KB | Durum makinesi, buton, ayar kalıcılığı | Sağlam |
| `hw.ino` | 5.7 KB | Kamera, SD, dosya numaralama | 1 gerçek hata |
| `st_viewfinder.ino` | 4.6 KB | Vizör + HUD + çekim | 1 tasarım uyumsuzluğu |
| `st_gallery.ino` | 4.9 KB | Cihaz üstü galeri | Kırılgan varsayım |
| `st_transfer.ino` | 15.2 KB | AP + QR + captive portal + web | En olgun dosya |
| `ui.ino` | 9.0 KB | Çizim yardımcıları | 1 gerçek hata |
| `config.h` | 4.6 KB | Pin/sabit/tip | Temiz |
| `User_Setup.h` | 1.6 KB | TFT_eSPI ayarı | Temiz |

Mimari tutarlı: her durum `xEnter / xLoop / xExit / xShort` dörtlüsünü uyguluyor,
ortak değişkenler ilk derlenen dosyada, duruma özel olanlar `static`.
Kodu okuyarak kaybolan `docs/` klasörünün içeriğinin çoğu geri çıkarılabiliyor.

---

## 2. Bulgular

### A — Gerçek hatalar (düzeltilmeli)

**A1. `camera_config_t` sıfırlanmamış** — `hw.ino`, `cameraInit()`

```cpp
camera_config_t c;        // stack çöpü
```

Struct'ın tüm alanları doldurulmuyor. `sccb_i2c_port`, `fb_size` gibi alanlar
rastgele değerle kalıyor. Şu an çalışması şans eseri; core sürümü değişince
ya da stack düzeni kayınca açıklanamayan init hatası verir.
Düzeltme: `camera_config_t c = {};`

**A2. Uzun basış ipucu vizörde görünmüyor** — `RetroCam.ino` + `ui.ino`

`uiHint()` görüntü alanının içine çiziyor, ama `vfLoop()` her karede
`uiPushBuffer()` ile tüm görüntü alanını eziyor. Yani vizörde ipucu bir kare
sonra siliniyor — kullanıcı basılı tutarken hiçbir şey görmüyor.
Galeri ve aktarımda çalışıyor (orada sürekli çizim yok).
Düzeltme: `ui_hint_active` bayrağı; açıkken `vfLoop` tamponu basmayı atlasın.

**A3. `setSwapBytes` durumu örtük** — `ui.ino` + `st_gallery.ino`

`uiPushBuffer()` `tft.setSwapBytes(false)` yapıyor ve bu durum kalıcı.
Galeri `jpegToScreen()` içinde `pushImage` çağırırken o an ne ayarlıysa onu
kullanıyor. Şu an doğru çalışıyor çünkü açılışta her zaman önce vizöre
giriliyor. Çağrı sırası değişirse galeri renkleri bozulur.
Düzeltme: `galShowPhoto()` içinde açıkça `tft.setSwapBytes(false)`.

### B — Tasarım uyumsuzlukları (karar gerekiyor)

**B1. Vizör gördüğün ≠ çektiğin**

Vizör QVGA (320x240), görüntü alanı 320x192. `jpegToBuffer()` üstten ve
alttan 24'er satırı atıyor — yani vizör merkezden kırpılmış görüntü.
Çekim ise tam kare UXGA (1600x1200). Vizörde çerçeveye almadığın şeyler
fotoğrafa giriyor.

Seçenekler:
- **Kabul et** — retro kameralarda optik vizör paralaksı zaten vardır, karakter sayılabilir.
- **CIF + 1/2 ölçek** — 400x296 çek, 200x148 çiz, ortala. En-boy oranı korunur, kırpma biter, letterbox olur.
- **Şeritleri inceltmek** — `BAND_H` 24 → 16 yapılırsa görüntü alanı 208 olur, kırpma azalır ama bitmez.

**B2. Galeri ile web galerisi farklı mantık kullanıyor**

Cihaz galerisi `1..next_idx-1` aralığının kesintisiz olduğunu varsayıyor.
Web galerisi ise dizini tarayıp ne bulursa listeliyor. Kullanıcı kartı
bilgisayara takıp ortadan bir dosya silerse: `findNextIndex()` boşlukta
durur, sonraki tüm fotoğraflar cihazda görünmez olur ama web'de görünür.
Ayrıca yeni çekimler eski dosyaların üzerine yazar.
Düzeltme: `findNextIndex()` boşlukta durmak yerine en büyük numarayı bulsun.

**B3. Dekoratif pil göstergesi**

`uiBattery()` her zaman dolu üç çubuk çiziyor. Yanıltıcı. Boş ADC pini yok
(GPIO 33 lehim gerektirir). Ya kaldırılmalı ya gerçek ölçüme bağlanmalı.

### C — Eksikler

- `docs/` klasörü yok (kodda 5 dosyaya atıf var: 02-mekanik, 04-pin-haritasi, 05-dev-log)
- **Firmware sürüm numarası yok.** OTA'dan sonra "hangi sürüm yüklü" sorusunun cevabı olmayacak. Kritik.
- Kablosuz güncelleme yok
- SD'den kurtarma yolu yok
- Fotoğraflarda zaman damgası yok (RTC yok, NTP yok — AP modunda internet de yok)

### D — Fazlalıklar

- `#include <SPI.h>` — TFT_eSPI zaten çekiyor, zararsız
- `vfExit()`, `galExit()`, `trShort()` boş — mimari simetri için, kalsın
- `sdWatchChanged()` kart yokken her 1.5 sn `end()`+`begin()` deniyor, vizörü hafifçe takıyor

---

## 3. Ana sorun: seri port yok

```
GPIO 1 = ekran CS     (UART TX)
GPIO 3 = buton        (UART RX)
```

Her iki UART pini de dolu. Sonuç:
- Her yükleme kabloyla uğraşmak demek
- Hiçbir seri debug çıktısı yok, tüm geri bildirim ekrandan
- Bozuk bir build yüklenirse elde tek kurtarma yolu yine kablo

Bu yüzden kablosuz güncelleme konfor değil, **altyapı gereği**.

### Neden AKTARIM modu OTA için hazır

`trEnter()` zaten şunları yapıyor:
1. `esp_camera_deinit()` — kamera kapalı, RAM boş
2. `WiFi.softAP("Retro_Cam")` — AP ayakta
3. `WebServer server(80)` — HTTP sunucusu çalışıyor
4. Nav çubuğu ve CSS hazır

Yani `ArduinoOTA` + mDNS zahmetine gerek yok. Mevcut sunucuya üçüncü bir
sekme eklemek yeterli: `/guncelle`.

---

## 4. Plan

### Faz 0 — TTL bağlıyken, kod değişmeden önce

- [ ] `esptool read-flash 0 0x400000 backup.bin` — mevcut çalışan firmware'in yedeği
- [ ] Bu yedeği güvenli bir yere kopyala (bulut dahil, SSD bir daha bozulabilir)
- [ ] Mevcut kodu bir git deposuna koy — bu klasör kaybolursa her şey biter

### Faz 1 — Zorunlu düzeltmeler

- [ ] A1: `camera_config_t c = {};`
- [ ] A2: `ui_hint_active` bayrağı, vizörde tampon basmayı atla
- [ ] A3: `galShowPhoto()` içinde açık `setSwapBytes(false)`
- [ ] `config.h` içine `#define FW_VERSION "1.0.0"` ve derleme tarihi

### Faz 2 — Kablosuz güncelleme

- [ ] `st_transfer.ino` içine `/guncelle` sayfası (mevcut CSS + nav'a oturacak)
- [ ] `Update` tabanlı upload handler
- [ ] Nav çubuğuna üçüncü sekme, sayfada yüklü sürüm gösterimi
- [ ] Ekranda yükleme ilerleme çubuğu (yüzde %5'te bir çizim)
- [ ] **`ota_active` bayrağı — açıkken `buttonTask()` erken dönsün.**
      Yükleme sırasında uzun basış `trExit()` çağırır, Wi-Fi kapanır,
      flash yarım kalır. Cihaz ölür. Bu maddeyi atlamak en pahalı hata olur.

### Faz 3 — SD kurtarma yolu

- [ ] `hw.ino` içine `sdCheckFirmware()`: kartta `/firmware.bin` varsa
      `Update.writeStream()` ile yaz, `firmware.bak` olarak yeniden adlandır, resetle
- [ ] `setup()` sırası: `tft.init()` → kısa bilgi ekranı → `sdCheckFirmware()` → geri kalan her şey
- [ ] Neden bu sıra: bozuk bir build kamera ya da Wi-Fi'de patlasa bile
      `setup()`'ın ilk satırları çalışır, kart takıp kurtarabilirsin

### Faz 4 — Derleme ayarları ve test protokolü

- [ ] Arduino IDE → Tools → Partition Scheme → **Minimal SPIFFS (1.9MB APP with OTA / 190KB SPIFFS)**
      Varsayılan şema iki OTA bölmesine yetmez. "Huge APP" seçilirse OTA hiç olmaz.
- [ ] **Uyarı: partition tablosu değişince NVS taşınır, kayıtlı ayarlar sıfırlanır.**
      Web'den yeniden ayarlarsın, bir kerelik.
- [ ] PSRAM: Enabled (zaten zorunlu)
- [ ] TTL hâlâ takılıyken sırayla:
      1. Yeni sketch'i kabloyla yükle, açılışın sorunsuz olduğunu gör
      2. `Sketch > Export Compiled Binary` ile `.bin` al
      3. AKTARIM moduna geç, telefondan/laptoptan `/guncelle` ile aynı `.bin`'i yükle
      4. Kartın yeniden başladığını ve sürüm numarasının göründüğünü doğrula
      5. SD karta `firmware.bin` koy, resetle, kurtarma yolunun da çalıştığını gör
      6. **Ancak bundan sonra** kabloları sök

### Faz 5 — Kablo söküldükten sonra, acelesi yok

- [ ] B1 kararı: vizör kırpması (CIF + 1/2 ölçek denemesi)
- [ ] B2: `findNextIndex()` boşluk toleransı
- [ ] B3: pil göstergesi kararı
- [ ] `docs/` yeniden yazımı — koddaki yorumlardan çıkarılabilir
- [ ] Web galerisinde sıralama (şu an FAT dizin sırası)

---

## 5. Öneri: bir de donanım sigortası

Kart zaten açıkken **U0T / U0R / GND / 5V için 4 pinlik header lehimle.**
Beş dakikalık iş. OTA her şeye rağmen ölürse bir daha lehim yapmazsın,
adaptörü takıp çıkarırsın.
