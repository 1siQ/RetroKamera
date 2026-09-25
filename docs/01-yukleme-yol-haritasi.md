# Retro Cam — Kablolu Yükleme Yol Haritası

Amaç: kabloyu **son kez** takmak. Bu oturumun sonunda cihaz kablosuz
güncellenebilir olacak ve bir daha TTL'e dönmeyeceksin.

Sıra önemli. Özellikle ADIM 2 atlanırsa geri dönüşü yok.

---

## ADIM 0 — Bilgisayar tarafı (donanıma dokunmadan)

SSD değiştiği için Arduino ortamı büyük ihtimalle sıfırdan kurulu.
Aşağıdakiler **kodun çalışması için şart**, eksikse ekran açılmaz ve
sen kodu suçlarsın.

- [ ] Arduino IDE kurulu
- [ ] Boards Manager → **esp32 by Espressif Systems** kurulu
- [ ] Library Manager → **TFT_eSPI** (Bodmer) kurulu
- [ ] Library Manager → **TJpg_Decoder** (Bodmer) kurulu
- [ ] **ricmoo/QRCode kurulu DEĞİL.** Kuruluysa kaldır — çekirdekteki
      `qrcode.h` ile aynı dosya adı, derleme çakışır.
- [ ] `pip install esptool`

### ⚠ EN SIK UNUTULAN ADIM

`User_Setup.h` dosyasının içeriğini şuraya yapıştır:

```
Belgeler\Arduino\libraries\TFT_eSPI\User_Setup.h
```

Kütüphane yeni kurulduğu için varsayılan ayarlarla geliyor. Yapıştırmazsan
ekran ya hiç açılmaz ya da çöp gösterir. Kütüphaneyi her güncellediğinde
bu ayar sıfırlanır.

### Kart ayarları (Tools menüsü)

| Ayar | Değer |
|---|---|
| Board | AI Thinker ESP32-CAM |
| PSRAM | **Enabled** (zorunlu) |
| Partition Scheme | **Minimal SPIFFS (1.9MB APP with OTA / 190KB SPIFFS)** |
| Upload Speed | 115200 (sorun çıkarsa düşür) |

> Partition şeması değişince NVS taşınır, **kayıtlı kamera ayarların sıfırlanır.**
> Web arayüzünden bir kez yeniden ayarlarsın. Normal.
> "Huge APP" seçme — o şemada OTA bölmesi yok, tüm iş boşa gider.

---

## ADIM 1 — Donanım (güç kapalıyken bağla)

| TTL | ESP32-CAM | Not |
|---|---|---|
| TX | U0R / GPIO 3 | buton hattına paralel |
| RX | U0T / GPIO 1 | ekran CS hattına paralel |
| GND | GND | **şart** |
| 5V / 3V3 | **hiçbir yere** | ucunu izole et |

- [ ] TTL lojik seviyesi **3.3V**'a ayarlı (jumper/anahtar kontrol edildi)
- [ ] Batarya bağlı ve kartı besliyor
- [ ] Adaptörün güç ucu boşta ve bir yere değmiyor

### Boot moduna alma (her yüklemede)

1. GPIO 0 ↔ GND jumperını tak
2. RST'ye bas (ya da bataryayı kapat-aç)
3. Yükle
4. **Jumperı çıkar**, tekrar RST

GPIO 0 aynı zamanda kameranın XCLK'i — jumper takılı kalırsa kamera açılmaz.

---

## ADIM 2 — YEDEK ⛔ ATLANMAZ

Kartta şu an **çalışan gerçek son sürüm** duruyor; klasördeki koddan daha
yeni. Üzerine yazarsan o sürüm tamamen yok olur.

```bash
# 1. Bağlantı testi — port numaranı Aygıt Yöneticisi'nden al
esptool -p COM5 flash-id

# 2. Tam yedek (4MB, ~1.5 dk)
esptool -p COM5 -b 460800 read-flash 0 0x400000 backup.bin
```

> Eski esptool sürümlerinde komutlar alt çizgili: `flash_id`, `read_flash`.
> 460800 hata verirse `-b 115200` ile tekrar dene (~6 dk sürer).

- [ ] `backup.bin` tam 4.194.304 bayt
- [ ] Dosyanın ilk baytı `E9` (geçerli ESP32 imajı)
- [ ] **Yedek iki ayrı yere kopyalandı** (biri bulut — SSD bir daha bozulabilir)
- [ ] `strings backup.bin` çıktısına bir göz at, tanıdık metinler var mı

### Geri dönüş komutu

Her şey ters giderse, kartı bugünkü haline döndüren komut bu:

```bash
esptool -p COM5 write-flash 0 backup.bin
```

Bu komutu bir yere not et. Sigortan budur.

---

## ADIM 3 — Referans derleme (kod değiştirmeden)

Klasördeki mevcut kodu **olduğu gibi derle** (yükleme, sadece Verify).

Amacı: ortamın sağlam olduğunu kanıtlamak. Bunu atlarsan, benim yazdığım
kod derlenmediğinde sorunun kodda mı ortamda mı olduğunu bilemezsin.

- [ ] Derleme hatasız tamamlandı
- [ ] Çıktıdaki program boyutunu not al (OTA sonrası kıyaslamak için)

Hata alırsan buraya yaz — devam etmeden çözelim.

---

## ADIM 4 — Kod değişiklikleri (bende)

Bu adımda sen bekliyorsun. Yazılacaklar:

1. `camera_config_t c = {};` düzeltmesi
2. Vizörde uzun basış ipucunun görünmesi (`ui_hint_active`)
3. Galeride açık `setSwapBytes(false)`
4. `FW_VERSION` sabiti + ekranda ve web'de gösterimi
5. `/guncelle` sayfası — mevcut nav ve CSS'e oturacak
6. `ota_active` kilidi — yükleme sırasında buton devre dışı
7. `sdCheckFirmware()` — kartta `firmware.bin` varsa kurtarma

---

## ADIM 5 — İlk kablolu yükleme

- [ ] GPIO 0 jumper takılı, RST'ye basıldı
- [ ] Yükle
- [ ] Jumperı çıkar, RST
- [ ] Açılış ekranı geliyor, tüm adımlar OK
- [ ] Sürüm numarası ekranda görünüyor
- [ ] Vizör çalışıyor, çekim yapılıyor, galeri açılıyor
- [ ] Uzun basışta ipucu kutusu artık **vizörde de** görünüyor

Buraya kadar sorun yoksa: `Sketch > Export Compiled Binary` → `.bin` dosyasını al.

---

## ADIM 6 — OTA testi (kablo hâlâ takılı!)

Bu adım kabloyu sökmeden yapılmalı. Amaç OTA'nın gerçekten çalıştığını
görmek — sökeceğin kabloya bir daha ihtiyacın olmayacağından emin olmak.

- [ ] Uzun basışlarla AKTARIM moduna geç
- [ ] Telefon ya da laptop `Retro_Cam` ağına bağlandı
- [ ] `192.168.4.1/guncelle` açılıyor
- [ ] ADIM 5'te aldığın **aynı** `.bin` dosyasını yükle
- [ ] Ekranda ilerleme çubuğu ilerliyor
- [ ] Kart kendiliğinden yeniden başladı
- [ ] Sürüm numarası hâlâ doğru görünüyor

> Yükleme sırasında butona dokunma. `ota_active` kilidi koruyor ama
> gereksiz yere test etme.

---

## ADIM 7 — SD kurtarma testi

- [ ] Aynı `.bin`'i SD karta `firmware.bin` adıyla kopyala
- [ ] Kartı tak, RST
- [ ] Açılışta "firmware güncelleniyor" ekranı çıkıyor
- [ ] Kart yeniden başlıyor
- [ ] Kartta dosya `firmware.bak` olmuş (tekrar tekrar yüklemiyor)

Bu adım çalışıyorsa, OTA tamamen ölse bile cihazı kurtarabilirsin demektir.

---

## ADIM 8 — Kabloları sök

ADIM 6 ve 7'nin **ikisi de** geçtiyse:

- [ ] GPIO 0 jumperı çıkarıldı
- [ ] **TTL TX kablosu çıkarıldı** (boştayken buton hattını yüksek sürer,
      buton okunamaz hale gelir)
- [ ] TTL RX ve GND çıkarıldı
- [ ] Kart tek başına açılıyor, buton düzgün çalışıyor

### İsteğe bağlı sigorta

Kart zaten açıkken **U0T / U0R / GND / 5V için 4 pinlik header lehimle.**
Beş dakika sürer. Her şeye rağmen bir gün kabloya dönmen gerekirse
lehim değil, sadece takıp çıkarma olur.

---

## Bir şeyler ters giderse

| Belirti | Muhtemel sebep |
|---|---|
| `esptool` kartı bulamıyor | GND bağlı değil / GPIO 0 jumperı yok / yanlış COM portu |
| Yükleme başlıyor, ortada kopuyor | Batarya zayıf, brownout — şarj et |
| Ekran çöp gösteriyor | `User_Setup.h` yapıştırılmamış |
| Derleme "sketch too big" | Partition şeması Minimal SPIFFS değil |
| `qrcode.h` çakışması | ricmoo/QRCode kütüphanesi kurulu, kaldır |
| Kamera açılmıyor | GPIO 0 jumperı takılı kalmış |
| Buton tepki vermiyor | TTL TX kablosu hâlâ bağlı |
| Kart hiç açılmıyor | `esptool write-flash 0 backup.bin` |
