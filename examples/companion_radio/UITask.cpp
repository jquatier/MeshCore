#include "UITask.h"
#include <Arduino.h>
#include <helpers/TxtDataHelpers.h>
#include "NodePrefs.h"

#define AUTO_OFF_MILLIS     15000   // 15 seconds
#define BOOT_SCREEN_MILLIS   4000   // 4 seconds

#ifdef PIN_STATUS_LED
#define LED_ON_MILLIS     20
#define LED_ON_MSG_MILLIS 200
#define LED_CYCLE_MILLIS  4000
#endif

#ifndef USER_BTN_PRESSED
#define USER_BTN_PRESSED LOW
#endif

// 'meshcore', 128x13px
static const uint8_t meshcore_logo [] PROGMEM = {
    0x3c, 0x01, 0xe3, 0xff, 0xc7, 0xff, 0x8f, 0x03, 0x87, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 
    0x3c, 0x03, 0xe3, 0xff, 0xc7, 0xff, 0x8e, 0x03, 0x8f, 0xfe, 0x3f, 0xfe, 0x1f, 0xff, 0x1f, 0xfe, 
    0x3e, 0x03, 0xc3, 0xff, 0x8f, 0xff, 0x0e, 0x07, 0x8f, 0xfe, 0x7f, 0xfe, 0x1f, 0xff, 0x1f, 0xfc, 
    0x3e, 0x07, 0xc7, 0x80, 0x0e, 0x00, 0x0e, 0x07, 0x9e, 0x00, 0x78, 0x0e, 0x3c, 0x0f, 0x1c, 0x00, 
    0x3e, 0x0f, 0xc7, 0x80, 0x1e, 0x00, 0x0e, 0x07, 0x1e, 0x00, 0x70, 0x0e, 0x38, 0x0f, 0x3c, 0x00, 
    0x7f, 0x0f, 0xc7, 0xfe, 0x1f, 0xfc, 0x1f, 0xff, 0x1c, 0x00, 0x70, 0x0e, 0x38, 0x0e, 0x3f, 0xf8, 
    0x7f, 0x1f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x0e, 0x38, 0x0e, 0x3f, 0xf8, 
    0x7f, 0x3f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x1e, 0x3f, 0xfe, 0x3f, 0xf0, 
    0x77, 0x3b, 0x87, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xfc, 0x38, 0x00, 
    0x77, 0xfb, 0x8f, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xf8, 0x38, 0x00, 
    0x73, 0xf3, 0x8f, 0xff, 0x0f, 0xff, 0x1c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x78, 0x7f, 0xf8, 
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfe, 0x3c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x3c, 0x7f, 0xf8, 
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfc, 0x3c, 0x0e, 0x1f, 0xf8, 0xff, 0xf8, 0x70, 0x3c, 0x7f, 0xf8, 
};

void UITask::begin(DisplayDriver* display, NodePrefs* node_prefs, SensorManager* sensors, const char* build_date, const char* firmware_version, uint32_t pin_code) {
  _display = display;
  _auto_off = millis() + AUTO_OFF_MILLIS;
  clearMsgPreview();
  _node_prefs = node_prefs;
  _sensors = sensors;
  _pin_code = pin_code;
  if (_display != NULL) {
    _display->turnOn();
  }

  // strip off dash and commit hash by changing dash to null terminator
  // e.g: v1.2.3-abcdef -> v1.2.3
  char *version = strdup(firmware_version);
  char *dash = strchr(version, '-');
  if(dash){
    *dash = 0;
  }

  // v1.2.3 (1 Jan 2025)
  sprintf(_version_info, "%s (%s)", version, build_date);
  
}

void UITask::msgRead(int msgcount) {
  _msgcount = msgcount;
  if (msgcount == 0) {
    clearMsgPreview();
  }
}

void UITask::clearMsgPreview() {
  _origin[0] = 0;
  _msg[0] = 0;
  _need_refresh = true;
}

void UITask::newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) {
  _msgcount = msgcount;

  if (path_len == 0xFF) {
    sprintf(_origin, "(F) %s", from_name);
  } else {
    sprintf(_origin, "(%d) %s", (uint32_t) path_len, from_name);
  }
  StrHelper::strncpy(_msg, text, sizeof(_msg));

  if (_display != NULL) {
    if (!_display->isOn()) _display->turnOn();
    _auto_off = millis() + AUTO_OFF_MILLIS;  // extend the auto-off timer
    _need_refresh = true;
  }
}

void renderBatteryIndicator(DisplayDriver* _display, uint16_t batteryMilliVolts) {
  // Convert millivolts to percentage
  const int minMilliVolts = 3000; // Minimum voltage (e.g., 3.0V)
  const int maxMilliVolts = 4200; // Maximum voltage (e.g., 4.2V)
  int batteryPercentage = ((batteryMilliVolts - minMilliVolts) * 100) / (maxMilliVolts - minMilliVolts);
  if (batteryPercentage < 0) batteryPercentage = 0; // Clamp to 0%
  if (batteryPercentage > 100) batteryPercentage = 100; // Clamp to 100%

  // battery icon
  int iconWidth = 18;
  int iconHeight = 9;
  int iconX = _display->width() - iconWidth - 5; // Position the icon near the top-right corner
  int iconY = 1;  // Moved down slightly to vertically center it better
  _display->setColor(DisplayDriver::GREEN);

  // battery outline
  _display->drawRect(iconX, iconY, iconWidth, iconHeight);

  // battery "cap"
  _display->fillRect(iconX + iconWidth, iconY + 2, 2, iconHeight - 4);

  // fill the battery based on the percentage
  int fillWidth = (batteryPercentage * (iconWidth - 2)) / 100;
  _display->fillRect(iconX + 1, iconY + 1, fillWidth, iconHeight - 2);
}

void UITask::renderCurrScreen() {
  if (_display == NULL) return;  // assert() ??

  char tmp[80];
  if (_origin[0] && _msg[0]) { // message preview
    // render message preview
    _display->setCursor(0, 0);
    _display->setTextSize(1);
    _display->setColor(DisplayDriver::GREEN);
    _display->print(_node_prefs->node_name);

    _display->setCursor(0, 12);
    _display->setColor(DisplayDriver::YELLOW);
    _display->print(_origin);
    _display->setCursor(0, 24);
    _display->setColor(DisplayDriver::LIGHT);
    _display->print(_msg);

    _display->setCursor(_display->width() - 28, 9);
    _display->setTextSize(2);
    _display->setColor(DisplayDriver::ORANGE);
    sprintf(tmp, "%d", _msgcount);
    _display->print(tmp);
    _display->setColor(DisplayDriver::YELLOW); // last color will be kept on T114
  } else if (millis() < BOOT_SCREEN_MILLIS) { // boot screen
    // meshcore logo
    _display->setColor(DisplayDriver::BLUE);
    int logoWidth = 128;
    _display->drawXbm((_display->width() - logoWidth) / 2, 3, meshcore_logo, logoWidth, 13);

    // version info
    _display->setColor(DisplayDriver::LIGHT);
    _display->setTextSize(1);
    uint16_t textWidth = _display->getTextWidth(_version_info);
    _display->setCursor((_display->width() - textWidth) / 2, 22);
    _display->print(_version_info);
  } else {  // home screen with pages
    // Render shared header for all pages
    renderSharedHeader();
    
    // Render current page content
    switch (_current_page) {
      case 0:
        renderMainPage();
        break;
      case 1:
        if (_sensors != NULL && _sensors->has_gps) {
          renderGPSPage();
        } else {
          // If GPS not available, wrap back to page 0
          _current_page = 0;
          renderMainPage();
        }
        break;
      default:
        _current_page = 0;
        renderMainPage();
        break;
    }

    // monochrome displays like T114 need to set color for the next frame
    if(_connected) {
      _display->setColor(DisplayDriver::LIGHT);
    } else {
      _display->setColor(DisplayDriver::GREEN);
    }
  }
  _need_refresh = false;
}

void UITask::renderSharedHeader() {
  if (_display == NULL) return;
  
  // Node name at top left
  _display->setCursor(0, 0);
  _display->setTextSize(1);
  _display->setColor(DisplayDriver::GREEN);
  _display->print(_node_prefs->node_name);
  
  
  // Battery voltage
  renderBatteryIndicator(_display, _board->getBattMilliVolts());
  
  // Page indicator using dots - only show if more than 1 page
  if (_total_pages > 1) {
    int dotSize = 3;  // Size of each dot
    int dotSpacing = 6;  // Space between dots
    int totalWidth = (_total_pages * dotSize) + ((_total_pages - 1) * dotSpacing);
    int startX = _display->width() - totalWidth - 5;  // Position at lower right with 5px margin
    int y = _display->height() - 5;  // Position near bottom with 5px margin
    
    for (int i = 0; i < _total_pages; i++) {
      int x = startX + (i * (dotSize + dotSpacing));
      if (i == _current_page) {
        _display->setColor(DisplayDriver::GREEN);
        _display->fillRect(x, y, dotSize, dotSize);
      } else {
        _display->setColor(DisplayDriver::LIGHT);
        _display->drawRect(x, y, dotSize, dotSize);
      }
    }
  }
}

void UITask::renderMainPage() {
  if (_display == NULL) return;
  
  char tmp[80];
  
  // Freq / SF
  _display->setCursor(0, 16);  // Changed from 20 to 16 to align with LAT
  _display->setColor(DisplayDriver::YELLOW);
  sprintf(tmp, "FREQ: %06.3f SF%d", _node_prefs->freq, _node_prefs->sf);
  _display->print(tmp);
  
  // BW / CR
  _display->setCursor(0, 26);  // Changed from 30 to 26 to maintain 10px spacing
  sprintf(tmp, "BW: %03.2f CR: %d", _node_prefs->bw, _node_prefs->cr);
  _display->print(tmp);
  
  // BT pin if disconnected
  if (!_connected && _pin_code != 0) {
    _display->setColor(DisplayDriver::RED);
    _display->setTextSize(2);
    _display->setCursor(0, 43);
    sprintf(tmp, "Pin:%d", _pin_code);
    _display->print(tmp);
    _display->setColor(DisplayDriver::GREEN);
    _display->setTextSize(1);  // Reset text size
  }
}

void UITask::renderGPSPage() {
  if (_display == NULL || _sensors == NULL) return;
  
  char tmp[80];
  
  // GPS Status (latitude/longitude)
  _display->setCursor(0, 16);
  _display->setTextSize(1);
  _display->setColor(DisplayDriver::YELLOW);
  
  if (_sensors->node_lat != 0.0 || _sensors->node_lon != 0.0) {
    // Show latitude
    sprintf(tmp, "LAT: %.6f", _sensors->node_lat);
    _display->print(tmp);
    
    // Show longitude
    _display->setCursor(0, 26);
    sprintf(tmp, "LON: %.6f", _sensors->node_lon);
    _display->print(tmp);
    
    // Show altitude
    _display->setCursor(0, 36);
    _display->setColor(DisplayDriver::LIGHT);
    sprintf(tmp, "ALT: %.1fm", _sensors->altitude_meters);
    _display->print(tmp);
    
    // Show satellites
    _display->setCursor(0, 46);
    sprintf(tmp, "SATS: %d", _sensors->num_satellites);
    _display->print(tmp);
  } else {
    // No GPS fix
    _display->print("GPS: No Fix");
    
    _display->setCursor(0, 26);
    _display->setColor(DisplayDriver::RED);
    _display->print("Searching...");
    
    _display->setCursor(0, 36);
    _display->setColor(DisplayDriver::LIGHT);
    sprintf(tmp, "SATS: %d", _sensors->num_satellites);
    _display->print(tmp);
  }
}

void UITask::userLedHandler() {
#ifdef PIN_STATUS_LED
  static int state = 0;
  static int next_change = 0;
  static int last_increment = 0;

  int cur_time = millis();
  if (cur_time > next_change) {
    if (state == 0) {
      state = 1;
      if (_msgcount > 0) {
        last_increment = LED_ON_MSG_MILLIS;
      } else {
        last_increment = LED_ON_MILLIS;
      }
      next_change = cur_time + last_increment;
    } else {
      state = 0;
      next_change = cur_time + LED_CYCLE_MILLIS - last_increment;
    }
    digitalWrite(PIN_STATUS_LED, state);
  }
#endif
}

void UITask::buttonHandler() {
#ifdef PIN_USER_BTN
  static int prev_btn_state = !USER_BTN_PRESSED;
  static unsigned long btn_state_change_time = 0;
  static unsigned long next_read = 0;
  int cur_time = millis();
  if (cur_time >= next_read) {
    int btn_state = digitalRead(PIN_USER_BTN);
    if (btn_state != prev_btn_state) {
      if (btn_state == USER_BTN_PRESSED) {  // pressed?
        if (_display != NULL) {
          if (_display->isOn()) {
            // If display is on, check if we have a message preview
            if (_origin[0] != 0 || _msg[0] != 0) {
              clearMsgPreview();  // Clear message preview
            } else {
              // Switch to next page
              _current_page = (_current_page + 1) % _total_pages;
              _need_refresh = true;
            }
          } else {
            _display->turnOn();
            _need_refresh = true;
          }
          _auto_off = cur_time + AUTO_OFF_MILLIS;   // extend auto-off timer
        }
      } else { // unpressed ? check pressed time ...
        if ((cur_time - btn_state_change_time) > 5000) {
        #ifdef PIN_STATUS_LED
          digitalWrite(PIN_STATUS_LED, LOW);
          delay(10);
        #endif
          _board->powerOff();
        }
      }
      btn_state_change_time = millis();
      prev_btn_state = btn_state;
    }
    next_read = millis() + 100;  // 10 reads per second
  }
#endif
}

void UITask::loop() {
  buttonHandler();
  userLedHandler();
  
  // Update total pages based on GPS availability
  _total_pages = (_sensors != NULL && _sensors->has_gps) ? 2 : 1;

  if (_display != NULL && _display->isOn()) {
    static bool _firstBoot = true;
    if(_firstBoot && millis() >= BOOT_SCREEN_MILLIS) {
      _need_refresh = true;
      _firstBoot = false;
    }
    if (millis() >= _next_refresh && _need_refresh) {
      _display->startFrame();
      renderCurrScreen();
      _display->endFrame();

      _next_refresh = millis() + 1000;   // refresh every second
    }
    if (millis() > _auto_off) {
      _display->turnOff();
    }
  }
}
