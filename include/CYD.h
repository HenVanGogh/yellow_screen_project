#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>
#include <vector>
#include "PomodoroManager.h"
#include "AudioManager.h"

// Touch Screen Pin Definitions
static constexpr uint8_t PIN_TOUCH_MISO = 39;
static constexpr uint8_t PIN_TOUCH_MOSI = 32;
static constexpr uint8_t PIN_TOUCH_SCLK = 25;
static constexpr uint8_t PIN_TOUCH_CS   = 33;
static constexpr uint8_t PIN_TOUCH_IRQ  = 36;

// LED Pin Definitions
static constexpr uint8_t PIN_LED_BLUE  = 17;
static constexpr uint8_t PIN_LED_RED   = 4;
static constexpr uint8_t PIN_LED_GREEN = 16;

// UI Constants
static constexpr uint16_t SLIDER_HEIGHT = 25;
static constexpr uint16_t SLIDER_WIDTH = 180;
static constexpr uint16_t SLIDER_X = 70;
static constexpr uint16_t HEADER_HEIGHT = 30;
static constexpr uint16_t MARGIN = 10;

// Custom Colors
static constexpr uint16_t UI_BACKGROUND = TFT_BLACK;
static constexpr uint16_t UI_ACCENT = TFT_SKYBLUE;
static constexpr uint16_t UI_SECONDARY = 0x4208;
static constexpr uint16_t UI_TEXT = TFT_WHITE;
static constexpr uint16_t UI_SUBTEXT = 0x8C71;
static constexpr uint16_t SLIDER_BG = 0x2124;
static constexpr uint16_t TEMP_WARN = TFT_ORANGE;
static constexpr uint16_t TEMP_CRITICAL = TFT_RED;

class Slider {
public:
    Slider(int x, int y, const String& label, uint16_t color = UI_ACCENT);
    void draw(TFT_eSPI& tft);
    bool updateValue(int16_t touchX, int16_t touchY);
    uint8_t getValue() const { return m_value; }

private:
    const int m_x;
    const int m_y;
    uint8_t m_value;
    const String m_label;
    const uint16_t m_color;
};

class CYD {
public:
    explicit CYD(AudioManager& audio);
    
    // Core functionality
    void begin();
    void update();
    
    // WiFi management
    bool connectWiFi(const char* ssid, const char* password, uint32_t timeout = 20000);
    void disconnectWiFi();
    bool isWiFiConnected() const;
    
    // LED control
    void setLED(uint8_t r, uint8_t g, uint8_t b);
    void setLEDColor(uint32_t color);
    void turnOffLED();
    
    // Time management
    void syncTime();
    String getCurrentTime() const;
    String getCurrentDate() const;
    
    // UI methods
    void drawUI();

private:
    // Hardware components
    TFT_eSPI m_tft;
    SPIClass m_touchSPI;
    XPT2046_Touchscreen m_touchscreen;
    AudioManager& m_audioManager;
    
    // UI components
    Slider m_brightnessSlider;
    Slider m_colorTempSlider;
    PomodoroManager* m_pomodoroManager;
    
    // State variables
    bool m_wifiConnected;
    bool m_timeInitialized;
    bool m_inPomodoroMode;
    float m_currentTemp;
    
    // Timing variables
    unsigned long m_lastTimeUpdate;
    unsigned long m_lastTempUpdate;
    
    // Temperature history
    std::vector<float> m_temperatureHistory;
    
    // Initialization methods
    void initLEDs();
    void initDisplay();
    void initTouch();
    
    // UI helper methods
    void drawHeader();
    void drawMainMenu();
    void updateTimeDisplay();
    void updateTemperatureDisplay();
    void handleTouch();
    void getTouchScreenCoordinates(int16_t& x, int16_t& y);
    
    // Temperature simulation
    float getDummyTemperature();
    
    // Pomodoro management
    void togglePomodoroMode();
    
    // Light control
    void sendLightingValues(uint8_t brightness, uint8_t colorTemp);
}; 