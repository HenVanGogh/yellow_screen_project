#include "SDManager.h"

SDManager::SDManager() : m_spiSD(HSPI) {
}

bool SDManager::begin() {
    m_spiSD.begin(PIN_SD_SCLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    
    if (!SD.begin(PIN_SD_CS, m_spiSD, SD_SPI_FREQUENCY)) {
        Serial.println(F("SD Card Mount Failed"));
        return false;
    }
    return true;
}

void SDManager::printCardInfo() const {
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println(F("No SD card attached"));
        return;
    }
    
    Serial.print(F("Card Type: "));
    Serial.println(getCardTypeString(cardType));

    uint64_t cardSizeMB = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSizeMB);
}

const char* SDManager::getCardTypeString(uint8_t cardType) const {
    switch(cardType) {
        case CARD_MMC:  return "MMC";
        case CARD_SD:   return "SDSC";
        case CARD_SDHC: return "SDHC";
        default:        return "UNKNOWN";
    }
}

bool SDManager::exists(const char* path) const {
    return SD.exists(path);
}

File SDManager::openFile(const char* path) const {
    return SD.open(path);
} 