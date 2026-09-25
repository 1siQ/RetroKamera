# Bilinen sorunlar

Firmware v1.3.1 itibarıyla açık olan konular. Çoğu yazılımla çözülemiyor ya da
bir donanım değişikliği gerektiriyor.

**Vizörde gördüğünüz, çektiğinizle tam aynı değil.** Vizör QVGA görüntünün
merkezinden 320x192'lik bir kırpma, çekim ise tam kare UXGA. Üstte ve altta
%10 kadar fazladan alan fotoğrafa giriyor.

**Açılışta kısa bir kırmızı parlama var.** ESP32 boot ederken GPIO 1'e ROM
bootloader'ın seri mesajı basılıyor ve o pin ekranın CS'i. Ekranın SCLK'si de
o sırada boşta olduğu için panel rastgele veri yutuyor. Kod çalışmaya
başlamadan önce olduğu için yazılımla engellenemiyor; kalıcı çözüm GPIO 13 ile
GND arasına bir pull-down direnci.

**Pil ömrü kısa.** Cihazın boştaki hali yok, açık olduğu her an tam güçte
çalışıyor. Arka ışık doğrudan VCC'ye bağlı olduğu için hiç sönmüyor, ve buton
GPIO 3'te olduğundan (RTC pini değil) uykudan butonla uyanmak mümkün değil.
Yaklaşık 200 mA çekiyor.

**Fotoğraflarda hafif puslanma var.** Kontrast düşük ve renkler yıkanmış
görünüyor. Muhtemelen lens yüzeyi ya da modülün IR filtresiyle ilgili, yazılım
tarafında çözülecek bir şey değil.
