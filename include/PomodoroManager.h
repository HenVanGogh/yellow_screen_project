#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "AudioManager.h"

class PomodoroManager {
public:
    // Constants
    static constexpr uint16_t BUTTON_HEIGHT = 40;
    static constexpr uint16_t BUTTON_WIDTH = 60;
    static constexpr uint16_t TIME_Y = 80;
    static constexpr uint16_t DEFAULT_WORK_MINUTES = 50;
    static constexpr uint16_t DEFAULT_BREAK_MINUTES = 10;
    static constexpr uint16_t ALARM_INTERVAL_MS = 500;

    PomodoroManager(TFT_eSPI& tft, AudioManager& audio);
    
    // Core functionality
    void begin();
    void update();
    void handleTouch(int16_t x, int16_t y);
    
    // State queries
    bool isActive() const { return m_isActive; }

private:
    // References to external components
    TFT_eSPI& m_tft;
    AudioManager& m_audio;
    
    // Timer settings
    uint16_t m_workMinutes;
    uint16_t m_breakMinutes;
    uint32_t m_currentSeconds;
    
    // State flags
    bool m_isWorkTime;
    bool m_isRunning;
    bool m_isActive;
    bool m_isAlarmSounding;
    
    // Timing variables
    unsigned long m_lastUpdate;
    unsigned long m_lastAlarmTime;

    // UI helper methods
    void drawTimeAdjustButtons(int y, const char* label, int minutes);
    void drawButton(int x, int y, int w, int h, const char* label, uint16_t color);
    void drawInterface();
    void drawTimer(bool fullRedraw);
    String formatTime(int seconds) const;
}; 