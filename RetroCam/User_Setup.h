// ============================================================================
//  Retro Cam — TFT_eSPI User_Setup.h  (NİHAİ)
// ============================================================================
//  KURULUM:
//    Bu dosyanin icerigini TFT_eSPI kutuphanesinin User_Setup.h dosyasina
//    yapistir:  Belgeler/Arduino/libraries/TFT_eSPI/User_Setup.h
//    (Kutuphaneyi guncellersen bu ayar sifirlanir, tekrar yapistir.)
//
//  Degerler docs/04-pin-haritasi.md ile ayni olmali.
// ============================================================================

#define USER_SETUP_INFO "RetroCam"

// --- Panel: GMT020-02, ST7789V, 240x320 -----------------------------------
#define ST7789_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Dogrulandi: bu panelde BGR dogru. TFT_RGB iken sari -> camgobegi cikiyordu.
#define TFT_RGB_ORDER TFT_BGR

// --- Pinler ---------------------------------------------------------------
#define TFT_MOSI  4    // SDA   (flas LED'i SOKULMUS olmali)
#define TFT_SCLK 13    // SCL
#define TFT_DC   12    // DC    (4.7k pulldown -> GND, strapping pini)
#define TFT_CS    1    // CS    (surulmek ZORUNDA, GND'ye sabitlenemez)
#define TFT_RST  -1    // RST -> 3.3V'a sabit
// BLK yok: bu modulde arka isik VCC'den surekli acik.

// --- Yazi tipleri ---------------------------------------------------------
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4

// --- SPI ------------------------------------------------------------------
// 27 MHz dogrulandi. 40000000 denenebilir (daha akici); bozulma olursa geri dus.
#define SPI_FREQUENCY 27000000
