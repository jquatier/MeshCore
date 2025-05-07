#ifdef ST7789

#include "ST7789Display.h"

#ifndef X_OFFSET
#define X_OFFSET 0  // No offset needed for landscape
#endif

#ifndef Y_OFFSET
#define Y_OFFSET 1  // Vertical offset to prevent top row cutoff
#endif

#define SCALE_X  1.875f     // 240 / 128
#define SCALE_Y  2.109375f   // 135 / 64

bool ST7789Display::begin() {
  if(!_isOn) {
    pinMode(PIN_TFT_VDD_CTL, OUTPUT);
    pinMode(PIN_TFT_LEDA_CTL, OUTPUT);
    digitalWrite(PIN_TFT_VDD_CTL, LOW);
    digitalWrite(PIN_TFT_LEDA_CTL, LOW);
    digitalWrite(PIN_TFT_RST, HIGH);

    display.init();
    display.landscapeScreen();
    display.displayOn();
    setCursor(0,0);

    _isOn = true;
  }
  return true;
}

void ST7789Display::turnOn() {
  ST7789Display::begin();
}

void ST7789Display::turnOff() {
  digitalWrite(PIN_TFT_VDD_CTL, HIGH);
  digitalWrite(PIN_TFT_LEDA_CTL, HIGH);
  digitalWrite(PIN_TFT_RST, LOW);
  _isOn = false;
}

void ST7789Display::clear() {
  display.clear();
}

void ST7789Display::startFrame(Color bkg) {
  display.clear();
}

void ST7789Display::setTextSize(int sz) {
  switch(sz) {
    case 1 :
      display.setFont(ArialMT_Plain_16);
      break;
    case 2 :
      display.setFont(ArialMT_Plain_24);
      break;
    default:
      display.setFont(ArialMT_Plain_16);
  }
}

void ST7789Display::setColor(Color c) {
  switch (c) {
    case DisplayDriver::DARK :
      _color = ST77XX_BLACK;
      break;
    case DisplayDriver::LIGHT : 
      _color = ST77XX_WHITE;
      break;
    case DisplayDriver::RED : 
      _color = ST77XX_RED;
      break;
    case DisplayDriver::GREEN : 
      _color = ST77XX_GREEN;
      break;
    case DisplayDriver::BLUE : 
      _color = ST77XX_BLUE;
      break;
    case DisplayDriver::YELLOW : 
      _color = ST77XX_YELLOW;
      break;
    case DisplayDriver::ORANGE : 
      _color = ST77XX_ORANGE;
      break;
    default:
      _color = ST77XX_WHITE;
      break;
  }
  display.setRGB(_color);
}

void ST7789Display::setCursor(int x, int y) {
  _x = x*SCALE_X + X_OFFSET;
  _y = y*SCALE_Y + Y_OFFSET;
}

void ST7789Display::print(const char* str) {
  display.drawString(_x, _y, str);
}

void ST7789Display::fillRect(int x, int y, int w, int h) {
  display.fillRect(x*SCALE_X + X_OFFSET, y*SCALE_Y + Y_OFFSET, w*SCALE_X, h*SCALE_Y);
}

void ST7789Display::drawRect(int x, int y, int w, int h) {
  display.drawRect(x*SCALE_X + X_OFFSET, y*SCALE_Y + Y_OFFSET, w*SCALE_X, h*SCALE_Y);
}

void ST7789Display::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
  // Calculate the base position in display coordinates
  uint16_t startX = x * SCALE_X + X_OFFSET;
  uint16_t startY = y * SCALE_Y + Y_OFFSET;
  
  // Width in bytes for bitmap processing
  uint16_t widthInBytes = (w + 7) / 8;
  
  // Determine the height of each row on the screen
  uint16_t rowHeight = (uint16_t)(SCALE_Y) + 1;
  
  // Process the bitmap row by row
  for (uint16_t by = 0; by < h; by++) {
    uint16_t yPos = startY + (uint16_t)(by * SCALE_Y);
    
    // For each row, we'll track line segments to draw
    int16_t lineStart = -1;
    int16_t lineEnd = -1;
    
    // Scan across the row bit by bit
    for (uint16_t bx = 0; bx <= w; bx++) {
      bool bitSet = false;
      
      // Get the current bit (if we're still within the bitmap width)
      if (bx < w) {
        uint16_t byteOffset = (by * widthInBytes) + (bx / 8);
        uint8_t bitMask = 0x80 >> (bx & 7);
        bitSet = pgm_read_byte(bits + byteOffset) & bitMask;
      }
      
      // If we found a set bit and haven't started a line segment yet
      if (bitSet && lineStart < 0) {
        lineStart = bx;
      }
      // If we found an unset bit or reached the end, and we have an active line segment
      else if ((!bitSet || bx == w) && lineStart >= 0) {
        lineEnd = bx - 1;
        
        // Draw the line segment as a filled rectangle
        uint16_t segStart = startX + (uint16_t)(lineStart * SCALE_X);
        uint16_t segWidth = (uint16_t)((lineEnd - lineStart + 1) * SCALE_X) + 1;
        
        // Draw a filled rectangle for this line segment
        // We use rowHeight to ensure no gaps between rows
        display.fillRect(segStart, yPos, segWidth, rowHeight);
        
        // Reset for the next line segment
        lineStart = -1;
      }
    }
  }
}

uint16_t ST7789Display::getTextWidth(const char* str) {
  return display.getStringWidth(str) / SCALE_X;
}

void ST7789Display::endFrame() {
  display.display();
}

#endif