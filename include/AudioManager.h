#pragma once

#include "Audio.h"
#include "SDManager.h"
#include "driver/dac.h"

class AudioManager {
public:
    // Constants
    static constexpr uint8_t DEFAULT_VOLUME = 1;
    static constexpr uint32_t DAC_BUFFER_SIZE = 64 * 1024;  // 64KB buffer
    static constexpr uint16_t ALARM_COOLDOWN_MS = 500;      // Cooldown between alarm sounds

    AudioManager();
    
    // Core functionality
    void begin();
    void loop();
    
    // Playback control
    void playFile(const char* filename);
    void stop();
    void setVolume(uint8_t volume);
    
    // State queries
    bool isPlaying() const;

private:
    Audio m_audio;
    bool m_isDacEnabled;

    // DAC control methods
    void enableDAC();
    void disableDAC();
}; 