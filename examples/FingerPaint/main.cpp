// Test sketch: LovyanGFX on the ES3C35P (ST77922 320x480 QSPI + built-in touch)
//
// Everything is drawn into a full-screen PSRAM sprite (`canvas`) and pushed at
// (0,0). The ST77922 needs even-aligned write windows, so don't draw text or
// thin shapes straight to `lcd` - see src/LGFX_ES3C35P.hpp.
//
// 1. Colour/edge test screen - 4 s.
// 2. Finger paint: drag to draw, tap a colour to switch, tap CLEAR to wipe.
#include <Arduino.h>
#include "LGFX_ES3C35P.hpp"

LGFX lcd;
LGFX_Sprite canvas(&lcd);

static void colourCheck()
{
  canvas.fillScreen(TFT_BLACK);

  // 1-px nested borders: white, red, green, blue - all must run full height.
  const uint16_t edge[] = { TFT_WHITE, TFT_RED, TFT_GREEN, TFT_BLUE };
  for (int i = 0; i < 4; ++i)
    canvas.drawRect(i, i, canvas.width() - 2 * i, canvas.height() - 2 * i, edge[i]);

  struct { uint16_t c; const char* name; } sw[] = {
    { TFT_RED, "RED" },     { TFT_GREEN, "GREEN" },   { TFT_BLUE, "BLUE" },
    { TFT_YELLOW, "YELLOW" }, { TFT_MAGENTA, "MAGENTA" }, { TFT_CYAN, "CYAN" },
  };
  const int cols = 2, w = 140, h = 70, x0 = 16, y0 = 60, gap = 8;
  canvas.setTextDatum(middle_center);
  canvas.setFont(&fonts::FreeSansBold9pt7b);
  for (int i = 0; i < 6; ++i) {
    int x = x0 + (i % cols) * (w + gap);
    int y = y0 + (i / cols) * (h + gap);
    canvas.fillRect(x, y, w, h, sw[i].c);
    canvas.setTextColor(TFT_BLACK);
    canvas.drawString(sw[i].name, x + w / 2, y + h / 2);
  }

  for (int x = 0; x < 288; ++x) {   // grey ramp
    uint8_t v = x * 255 / 287;
    canvas.drawFastVLine(16 + x, 310, 30, canvas.color565(v, v, v));
  }

  canvas.setTextDatum(top_left);
  canvas.setTextColor(TFT_WHITE);
  canvas.setFont(&fonts::FreeSansBold12pt7b);
  canvas.drawString("ES3C35P  ST77922", 16, 16);
  canvas.setFont(&fonts::Font2);
  canvas.setCursor(16, 360);
  canvas.printf("LovyanGFX %d.%d.%d  %dx%d  QSPI\n",
                LGFX_VERSION_MAJOR, LGFX_VERSION_MINOR, LGFX_VERSION_PATCH,
                lcd.width(), lcd.height());
  canvas.setCursor(16, canvas.getCursorY());
  canvas.printf("PSRAM free: %u KB\n", ESP.getFreePsram() / 1024);

  canvas.pushSprite(0, 0);
}

// ---- Finger paint ----
static const uint16_t palette[] = { TFT_WHITE, TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_MAGENTA };
static constexpr int N_COL = sizeof(palette) / sizeof(palette[0]);
static constexpr int BAR_H = 44;     // bottom toolbar
static constexpr int STATUS_H = 22;  // top status line
static uint16_t brush = TFT_WHITE;

static void drawToolbar()
{
  int y = canvas.height() - BAR_H;
  canvas.fillRect(0, y, canvas.width(), BAR_H, TFT_DARKGREY);
  int sw = 34;
  for (int i = 0; i < N_COL; ++i) {
    int x = 6 + i * (sw + 6);
    canvas.fillRoundRect(x, y + 5, sw, BAR_H - 10, 6, palette[i]);
    if (palette[i] == brush) canvas.drawRoundRect(x - 2, y + 3, sw + 4, BAR_H - 6, 8, TFT_ORANGE);
  }
  canvas.fillRoundRect(canvas.width() - 70, y + 5, 64, BAR_H - 10, 6, TFT_BLACK);
  canvas.setTextColor(TFT_WHITE);
  canvas.setTextDatum(middle_center);
  canvas.setFont(&fonts::Font2);
  canvas.drawString("CLEAR", canvas.width() - 38, y + BAR_H / 2);
  canvas.setTextDatum(top_left);
}

static void clearPaintArea()
{
  canvas.fillRect(0, STATUS_H, canvas.width(), canvas.height() - STATUS_H - BAR_H, TFT_BLACK);
}

static void status(const char* s)
{
  canvas.fillRect(0, 0, canvas.width(), STATUS_H, TFT_NAVY);
  canvas.setTextColor(TFT_WHITE);
  canvas.setFont(&fonts::Font2);
  canvas.drawString(s, 6, 3);
}

void setup()
{
  Serial.begin(115200);

  lcd.init();
  lcd.setRotation(0);
  lcd.setBrightness(200);

  canvas.setPsram(true);
  canvas.setColorDepth(16);
  if (!canvas.createSprite(lcd.width(), lcd.height())) {
    Serial.println("Sprite alloc failed - is PSRAM enabled (qio_opi)?");
    while (true) delay(1000);
  }
  Serial.printf("LCD %dx%d up, touch %s\n", lcd.width(), lcd.height(),
                lcd.touch() ? "configured" : "missing");

  colourCheck();
  delay(4000);

  canvas.fillScreen(TFT_BLACK);
  status("Touch to draw");
  drawToolbar();
  canvas.pushSprite(0, 0);
}

void loop()
{
  static bool wasDown = false;
  static int lastX = -1, lastY = -1;

  int32_t x, y;
  bool down = lcd.getTouch(&x, &y);
  bool changed = false;

  if (down) {
    if (y >= canvas.height() - BAR_H) {                    // toolbar
      if (!wasDown) {
        if (x >= canvas.width() - 70) clearPaintArea();
        else {
          int i = (x - 6) / 40;
          if (i >= 0 && i < N_COL) { brush = palette[i]; drawToolbar(); }
        }
      }
    } else if (y > STATUS_H + 4) {                          // paint area
      if (wasDown && lastX >= 0) canvas.drawWideLine(lastX, lastY, x, y, 4, brush);
      else                      canvas.fillCircle(x, y, 4, brush);
      lastX = x; lastY = y;
    }
    char buf[48];
    snprintf(buf, sizeof(buf), "x=%3ld  y=%3ld", (long)x, (long)y);
    status(buf);
    changed = true;
  } else if (wasDown) {
    lastX = lastY = -1;
  }
  wasDown = down;

  if (changed) canvas.pushSprite(0, 0);   // one full-screen push per frame
  else delay(5);
}
