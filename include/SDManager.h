#pragma once

#include "SD.h"
#include "FS.h"
#include "SPI.h"

class SDManager {
public:
    // Constants
    static constexpr uint8_t PIN_SD_SCLK = 18;
    static constexpr uint8_t PIN_SD_MISO = 19;
    static constexpr uint8_t PIN_SD_MOSI = 23;
    static constexpr uint8_t PIN_SD_CS = SS;
    static constexpr uint32_t SD_SPI_FREQUENCY = 4000000;  // 4MHz

    SDManager();
    
    // Core functionality
    bool begin();
    void end() { m_spiSD.end(); }
    
    // File operations
    bool exists(const char* path) const;
    File openFile(const char* path) const;
    
    // Diagnostics
    void printCardInfo() const;

private:
    SPIClass m_spiSD;  // Prefix 'm_' indicates member variable
    
    // Helper methods
    const char* getCardTypeString(uint8_t cardType) const;
}; 