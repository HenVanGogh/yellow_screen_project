#include "CYD.h"

// Slider implementation
Slider::Slider(int x, int y, const String& label, uint16_t color)
    : m_x(x)
    , m_y(y)
    , m_value(0)
    , m_label(label)
    , m_color(color) {
}

void Slider::draw(TFT_eSPI& tft) {
    tft.setTextColor(UI_SUBTEXT);
    tft.drawString(m_label, m_x, m_y - 15, 2);
    tft.fillRoundRect(m_x, m_y, SLIDER_WIDTH, SLIDER_HEIGHT, SLIDER_HEIGHT/2, SLIDER_BG);
    
    int fillWidth = (SLIDER_WIDTH * m_value) / 100;
    if(fillWidth > 0) {
        tft.fillRoundRect(m_x, m_y, fillWidth, SLIDER_HEIGHT, SLIDER_HEIGHT/2, m_color);
    }
    
    String valText = String(m_value) + "%";
    tft.setTextColor(UI_TEXT);
    tft.drawString(valText, m_x + SLIDER_WIDTH + 10, m_y + (SLIDER_HEIGHT/2) - 8, 2);
}

bool Slider::updateValue(int16_t touchX, int16_t touchY) {
    if (touchY >= m_y && touchY <= m_y + SLIDER_HEIGHT &&
        touchX >= m_x && touchX <= m_x + SLIDER_WIDTH) {
        m_value = ((touchX - m_x) * 100) / SLIDER_WIDTH;
        m_value = constrain(m_value, 0, 100);
        return true;
    }
    return false;
}

// CYD implementation
CYD::CYD(AudioManager& audio)
    : m_touchSPI(VSPI)
    , m_touchscreen(PIN_TOUCH_CS, PIN_TOUCH_IRQ)
    , m_audioManager(audio)
    , m_brightnessSlider(SLIDER_X, 45, "Brightness", UI_ACCENT)
    , m_colorTempSlider(SLIDER_X, 100, "Color Temperature", UI_SECONDARY)
    , m_pomodoroManager(nullptr)
    , m_wifiConnected(false)
    , m_timeInitialized(false)
    , m_inPomodoroMode(false)
    , m_currentTemp(23.0f)
    , m_lastTimeUpdate(0)
    , m_lastTempUpdate(0) {
}

void CYD::begin() {
    Serial.println(F("CYD initialization starting..."));
    
    // Initialize SPI for display first
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
    Serial.println(F("Display SPI initialized"));
    
    // Initialize display
    initDisplay();
    Serial.println(F("Display initialized"));
    
    // Initialize touch with delay to allow display to stabilize
    delay(100);
    initTouch();
    Serial.println(F("Touch initialized"));
    
    // Initialize LEDs
    initLEDs();
    Serial.println(F("LEDs initialized"));
    
    Serial.println(F("CYD initialization complete"));
}

void CYD::initLEDs() {
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);
    turnOffLED();
}

void CYD::initDisplay() {
    m_tft.init();
    m_tft.setRotation(3);
    m_tft.fillScreen(TFT_BLACK);
}

void CYD::initTouch() {
    Serial.println(F("Initializing touch screen..."));
    
    // Initialize dedicated SPI for touch
    m_touchSPI.begin(PIN_TOUCH_SCLK, PIN_TOUCH_MISO, PIN_TOUCH_MOSI, PIN_TOUCH_CS);
    
    // Set touch SPI to lower speed
    m_touchSPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    m_touchSPI.endTransaction();
    
    // Initialize touchscreen
    if (!m_touchscreen.begin(m_touchSPI)) {
        Serial.println(F("Touch initialization failed!"));
    } else {
        Serial.println(F("Touch initialized successfully"));
    }
    
    m_touchscreen.setRotation(1);
}

void CYD::setLED(uint8_t r, uint8_t g, uint8_t b) {
    digitalWrite(PIN_LED_RED, !r);    // Active LOW
    digitalWrite(PIN_LED_GREEN, !g);
    digitalWrite(PIN_LED_BLUE, !b);
}

void CYD::setLEDColor(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    setLED(r > 127, g > 127, b > 127);
}

void CYD::turnOffLED() {
    setLED(0, 0, 0);
}

bool CYD::connectWiFi(const char* ssid, const char* password, uint32_t timeout) {
    WiFi.begin(ssid, password);
    
    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
        delay(500);
    }
    
    m_wifiConnected = (WiFi.status() == WL_CONNECTED);
    if (m_wifiConnected) {
        syncTime();
    }
    return m_wifiConnected;
}

void CYD::disconnectWiFi() {
    WiFi.disconnect();
    m_wifiConnected = false;
}

bool CYD::isWiFiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

void CYD::syncTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    
    time_t now = time(nullptr);
    while (now < 24 * 3600) {
        delay(100);
        now = time(nullptr);
    }
    
    m_timeInitialized = true;
}

String CYD::getCurrentTime() const {
    if (!m_timeInitialized) return F("Time not synced");
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    return String(buffer);
}

String CYD::getCurrentDate() const {
    if (!m_timeInitialized) return F("Date not synced");
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    return String(buffer);
}

void CYD::getTouchScreenCoordinates(int16_t& x, int16_t& y) {
    x = y = -1;  // Default to no touch
    
    if (!m_touchscreen.touched() || !m_touchscreen.tirqTouched()) {
        return;
    }
    
    TS_Point p = m_touchscreen.getPoint();
    
    // Debug raw values
    Serial.printf("Raw touch - x:%d y:%d z:%d\n", p.x, p.y, p.z);
    
    // Only process valid touches
    if (p.z > 500) {
        x = map(p.x, 3800, 200, 0, 320);
        y = map(p.y, 3800, 200, 0, 240);
        x = constrain(x, 0, 319);
        y = constrain(y, 0, 239);
    }
}

void CYD::drawHeader() {
    m_tft.fillRect(0, 0, m_tft.width(), HEADER_HEIGHT, UI_SECONDARY);
    m_tft.setTextColor(UI_TEXT);
    m_tft.drawString(F("Smart Light Control"), MARGIN, 8, 2);
    
    // Draw connection status
    int statusX = m_tft.width() - 15;
    m_tft.fillCircle(statusX, HEADER_HEIGHT/2, 4, isWiFiConnected() ? TFT_GREEN : TFT_RED);
    
    // Draw time
    if(m_timeInitialized) {
        String timeStr = getCurrentTime();
        m_tft.drawString(timeStr, m_tft.width() - m_tft.textWidth(timeStr, 2) - 30, 8, 2);
    }
}

void CYD::drawUI() {
    m_tft.fillScreen(UI_BACKGROUND);
    
    if (m_inPomodoroMode) {
        if (m_pomodoroManager) {
            m_pomodoroManager->begin();
        }
        return;
    }
    
    drawHeader();
    drawMainMenu();
    m_brightnessSlider.draw(m_tft);
    m_colorTempSlider.draw(m_tft);
    updateTemperatureDisplay();
}

void CYD::update() {
    if (m_inPomodoroMode) {
        if (m_pomodoroManager) {
            m_pomodoroManager->update();
        }
        handleTouch();
        return;
    }
    
    m_audioManager.loop();
    handleTouch();
    updateTimeDisplay();
    
    // Update temperature every 2 seconds
    if (millis() - m_lastTempUpdate > 2000) {
        updateTemperatureDisplay();
        m_lastTempUpdate = millis();
    }
}

void CYD::drawMainMenu() {
    // Draw Pomodoro button
    m_tft.fillRoundRect(10, 190, 100, 40, 5, UI_ACCENT);
    m_tft.setTextColor(TFT_BLACK);
    m_tft.setTextDatum(MC_DATUM);
    m_tft.drawString("Pomodoro", 60, 210, 2);
}

void CYD::updateTimeDisplay() {
    if (!m_timeInitialized || m_inPomodoroMode) return;
    
    if (millis() - m_lastTimeUpdate > 1000) {
        String timeStr = getCurrentTime();
        // Clear the entire header area and redraw it
        drawHeader();
        m_lastTimeUpdate = millis();
    }
}

void CYD::handleTouch() {
    static unsigned long lastTouchTime = 0;
    unsigned long currentTime = millis();
    
    // Debounce delay
    if (currentTime - lastTouchTime < 100) {  // Debounce time
        return;
    }
    
    int16_t screenX, screenY;
    getTouchScreenCoordinates(screenX, screenY);
    
    if (screenX != -1 && screenY != -1) {
        Serial.printf("Valid touch at x:%d y:%d\n", screenX, screenY);
        lastTouchTime = currentTime;
        
        if (m_inPomodoroMode) {
            if (m_pomodoroManager) {
                m_pomodoroManager->handleTouch(screenX, screenY);
                if (!m_pomodoroManager->isActive()) {
                    togglePomodoroMode();
                }
            }
        } else {
            // Check for Pomodoro button
            if (screenY >= 180 && screenY <= 240 && screenX >= 5 && screenX <= 115) {
                Serial.println(F("Pomodoro button pressed"));
                togglePomodoroMode();
            } else if (m_brightnessSlider.updateValue(screenX, screenY) ||
                      m_colorTempSlider.updateValue(screenX, screenY)) {
                m_brightnessSlider.draw(m_tft);
                m_colorTempSlider.draw(m_tft);
                sendLightingValues(m_brightnessSlider.getValue(), m_colorTempSlider.getValue());
            }
        }
        
        delay(10);  // Small delay after processing touch
    }
}

void CYD::togglePomodoroMode() {
    m_inPomodoroMode = !m_inPomodoroMode;
    if (m_inPomodoroMode) {
        if (!m_pomodoroManager) {
            m_pomodoroManager = new PomodoroManager(m_tft, m_audioManager);
        }
        m_pomodoroManager->begin();
    } else {
        drawUI();
    }
}

float CYD::getDummyTemperature() {
    // Simulate temperature between 18-28°C with some variation
    static float lastTemp = 23.0;
    float variation = (random(-10, 11) / 10.0);
    lastTemp += variation;
    lastTemp = constrain(lastTemp, 18.0, 28.0);
    return lastTemp;
}

void CYD::updateTemperatureDisplay() {
    if (m_inPomodoroMode) return;
    
    static float lastDisplayedTemp = 0;
    m_currentTemp = getDummyTemperature();
    
    if (abs(m_currentTemp - lastDisplayedTemp) > 0.1 || lastDisplayedTemp == 0) {
        // Clear the temperature area
        m_tft.fillRect(SLIDER_X - 5, 150, SLIDER_WIDTH + 100, 50, UI_BACKGROUND);
        
        uint16_t tempColor = UI_ACCENT;
        if(m_currentTemp > 26.0) tempColor = TEMP_WARN;
        if(m_currentTemp > 27.0) tempColor = TEMP_CRITICAL;
        
        m_tft.setTextColor(UI_SUBTEXT);
        m_tft.drawString(F("Temperature"), SLIDER_X, 150, 2);
        
        m_tft.setTextColor(tempColor);
        String tempStr = String(m_currentTemp, 1) + "°C";
        m_tft.drawString(tempStr, SLIDER_X, 170, 4);
        
        lastDisplayedTemp = m_currentTemp;
    }
}

void CYD::sendLightingValues(uint8_t brightness, uint8_t colorTemp) {
    // Dummy function to simulate sending values to light controller
    Serial.printf("Sending - Brightness: %d%%, Color Temp: %d%%\n", brightness, colorTemp);
}

// ... (implement remaining methods) ... 