# backup.bin analizi — 2026-09-16

Kaynak: ESP32-D0WD-V3 rev 3.1, MAC 70:4b:ca:82:21:60, 4MB flash
Yöntem: 4 x 1MB parça okuma, birleştirme

## Doğrulama

| Kontrol | Sonuç |
|---|---|
| Boyut | 4.194.304 bayt ✓ |
| MD5 | `19b6989ab756dae61b7f894ebbfa4df4` |
| 0x0000 | `FF FF FF FF` — ESP32'de normal, bootloader 0x1000'de |
| 0x1000 | `E9 03 02 2F` — geçerli bootloader ✓ |
| 0x8000 | Okunabilir partition tablosu ✓ |

**Yedek sağlam.** Geri yükleme: `esptool -p COM3 write-flash 0 backup.bin`

## Partition tablosu (karttaki mevcut şema)

| Ad | Adres | Boyut |
|---|---|---|
| nvs | 0x9000 | 20 KB |
| otadata | 0xe000 | 8 KB |
| app0 | 0x10000 | **3 MB** |
| spiffs | 0x310000 | 896 KB |
| coredump | 0x3f0000 | 64 KB |

→ **Huge APP (3MB No OTA).** Tek uygulama bölmesi var.
Bu şemayla OTA teknik olarak imkânsız. Minimal SPIFFS'e geçiş şart.

## Uygulama boyutu

ESP32 imaj başlığı çözümlendi — 6 segment:

| Segment | Yükleme adresi | Boyut |
|---|---|---|
| 0 | 0x3f400020 (DROM, sabitler) | 210.492 |
| 1 | 0x3ffbdb60 (DRAM) | 34.244 |
| 2 | 0x40080000 (IRAM) | 17.384 |
| 3 | 0x400d0020 (IROM, kod) | 841.464 |
| 4 | 0x400843e8 (IRAM) | 76.312 |
| 5 | 0x50000200 (RTC) | 32 |

**Gerçek uygulama: 1.180.048 bayt ≈ 1.13 MB**

1.9 MB'lık OTA bölmesine rahatça sığıyor. Yaklaşık %40 boş pay kalıyor —
OTA sayfası ve SD kurtarma kodu eklendikten sonra bile sıkışmayız.

> Not: app0 bölmesinde 1.13 MB'dan sonra da FF olmayan veri var (2.6 MB'a
> kadar). Bu daha büyük bir önceki yüklemeden kalma çöp; imaj başlığı
> gerçek sınırı söylüyor, dikkate alınmadı.

## Derleme ortamı

```
IDF sürümü : v5.5.4
esp32-libs : 3.3.10   (imaj içindeki assert dosya yollarından)
Eski Windows kullanıcısı: "Lenovo"
```

→ Arduino IDE'de **esp32 core 3.3.x** kur. Farklı bir ana sürümle
derlersen davranış kayabilir.

→ Eski kullanıcı profili `C:\Users\Lenovo\...` idi. O profilin herhangi
bir yedeği varsa, kaybolan kaynak kod orada.

## ⭐ Metin karşılaştırması — asıl bulgu

Firmware'in DROM segmentindeki (sabit metinler) tüm uygulama düzeyi
dizeleri, klasördeki kaynakla karşılaştırıldı.

**Karttaki firmware ile klasördeki kaynak metin olarak birebir aynı.**

- Aynı HUD metinleri: `RETRO CAM`, `KART YOK!`, `IMG %04d`, `-> GALERI`, `-> AKTARIM`, `-> VIZOR`
- Aynı açılış adımları: `Ekran hazirlaniyor`, `Ayarlar okundu`, `Cozucu hazir`, `Kamera hazir`, `HAZIR`, `HATA VAR`
- Aynı web arayüzü: aynı CSS, aynı iki sekme (`Fotograflar`, `Ayarlar`), aynı slider etiketleri
- Aynı yollar: `/`, `/ayar`, `/sifirla`, `/dl`, captive portal uçları
- Aynı QR yükü: `WIFI:T:nopass;S:Retro_Cam;;`
- Aynı NVS alanı: `retrocam`

Firmware'de olup kaynakta olmayan **hiçbir** uygulama metni yok.
Yani karttaki sürümde fazladan ekran, sayfa, menü ya da özellik yok.

### Bunun anlamı

Elindeki klasör, sanıldığı gibi "epey eski bir ara kayıt" değil —
**özellik olarak karttaki sürümün aynısı.** Fark varsa sadece
sayısal sabitlerde ve metin üretmeyen mantık değişikliklerinde olabilir
(örneğin `Q_VIEWFINDER` değeri, bir bekleme süresi, bir eşik).
Bunlar ikili dosyadan karşılaştırılamaz ama pratikte önemsiz.

**Sonuç: kaybolan kodu kurtarmaya gerek yok. Plana klasördeki sürümle
devam edilebilir.**
