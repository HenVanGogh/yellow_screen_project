#include "PomodoroManager.h"

PomodoroManager::PomodoroManager(TFT_eSPI& tft, AudioManager& audio) 
    : m_tft(tft)
    , m_audio(audio)
    , m_workMinutes(DEFAULT_WORK_MINUTES)
    , m_breakMinutes(DEFAULT_BREAK_MINUTES)
    , m_currentSeconds(0)
    , m_isWorkTime(true)
    , m_isRunning(false)
    , m_isActive(true)
    , m_isAlarmSounding(false)
    , m_lastUpdate(0)
    , m_lastAlarmTime(0) {
}

void PomodoroManager::drawButton(int x, int y, int w, int h, const char* label, uint16_t color) {
    m_tft.fillRoundRect(x, y, w, h, 5, color);
    m_tft.setTextColor(TFT_BLACK);
    m_tft.setTextDatum(MC_DATUM);
    m_tft.drawString(label, x + w/2, y + h/2, 2);
}

void PomodoroManager::begin() {
    m_tft.fillScreen(TFT_BLACK);
    drawInterface();
}

void PomodoroManager::drawInterface() {
    m_tft.fillScreen(TFT_BLACK);
    
    if (m_isRunning) {
        drawTimer(true);  // Full redraw
        return;
    }
    
    // Draw title
    m_tft.setTextColor(TFT_WHITE);
    m_tft.setTextDatum(TC_DATUM);
    m_tft.drawString("Pomodoro Timer", 160, 20, 4);
    
    // Draw work time settings
    drawTimeAdjustButtons(70, "Work Time", m_workMinutes);
    
    // Draw break time settings
    drawTimeAdjustButtons(150, "Break Time", m_breakMinutes);
    
    // Draw start button
    drawButton(110, 190, 100, 40, "START", TFT_GREEN);
    
    // Draw exit button
    drawButton(5, 5, 50, 30, "X", TFT_RED);
}

void PomodoroManager::drawTimeAdjustButtons(int y, const char* label, int minutes) {
    m_tft.setTextColor(TFT_WHITE);
    m_tft.setTextDatum(TC_DATUM);
    m_tft.drawString(label, 160, y - 20, 2);
    
    drawButton(60, y, BUTTON_WIDTH, BUTTON_HEIGHT, "-", TFT_BLUE);
    
    m_tft.setTextColor(TFT_WHITE);
    m_tft.setTextDatum(MC_DATUM);
    m_tft.drawString(String(minutes), 160, y + BUTTON_HEIGHT/2, 4);
    
    drawButton(200, y, BUTTON_WIDTH, BUTTON_HEIGHT, "+", TFT_BLUE);
}

String PomodoroManager::formatTime(int seconds) const {
    int minutes = seconds / 60;
    int secs = seconds % 60;
    char buffer[6];
    sprintf(buffer, "%02d:%02d", minutes, secs);
    return String(buffer);
}

void PomodoroManager::drawTimer(bool fullRedraw = false) {
    uint16_t sessionColor = m_isWorkTime ? TFT_GREEN : TFT_ORANGE;
    
    if (fullRedraw) {
        m_tft.fillScreen(TFT_BLACK);
        
        // Draw session type
        m_tft.setTextColor(sessionColor);
        m_tft.setTextDatum(TC_DATUM);
        m_tft.drawString(m_isWorkTime ? "WORK TIME" : "BREAK TIME", 160, 40, 4);
        
        // Draw stop button
        drawButton(110, 190, 100, 40, "STOP", TFT_RED);
    }
    
    // Clear only the time display area
    m_tft.fillRect(40, 80, 240, 70, TFT_BLACK);
    
    // Draw time remaining
    m_tft.setTextColor(TFT_WHITE);
    m_tft.setTextFont(7);
    m_tft.setTextDatum(TC_DATUM);
    m_tft.drawString(formatTime(m_currentSeconds), 160, 100);
    m_tft.setTextFont(2);
    
    // Update progress bar
    int totalSeconds = m_isWorkTime ? m_workMinutes * 60 : m_breakMinutes * 60;
    int barWidth = 240;
    int barHeight = 10;
    int progress = map(m_currentSeconds, 0, totalSeconds, 0, barWidth);
    
    // Draw background bar
    m_tft.fillRoundRect(40, 160, barWidth, barHeight, barHeight/2, TFT_DARKGREY);
    // Draw progress
    if (progress > 0) {
        m_tft.fillRoundRect(40, 160, progress, barHeight, barHeight/2, sessionColor);
    }
}

void PomodoroManager::update() {
    if (!m_isRunning) return;
    
    unsigned long currentTime = millis();
    if (currentTime - m_lastUpdate >= 1000) {
        if (m_currentSeconds > 0) {
            m_currentSeconds--;
            m_lastUpdate = currentTime;
            drawTimer(false);  // Partial redraw
        }
        
        if (m_currentSeconds <= 0 && !m_isAlarmSounding) {
            m_isAlarmSounding = true;
            m_audio.playFile("/beep.mp3");
            m_lastAlarmTime = currentTime;  // Store the time when the alarm was triggered
        }
    }
    
    // Keep playing alarm if it's sounding
    if (m_isAlarmSounding) {
        // Check if enough time has passed since the last alarm
        if (currentTime - m_lastAlarmTime >= 500) {  // 500 ms cooldown
            m_audio.playFile("/beep.mp3");
            m_lastAlarmTime = currentTime;  // Update the last alarm time
        }
        
    }
}

void PomodoroManager::handleTouch(int16_t x, int16_t y) {
    if (m_isAlarmSounding) {
        // Stop alarm on any touch
        m_audio.stop();
        m_isAlarmSounding = false;
        m_isWorkTime = !m_isWorkTime;
        m_currentSeconds = (m_isWorkTime ? m_workMinutes : m_breakMinutes) * 60;
        drawTimer(true);
        return;
    }
    
    if (m_isRunning) {
        // Stop button
        if (y >= 190 && y <= 230 && x >= 110 && x <= 210) {
            m_isRunning = false;
            drawInterface();
        }
        return;
    }
    
    // Exit button
    if (x < 55 && y < 35) {
        m_isActive = false;
        return;
    }
    
    // Work time adjustment
    if (y >= 70 && y <= 110) {
        if (x >= 60 && x <= 120) m_workMinutes = max(5, m_workMinutes - 5);
        if (x >= 200 && x <= 260) m_workMinutes = min(120, m_workMinutes + 5);
        drawInterface();
        return;
    }
    
    // Break time adjustment
    if (y >= 150 && y <= 190) {
        if (x >= 60 && x <= 120) m_breakMinutes = max(5, m_breakMinutes - 5);
        if (x >= 200 && x <= 260) m_breakMinutes = min(60, m_breakMinutes + 5);
        drawInterface();
        return;
    }
    
    // Start button
    if (y >= 190 && y <= 230 && x >= 110 && x <= 210) {
        m_isRunning = true;
        m_isWorkTime = true;
        m_currentSeconds = m_workMinutes * 60;
        m_lastUpdate = millis();
        m_isAlarmSounding = false;
        drawTimer(true);
    }
}