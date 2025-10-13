#include <tinyNeoPixel.h>

#define NUMLEDS 25     // LEDの数 (ATtiny402で25まで？)
#define LED_PIN 1

tinyNeoPixel leds = tinyNeoPixel(NUMLEDS, LED_PIN, NEO_GRB);

void setup() {
  leds.begin();
  leds.setBrightness(50); // 明るさを設定
  leds.clear();
  leds.show();
}

// 虹色パターンを生成する関数
void rainbow(int wait) {
  for (long firstPixelHue = 0; firstPixelHue < 65536; firstPixelHue += 256) {
    for (int i = 0; i < leds.numPixels(); i++) {
      int pixelHue = firstPixelHue + (i * 65536L / leds.numPixels());
      leds.setPixelColor(i, leds.gamma32(leds.ColorHSV(pixelHue)));
    }
    leds.show();
    delay(wait);
  }
}

void loop() {
  rainbow(10);
}
