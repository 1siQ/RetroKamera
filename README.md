# Retro Kamera

ESP32-CAM tabanlı, tek butonla kullanılan kompakt bir dijital fotoğraf makinesi.
Kendi Wi-Fi ağını kurar, fotoğrafları telefondan indirirsiniz, firmware
güncellemesi de aynı yerden yapılır.

## Ne yapıyor

Cihazın üç durumu var ve tek butonla yönetiliyor:

    VİZÖR  --uzun basış-->  GALERİ  --uzun basış-->  AKTARIM  --uzun basış-->  VİZÖR
      |                        |
    kısa: çek              kısa: önceki fotoğraf

**Vizör** canlı görüntüyü ve üstte/altta sabit bir bilgi şeridini gösterir.
Kısa basışta 1600x1200 (UXGA) fotoğraf çeker, sensörün ürettiği JPEG'i yeniden
sıkıştırmadan doğrudan SD karta yazar.

**Galeri** karttaki fotoğrafları cihaz ekranında gezdirir.

**Aktarım** kamerayı kapatıp bir erişim noktası açar (`Retro_Cam`, şifresiz).
Ekranda bir QR kod belirir; telefonu bağladığınızda captive portal galeri
sayfasını açar. Buradan fotoğrafları indirir, silersiniz; kamera ayarlarını
değiştirir ve firmware güncellersiniz.

Ayarlar bilerek cihazda değil web arayüzünde. Tek butonla menü gezdirmek
işkence olurdu ve deklanşörün anlık kalması her şeyden önemliydi. Bu yüzden
hiçbir yerde çift basış yok.

## Donanım

| Parça | Not |
|---|---|
| AI Thinker ESP32-CAM | PSRAM şart |
| GMT020-02 ekran | ST7789V, 240x320, yatay kullanılıyor |
| Mikroswitch | Tek buton |
| MicroSD kart | FAT32, azami 32 GB |
| LiPo + besleme devresi | Bkz. aşağıdaki pil notu |

### Pin haritası

    Ekran  SCL -> 13    SDA -> 4    DC -> 12 (4.7k pulldown)   CS -> 1    RST -> 3.3V
    SD_MMC 1 bit modu:  CLK -> 14   CMD -> 15   D0 -> 2
    Buton  GPIO 3 ile GND arasında

Kamera pinleri AI Thinker kartında sabit, değiştirilemiyor.

### Seri port yok

Bu projenin en can alıcı kısıtı bu. GPIO 1 ekranın CS'i, GPIO 3 butonun
girişi — yani her iki UART pini de dolu. Sonuçları:

- `Serial.begin()` çağrılmaz. Çağrılırsa UART pini ele geçirir ve ekran bozulur.
- Hiçbir hata ayıklama çıktısı yok, her geri bildirim ekrandan.
- Kablolu yükleme için FTDI takmak gerekiyor ki bu da cihazı sökmek demek.

Üçüncü madde yüzünden kablosuz güncelleme bir konfor değil, zorunluluk.

### Flaş LED'i

Modülün üzerindeki beyaz LED GPIO 4'te ve o pin ekranın SDA hattı.
LED sökülmeden ekran çalışmaz.

## Kablosuz güncelleme

Aktarım modunda kamera zaten kapalı, erişim noktası ayakta ve bir HTTP sunucusu
çalışıyor. Yeni bir şey kurmak yerine o sunucuya `/guncelle` sayfası eklendi:
Arduino IDE'nin ürettiği `.bin` dosyasını telefondan ya da bilgisayardan
yüklüyorsunuz, cihaz kendini güncelleyip yeniden başlıyor.

Yükleme başladığı anda buton tamamen devre dışı kalıyor. Aksi halde
yanlışlıkla uzun basmak Wi-Fi'yi kapatır, flash yarıda kalır ve cihaz bir daha
açılmaz.

### SD karttan kurtarma

Bozuk bir sürüm yüklenirse seri portu olmayan bir cihazda elde hiçbir şey
kalmaz. Bu yüzden açılışta, ekran hazırlandıktan hemen sonra ve kameradan
önce, kartta `firmware.bin` var mı diye bakılıyor. Varsa ve geçerli bir ESP32
imajıysa (ilk baytı `0xE9`) flash'a yazılıp yeniden başlatılıyor, dosya
`firmware.bak` olarak yeniden adlandırılıyor.

Bu kontrol bilerek `setup()`'ın en başında. Kamera ya da Wi-Fi'de patlayan bir
sürüm bile o satıra kadar geliyor.

## Kurulum

### Kütüphaneler

- TFT_eSPI (Bodmer)
- TJpg_Decoder (Bodmer)

QR kod için ayrı kütüphane gerekmiyor, ESP32 çekirdeğindeki `qrcode` bileşeni
kullanılıyor. **ricmoo/QRCode kuruluysa kaldırın**, aynı dosya adıyla çakışıyor.

### TFT_eSPI ayarı

`RetroCam/User_Setup.h` dosyasının içeriğini kütüphanenin kendi
`User_Setup.h`'ine yapıştırın:

    Belgeler/Arduino/libraries/TFT_eSPI/User_Setup.h

Kütüphaneyi her güncellediğinizde bu ayar sıfırlanır, tekrar yapıştırmak
gerekir. Ekran açılmıyorsa ya da çöp gösteriyorsa ilk bakılacak yer burası.

### Arduino IDE

| Ayar | Değer |
|---|---|
| Board | AI Thinker ESP32-CAM |
| Partition Scheme | Minimal SPIFFS (1.9MB APP with OTA) |
| Flash Mode | DIO |
| Flash Frequency | 80 MHz |
| CPU Frequency | 240 MHz |
| Core Debug Level | None |

Partition şeması önemli: varsayılan ve "Huge APP" şemalarında ikinci bir
uygulama bölmesi yok, yani OTA hiç çalışmaz.

Derlenen uygulama yaklaşık 1.17 MB, 1.9 MB'lık bölmenin %60'ı.

### İlk yükleme

İlk seferde kablo şart. FTDI'ın TX'i GPIO 3'e, RX'i GPIO 1'e, GND'si GND'ye.
Adaptörün güç ucunu bağlamayın, karta bataryadan besleyin — FTDI'ın regülatörü
ESP32-CAM'in akım tepelerine yetmiyor ve yükleme ortasında brownout oluyor.

GPIO 0'ı GND'ye kısa devre edip RST'ye basarak boot moduna alın, yükleyin,
jumper'ı çıkarıp tekrar resetleyin. GPIO 0 aynı zamanda kameranın XCLK'i,
takılı kalırsa kamera açılmaz.

Yükleme bitince **FTDI'ın TX kablosunu çıkarın.** Boşta kaldığında GPIO 3'ü
yüksek sürer ve buton okunamaz hale gelir.

Kabloları tamamen sökmeden önce `/guncelle` sayfasından bir OTA güncellemesi
yapıp çalıştığını doğrulayın. Sökeceğiniz kabloya bir daha ihtiyacınız olmadığından
emin olmadan sökmeyin.

## Kaynak düzeni

    RetroCam/
      RetroCam.ino        durum makinesi, buton, ayarlar, saat
      config.h            pinler, sabitler, tipler
      hw.ino              kamera, SD kart, fotoğraf taraması, açılış günlüğü
      st_viewfinder.ino   vizör
      st_gallery.ino      galeri
      st_transfer.ino     Wi-Fi, QR, web arayüzü, OTA
      ui.ino              ortak çizim yardımcıları
      User_Setup.h        TFT_eSPI ayarı (kütüphaneye kopyalanacak)
    docs/
      00-plan.md
      01-yukleme-yol-haritasi.md
      02-yedek-analizi.md
      03-bilinen-sorunlar.md

Arduino sketch'leri tek bir çeviri birimine birleştirdiği için paylaşılan
değişkenler `RetroCam.ino` içinde tanımlı, duruma özel olanlar kendi
dosyalarında `static`.

## Günlük

Cihazda seri port olmadığı için her açılışta SD karttaki `log.txt` dosyasına
bir satır yazılıyor: tarih, sürüm, reset sebebi, boş bellek, fotoğraf sayısı,
kartta kalan yer. Bir sorun yaşandığında kartı bilgisayara takıp oraya bakmak
tek teşhis yolu.

Önceki açılış anormal bittiyse açılış ekranında uyarı çıkıyor. `BESLEME DUSTU`
görüyorsanız sorun yazılımda değil, bataryada.

## Saat

RTC yok ve erişim noktası modunda internet de yok. Saati, aktarım sayfasını
açan tarayıcı veriyor; sayfa yüklenince kendi yerel saatini cihaza gönderiyor.
Bu sayede SD karta yazılan fotoğraflar gerçek tarih damgası alıyor. Son bilinen
zaman kalıcı belleğe yazılıyor, güç kesilse bile açılışta bir taban değer
oluyor.

## Sürüm

Şu anki firmware sürümü `1.3.1`. Sürüm numarası açılış ekranında ve web
arayüzünün başlığında görünüyor; OTA'dan sonra hangi sürümün yüklü olduğunu
anlamanın tek yolu bu.
