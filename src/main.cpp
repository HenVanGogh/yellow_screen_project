#include "CYD.h"
#include "AudioManager.h"
#include "SDManager.h"
#include "config.h"

AudioManager audioManager;
CYD cyd(audioManager);

SDManager sdManager;

void setup() {
    Serial.begin(115200);
    delay(100);
    
    // Initialize display and touch first
    cyd.begin();
    delay(100);
    
    //Initialize SD card after display
    if (sdManager.begin()) {
        sdManager.printCardInfo();
        
        // Initialize audio and play test sound
        audioManager.begin();
        delay(500);
        for(int i = 0; i < 3; i++) {
            audioManager.playFile("/beep.mp3");
            unsigned long startTime = millis();
            while (millis() - startTime < 500) {
                audioManager.loop();
            }
        }
    }
    
    // // End SD card SPI to free the bus
    //sdManager.endSPI();
    delay(100);
    
    
    // Connect to WiFi
    if (cyd.connectWiFi(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("Connected to WiFi");
        cyd.setLED(0, 1, 0);
    } else {
        Serial.println("Failed to connect to WiFi");
        cyd.setLED(1, 0, 0);
    }
    
    //audioManager.stop();
    cyd.drawUI();
}

void loop() {
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();
    
    // Update display and handle touch every 20ms
    if (currentTime - lastUpdate >= 20) {
        // Only end the main SPI transaction
        SPI.endTransaction();
        
        cyd.update();
        lastUpdate = currentTime;
    }
    audioManager.loop();
    
    // Small delay to prevent overwhelming the CPU

}