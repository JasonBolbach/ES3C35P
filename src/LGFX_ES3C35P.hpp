// LovyanGFX config for the ES3C35P (ESP32-S3 + 3.5" 320x480 ST77922 QSPI LCD)
//
// LovyanGFX has no ST77922 driver, but it does ship Panel_ST77916, a Sitronix
// QSPI LCD driver whose wire protocol is identical to what the vendor ST77922
// code does:
//   command : 0x02, 0x00, <cmd>, 0x00, params...    (all 1-line)
//   pixels  : 0x32, 0x00, 0x2C, 0x00 (1-line), then RGB565 data on 4 lines
// So Panel_ST77922 below is just Panel_ST77916 with the vendor init sequence
// (from ~/Downloads/cheap-black-display-board/st77922-driver-files ST77922.cpp)
// and the right panel size.
//
// !! COLUMN-PAIR RULE (confirmed on hardware) !!
// The ST77922 writes columns in pairs: every write window must start at an
// EVEN x and have an EVEN width. A direct 1x18 fillRect at odd x shows up as
// 2x9. Text, circles and 1-px lines drawn straight to the panel come out
// garbled. So: draw into an LGFX_Sprite and pushSprite() it at an even x
// with an even width. A full-screen sprite at (0,0) always works (~37 fps at
// 40 MHz). Large even-aligned fills (fillScreen, fillRect at even x/w) are
// fine directly.
//
// About the vendor "rotate pixels right by 5 bits" colour fix: not needed here.
// The red->blue, blue->green, green->orange pattern it describes is exactly what
// you get when RGB565 is sent little-endian. That driver sent uint16_t buffers
// straight out of memory (LE); LovyanGFX sends RGB565 big-endian, which is what
// the panel expects.
#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// ---- Board pins (from the working vendor/Launcher port) ----
#define LCD_QSPI_CS    10
#define LCD_QSPI_SCLK  12
#define LCD_QSPI_D0    11
#define LCD_QSPI_D1    13
#define LCD_QSPI_D2    14
#define LCD_QSPI_D3     9
#define LCD_BL         41
#define LCD_RST        -1   // not wired
// QSPI clock. The vendor driver uses 80 MHz, but that corrupted data under
// LovyanGFX (white drew as grey/green, frames lost). Override with -DLCD_SPI_FREQ=...
#ifndef LCD_SPI_FREQ
#define LCD_SPI_FREQ 60000000
#endif
#define LCD_W         320
#define LCD_H         480

// Integrated touch controller in the ST77922 package (driver: Touch_ST77922 below)
#define TOUCH_I2C_PORT  1   // lgfx I2C driver; port 1 leaves port 0 free for Arduino Wire
#define TOUCH_I2C_ADDR 0x55
#define TOUCH_SDA      38
#define TOUCH_SCL      39
#define TOUCH_RST      48
#define TOUCH_INT      47

// COLMOD value sent for 16-bit colour. 0x55 (standard MIPI) is confirmed
// correct on hardware; the vendor table's 0x01 is not needed.
#ifndef ST77922_COLMOD_RGB565
#define ST77922_COLMOD_RGB565 0x55
#endif

namespace lgfx { inline namespace v1 {

struct Panel_ST77922 : public Panel_ST77916
{
  Panel_ST77922(void)
  {
    _cfg.panel_width  = _cfg.memory_width  = LCD_W;
    _cfg.panel_height = _cfg.memory_height = LCD_H;
    _cfg.dummy_read_pixel = 8;
  }

  // Panel_ST77916::init() runs this list, then sends MADCTL and COLMOD itself.
  // INVON/MADCTL/COLMOD/RAMWR from the end of the vendor list are left out
  // because LovyanGFX manages those (cfg.invert, cfg.rgb_order, rotation).
  const uint8_t* getInitCommands(uint8_t listno) const override
  {
    static constexpr uint8_t list0[] = {
      0xF1,  1, 0x00,
      0x60,  3, 0x00, 0x00, 0x00,
      0x65,  1, 0x80,
      0x79,  1, 0x06,
      0x7B,  3, 0x00, 0x08, 0x08,
      0x80, 11, 0x55, 0x62, 0x2F, 0x17, 0xF0, 0x52, 0x70, 0xD2, 0x52, 0x62, 0xEA,
      0x81,  4, 0x26, 0x52, 0x72, 0x27,
      0x84,  2, 0x92, 0x25,
      0x87,  6, 0x10, 0x10, 0x58, 0x00, 0x02, 0x3A,
      0x88, 15, 0x00, 0x00, 0x2C, 0x10, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x06,
      0x89,  3, 0x00, 0x00, 0x00,
      0x8A, 11, 0x13, 0x00, 0x2C, 0x00, 0x00, 0x2C, 0x10, 0x10, 0x00, 0x3E, 0x19,
      0x8B,  9, 0x15, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x97, 0x8E,
      0x8C, 13, 0x1D, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x50, 0x0F, 0x01, 0xC5, 0x12, 0x09,
      0x8D,  1, 0x0C,
      0x8E,  6, 0x33, 0x01, 0x0C, 0x13, 0x01, 0x01,
      0xB3,  2, 0x00, 0x30,
      0xF1,  1, 0x00,
      0x71,  1, 0xD0,
      0x66,  2, 0x02, 0x3F,
      0xBE,  3, 0x26, 0x00, 0x9D,
      0x70, 12, 0x01, 0xA0, 0x11, 0x40, 0xE0, 0x00, 0x11, 0x69, 0x11, 0x00, 0x00, 0x1A,
      0x90,  9, 0x04, 0x04, 0x55, 0x74, 0x00, 0x40, 0x43, 0x27, 0x27,
      0x91,  9, 0x04, 0x04, 0x55, 0x75, 0x00, 0x40, 0x42, 0x27, 0x27,
      0x92, 10, 0x04, 0x44, 0x55, 0xC0, 0x06, 0x00, 0x07, 0x05, 0x90, 0x27,
      0x93, 10, 0x04, 0x43, 0x11, 0x00, 0x00, 0x00, 0x00, 0x05, 0x90, 0x27,
      0x94,  6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x95,  5, 0x96, 0x16, 0x00, 0x00, 0xFF,
      0x96, 12, 0x44, 0x53, 0x03, 0x12, 0x23, 0x24, 0x06, 0x05, 0x94, 0x27, 0x00, 0x44,
      0x97, 12, 0x44, 0x53, 0x47, 0x56, 0x20, 0x20, 0x02, 0x01, 0x94, 0x27, 0x00, 0x44,
      0xBA,  5, 0x55, 0x94, 0x2D, 0x94, 0x27,
      0x9A,  7, 0x40, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
      0x9B,  7, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
      0x9C, 13, 0x5C, 0x12, 0x00, 0x00, 0x10, 0x12, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00,
      0x9D,  8, 0x8A, 0x51, 0x00, 0x00, 0x00, 0x80, 0x1E, 0x01,
      0x9E,  7, 0x51, 0x00, 0x00, 0x00, 0x80, 0x1E, 0x01,
      0xB4, 12, 0x1D, 0x1C, 0x1E, 0x0B, 0x14, 0x02, 0x13, 0x09, 0x1E, 0x00, 0x1E, 0x10,
      0xB5, 12, 0x1D, 0x1C, 0x1E, 0x0A, 0x15, 0x03, 0x11, 0x08, 0x1E, 0x01, 0x1E, 0x12,
      0xB6,  7, 0x77, 0x77, 0x00, 0x0A, 0xFF, 0x0A, 0xFF,
      0x86, 14, 0xCD, 0x04, 0xB1, 0x02, 0x58, 0x12, 0x58, 0x0C, 0x13, 0x01, 0xA5, 0x00, 0xA5, 0xA5,
      0xB7, 16, 0x07, 0x0A, 0x0E, 0x06, 0x05, 0x03, 0x2B, 0x03, 0x03, 0x42, 0x07, 0x10, 0x10, 0x2E, 0x3F, 0x0D,
      0xB8, 16, 0x07, 0x0A, 0x0D, 0x05, 0x05, 0x02, 0x2B, 0x02, 0x03, 0x42, 0x06, 0x10, 0x0F, 0x2E, 0x3F, 0x0D,
      0xB9,  2, 0x23, 0x23,
      0xBF,  6, 0x10, 0x14, 0x14, 0x0B, 0x0B, 0x0B,
      0xF2,  1, 0x00,
      0x73,  5, 0x04, 0xDA, 0x12, 0x54, 0x47,
      0x77,  5, 0x6B, 0x5B, 0xFD, 0xC3, 0xC5,
      0x7A,  2, 0x15, 0x27,
      0x7B,  2, 0x04, 0x57,
      0x7E,  2, 0x01, 0x0E,
      0xBF,  1, 0x36,
      0xE3,  2, 0x40, 0x40,
      0xF0,  1, 0x00,
      0xD0,  1, 0x00,
      0x2A,  4, 0x00, 0x00, 0x01, 0x3F,              // CASET 0..319
      0x2B,  4, 0x00, 0x00, 0x01, 0xDF,              // RASET 0..479
      0x11,  0 | CMD_INIT_DELAY, 120,                // SLPOUT
      0x29,  0 | CMD_INIT_DELAY, 20,                 // DISPON
      0x35,  1, 0x01,                                // TE on
      0xFF, 0xFF
    };
    return (listno == 0) ? list0 : nullptr;
  }

  // Same as Panel_ST77916::setColorDepth, but with an overridable COLMOD value.
  color_depth_t setColorDepth(color_depth_t depth) override
  {
    uint8_t colmod;
    if      (depth == rgb565_2Byte) { colmod = ST77922_COLMOD_RGB565; }
    else if (depth == rgb666_3Byte) { colmod = 0x66; }
    else { return _write_depth; }
    _write_depth = _read_depth = depth;
    _write_bits  = _read_bits  = ((int)depth & color_depth_t::bit_mask);

    startWrite();
    cs_control(false);
    write_cmd(0x3A);
    _bus->writeCommand(colmod, 8);
    _bus->wait();
    cs_control(true);
    endWrite();
    return _write_depth;
  }
};

//----------------------------------------------------------------------------
// Touch controller built into the ST77922 package (I2C 0x55, 16-bit registers).
// Protocol from the vendor / Launcher ST77922_Touch.cpp:
//   0x0010 TOUCH_INFO  bit3 = new coordinates ready (reading points clears it)
//   0x0014 point data, 7 bytes per point: [0]bit7 valid, x = ([0]&0x3F)<<8|[1],
//                                          y = ([2]&0x3F)<<8|[3]
//   0x0009 max touch points, 0x0001 status (low nibble 0 = ready after reset)
// Between reports the last points are held for hold_ms, so a held finger
// doesn't flicker between "touched" and "not touched".
//----------------------------------------------------------------------------
struct Touch_ST77922 : public ITouch
{
  static constexpr uint8_t MAX_POINTS = 10;
  uint16_t hold_ms = 60;

  Touch_ST77922(void)
  {
    _cfg.i2c_addr = 0x55;
    _cfg.freq  = 400000;
    _cfg.x_min = 0; _cfg.x_max = LCD_W - 1;
    _cfg.y_min = 0; _cfg.y_max = LCD_H - 1;
  }

  bool init(void) override
  {
    if (_inited) return true;
    if (isSPI()) return false;

    if (_cfg.pin_rst >= 0) {
      lgfx::pinMode(_cfg.pin_rst, lgfx::pin_mode_t::output);
      lgfx::gpio_hi(_cfg.pin_rst); lgfx::delay(10);
      lgfx::gpio_lo(_cfg.pin_rst); lgfx::delay(100);
      lgfx::gpio_hi(_cfg.pin_rst); lgfx::delay(200);
    }
    if (_cfg.pin_int >= 0) lgfx::pinMode(_cfg.pin_int, lgfx::pin_mode_t::input_pullup);

    if (!lgfx::i2c::init(_cfg.i2c_port, _cfg.pin_sda, _cfg.pin_scl).has_value()) return false;

    uint8_t st = 0xFF;
    for (auto t = lgfx::millis(); (st & 0x0F) && lgfx::millis() - t < 500; ) {
      if (!readReg(0x0001, &st, 1)) return false;   // no ACK -> controller absent
    }
    uint8_t mt = 0;
    _max = (readReg(0x0009, &mt, 1) && mt > 0 && mt <= MAX_POINTS) ? mt : 5;
    _inited = true;
    return true;
  }

  void wakeup(void) override {}
  void sleep(void) override {}

  uint_fast8_t getTouchRaw(touch_point_t* tp, uint_fast8_t count) override
  {
    if (!_inited || !count) return 0;
    uint32_t now = lgfx::millis();

    uint8_t info = 0;
    if (readReg(0x0010, &info, 1) && (info & 0x08)) {
      uint8_t buf[7 * MAX_POINTS];
      if (readReg(0x0014, buf, 7 * _max)) {
        _n = 0;
        for (uint8_t i = 0; i < _max; ++i) {
          const uint8_t* p = &buf[i * 7];
          if (!(p[0] & 0x80)) continue;
          _pts[_n].x    = ((p[0] & 0x3F) << 8) | p[1];
          _pts[_n].y    = ((p[2] & 0x3F) << 8) | p[3];
          _pts[_n].size = p[4];
          _pts[_n].id   = i;
          ++_n;
        }
        _last = now;
      }
    } else if (_n && now - _last > hold_ms) {
      _n = 0;   // no fresh report for a while -> finger lifted
    }

    uint_fast8_t n = std::min<uint_fast8_t>(_n, count);
    for (uint_fast8_t i = 0; i < n; ++i) tp[i] = _pts[i];
    return n;
  }

protected:
  uint8_t _max = 5;
  uint8_t _n = 0;
  uint32_t _last = 0;
  touch_point_t _pts[MAX_POINTS];

  bool readReg(uint16_t reg, uint8_t* buf, size_t len)
  {
    const uint8_t r[2] = { (uint8_t)(reg >> 8), (uint8_t)reg };
    return lgfx::i2c::transactionWriteRead(_cfg.i2c_port, _cfg.i2c_addr, r, 2, buf, len, _cfg.freq).has_value();
  }
};

}} // namespace lgfx::v1

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST77922 _panel;
  lgfx::Bus_SPI       _bus;
  lgfx::Light_PWM     _light;
  lgfx::Touch_ST77922 _touch;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus.config();
      cfg.spi_host    = SPI2_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = LCD_SPI_FREQ;
      cfg.freq_read   = 16000000;
      cfg.pin_sclk    = LCD_QSPI_SCLK;
      cfg.pin_io0     = LCD_QSPI_D0;
      cfg.pin_io1     = LCD_QSPI_D1;
      cfg.pin_io2     = LCD_QSPI_D2;
      cfg.pin_io3     = LCD_QSPI_D3;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs       = LCD_QSPI_CS;
      cfg.pin_rst      = LCD_RST;
      cfg.pin_busy     = -1;
      cfg.panel_width  = LCD_W;
      cfg.panel_height = LCD_H;
      cfg.memory_width = LCD_W;
      cfg.memory_height= LCD_H;
      cfg.offset_x     = 0;
      cfg.offset_y     = 0;
      cfg.offset_rotation = 0;
      cfg.invert       = true;      // vendor init sends INVON (IPS panel)
      cfg.rgb_order    = true;      // true -> MADCTL BGR bit clear (vendor uses MADCTL 0x00)
      cfg.readable     = false;
      cfg.bus_shared   = false;
      _panel.config(cfg);
    }
    {
      auto cfg = _light.config();
      cfg.pin_bl      = LCD_BL;
      cfg.invert      = false;      // high = on
      cfg.freq        = 12000;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    {
      auto cfg = _touch.config();
      cfg.i2c_port = TOUCH_I2C_PORT;
      cfg.i2c_addr = TOUCH_I2C_ADDR;
      cfg.pin_sda  = TOUCH_SDA;
      cfg.pin_scl  = TOUCH_SCL;
      cfg.pin_rst  = TOUCH_RST;
      cfg.pin_int  = TOUCH_INT;
      cfg.freq     = 400000;
      cfg.x_min = 0; cfg.x_max = LCD_W - 1;   // raw coords match panel at rotation 0
      cfg.y_min = 0; cfg.y_max = LCD_H - 1;
      cfg.offset_rotation = 0;
      cfg.bus_shared = false;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }

  lgfx::Touch_ST77922& touchDriver(void) { return _touch; }   // e.g. touchDriver().hold_ms = 80;
};
