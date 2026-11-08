# ES3C35P — Hosyond ESP32-S3 Touchscreen Module (ST77922) for LovyanGFX

LovyanGFX configuration for the **Hosyond ES3C35P ESP32-S3 Touchscreen Module**: ESP32-S3R8 (8 MB OPI PSRAM, 16 MB flash) with a 3.5" 320x480 **ST77922** IPS LCD over QSPI and the ST77922's built-in capacitive touch controller.

LovyanGFX has no ST77922 driver, so this library provides:

- `lgfx::Panel_ST77922` — `Panel_ST77916` with the vendor init sequence and panel size
- `lgfx::Touch_ST77922` — driver for the integrated touch controller (I2C 0x55, up to 10 points)
- `LGFX` — a ready-to-use device class with the board's pins, backlight and touch wired up

## Install (PlatformIO)

```ini
lib_deps =
    https://github.com/JasonBolbach/ES3C35P.git#v1.0.1
```

LovyanGFX (`^1.3.0`) is pulled in automatically.

The board also needs these settings (OPI PSRAM in the wrong mode returns garbage from PSRAM sprites):

```ini
[env:es3c35p]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
board = esp32-s3-devkitc-1
framework = arduino
board_build.flash_mode = qio
board_build.psram_type = opi
board_build.arduino.memory_type = qio_opi
board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216
board_build.partitions = default_16MB.csv
build_flags =
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
    -DBOARD_HAS_PSRAM
lib_deps =
    https://github.com/JasonBolbach/ES3C35P.git#v1.0.1
```

## Usage

```cpp
#include <Arduino.h>
#include <LGFX_ES3C35P.hpp>

LGFX lcd;
LGFX_Sprite canvas(&lcd);

void setup() {
  lcd.init();
  lcd.setBrightness(200);
  canvas.setPsram(true);
  canvas.createSprite(lcd.width(), lcd.height());
  canvas.fillScreen(TFT_BLACK);
  canvas.drawString("Hello", 20, 20);
  canvas.pushSprite(0, 0);
}

void loop() {
  int32_t x, y;
  if (lcd.getTouch(&x, &y)) { /* ... */ }
}
```

See [examples/FingerPaint](examples/FingerPaint/main.cpp) for a full demo.

## Important: column-pair rule

The ST77922 writes columns in pairs: every write window must start at an **even x** and have an **even width**. Text, circles and 1-px lines drawn straight to the panel come out garbled. Draw into an `LGFX_Sprite` and `pushSprite()` it at an even x with an even width — a full-screen sprite at (0,0) always works (~37 fps). Large even-aligned fills directly on the panel are fine.

## Configuration

| Define | Default | Notes |
|---|---|---|
| `LCD_SPI_FREQ` | `60000000` | QSPI write clock. 80 MHz corrupts data under LovyanGFX. |
| `ST77922_COLMOD_RGB565` | `0x55` | COLMOD value for 16-bit colour. |

Touch hold time (how long the last points are kept between reports) can be tuned with `lcd.touchDriver().hold_ms = 80;`.

## Pins

| Function | GPIO |
|---|---|
| LCD QSPI CS / SCLK | 10 / 12 |
| LCD QSPI D0 / D1 / D2 / D3 | 11 / 13 / 14 / 9 |
| Backlight | 41 |
| LCD reset | not wired |
| Touch SDA / SCL | 38 / 39 |
| Touch RST / INT | 48 / 47 |

## Developing

Open this folder in PlatformIO; the root `platformio.ini` builds `examples/FingerPaint` against `src/`.

## License

MIT
