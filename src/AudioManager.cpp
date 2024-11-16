#include "AudioManager.h"

AudioManager::AudioManager() 
    : m_audio(true, I2S_DAC_CHANNEL_LEFT_EN)
    , m_isDacEnabled(false) {
}

void AudioManager::begin() {
    m_audio.setVolume(DEFAULT_VOLUME);
    disableDAC();  // Start with DAC disabled
}

void AudioManager::enableDAC() {
    if (!m_isDacEnabled) {
        dac_output_enable(DAC_CHANNEL_2);  // GPIO26 - right channel
        m_isDacEnabled = true;
        Serial.println(F("DAC enabled"));
    }
}

void AudioManager::disableDAC() {
    if (m_isDacEnabled) {
        dac_output_disable(DAC_CHANNEL_2);
        m_isDacEnabled = false;
        Serial.println(F("DAC disabled"));
    }
}

void AudioManager::playFile(const char* filename) {
    enableDAC();  // Enable DAC before playing
    
    if (m_audio.connecttoSD(filename)) {
        Serial.printf("Playing file: %s\n", filename);
    } else {
        Serial.printf("Failed to play file: %s\n", filename);
        disableDAC();  // Disable DAC if playback failed
    }
}

void AudioManager::stop() {
    m_audio.stopSong();
    disableDAC();
}

void AudioManager::loop() {
    m_audio.loop();
    
    // Auto-disable DAC when audio stops playing
    if (m_isDacEnabled && !m_audio.isRunning()) {
        disableDAC();
    }
}

void AudioManager::setVolume(uint8_t volume) {
    m_audio.setVolume(volume);
}

bool AudioManager::isPlaying() const {
    return const_cast<Audio&>(m_audio).isRunning();
} 