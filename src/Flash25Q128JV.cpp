#include "Flash25Q128JV.h"

Flash25Q128JV::Flash25Q128JV() : _initialized(false) {
    _spi = &SPI; // Use default SPI bus instead of creating new instance
}

bool Flash25Q128JV::begin() {
    // Configure pins
    pinMode(FLASH_CS_PIN, OUTPUT);
    digitalWrite(FLASH_CS_PIN, HIGH);  // Deselect by default
    
    // Initialize SPI with default pins
    _spi->begin();
    
    // Add a small delay after SPI initialization
    delay(10);
    
    // Try reading ID multiple times with timeout
    unsigned long startTime = millis();
    uint32_t id = 0;
    
    while (millis() - startTime < 1000) { // 1 second timeout
        id = readID();
        Serial.printf("Attempting Flash ID read: 0x%06X\n", id);
        
        if (id == 0x17701860) {
            _initialized = true;
            break;
        }
        delay(10);
    }
    
    if (!_initialized) {
        Serial.printf("Flash initialization failed. Last ID read: 0x%06X\n", id);
    }
    
    return _initialized;
}

uint32_t Flash25Q128JV::readID() {
    uint32_t id = 0;
    
    select();
    delayMicroseconds(1);
    
    _spi->transfer(FLASH_CMD_READ_ID);
    id = (_spi->transfer(0) << 16);
    id |= (_spi->transfer(0) << 8);
    id |= _spi->transfer(0);
    
    delayMicroseconds(1);
    deselect();
    
    return id;
}

uint8_t Flash25Q128JV::readStatus() {
    select();
    _spi->transfer(FLASH_CMD_READ_STATUS);
    uint8_t status = _spi->transfer(0);
    deselect();
    return status;
}

bool Flash25Q128JV::isBusy() {
    return (readStatus() & 0x01) != 0;
}

void Flash25Q128JV::waitUntilReady() {
    while(isBusy()) {
        delay(1);
    }
}

void Flash25Q128JV::writeEnable() {
    select();
    _spi->transfer(FLASH_CMD_WRITE_ENABLE);
    deselect();
}

void Flash25Q128JV::writeDisable() {
    select();
    _spi->transfer(FLASH_CMD_WRITE_DISABLE);
    deselect();
}

void Flash25Q128JV::select() {
    _spi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(FLASH_CS_PIN, LOW);
}

void Flash25Q128JV::deselect() {
    digitalWrite(FLASH_CS_PIN, HIGH);
    _spi->endTransaction();
}

bool Flash25Q128JV::read(uint32_t address, uint8_t* buffer, uint32_t length) {
    if (!_initialized || !buffer || address + length > FLASH_CHIP_SIZE) {
        return false;
    }
    
    select();
    _spi->transfer(FLASH_CMD_READ_DATA);
    _spi->transfer((address >> 16) & 0xFF);
    _spi->transfer((address >> 8) & 0xFF);
    _spi->transfer(address & 0xFF);
    
    for (uint32_t i = 0; i < length; i++) {
        buffer[i] = _spi->transfer(0);
    }
    
    deselect();
    return true;
}

bool Flash25Q128JV::write(uint32_t address, const uint8_t* buffer, uint32_t length) {
    if (!_initialized || !buffer || address + length > FLASH_CHIP_SIZE) {
        return false;
    }
    
    uint32_t remainingBytes = length;
    uint32_t currentAddr = address;
    const uint8_t* currentBuffer = buffer;
    
    while (remainingBytes > 0) {
        uint32_t pageOffset = currentAddr % FLASH_PAGE_SIZE;
        uint32_t pageRemaining = FLASH_PAGE_SIZE - pageOffset;
        uint32_t bytesToWrite = min(remainingBytes, pageRemaining);
        
        if (!writePageInternal(currentAddr, currentBuffer, bytesToWrite)) {
            return false;
        }
        
        remainingBytes -= bytesToWrite;
        currentAddr += bytesToWrite;
        currentBuffer += bytesToWrite;
    }
    
    return true;
}

bool Flash25Q128JV::writePageInternal(uint32_t address, const uint8_t* buffer, uint32_t length) {
    writeEnable();
    
    select();
    _spi->transfer(FLASH_CMD_PAGE_PROGRAM);
    _spi->transfer((address >> 16) & 0xFF);
    _spi->transfer((address >> 8) & 0xFF);
    _spi->transfer(address & 0xFF);
    
    for (uint32_t i = 0; i < length; i++) {
        _spi->transfer(buffer[i]);
    }
    
    deselect();
    
    waitUntilReady();
    return true;
}

bool Flash25Q128JV::eraseSector(uint32_t address) {
    if (!_initialized || address >= FLASH_CHIP_SIZE) {
        return false;
    }
    
    writeEnable();
    
    select();
    _spi->transfer(FLASH_CMD_SECTOR_ERASE);
    _spi->transfer((address >> 16) & 0xFF);
    _spi->transfer((address >> 8) & 0xFF);
    _spi->transfer(address & 0xFF);
    deselect();
    
    waitUntilReady();
    return true;
}

bool Flash25Q128JV::eraseChip() {
    if (!_initialized) {
        return false;
    }
    
    writeEnable();
    
    select();
    _spi->transfer(FLASH_CMD_CHIP_ERASE);
    deselect();
    
    waitUntilReady();
    return true;
}

bool Flash25Q128JV::performSelfTest() {
    if (!_initialized) {
        Serial.println("Flash not initialized!");
        return false;
    }
    
    Serial.println("Starting Flash Memory Self-Test...");
    
    // Test 1: Read ID
    uint32_t id = readID();
    Serial.printf("Flash ID: 0x%06X\n", id);
    if (id != 0x17701860) {
        Serial.println("ID verification failed!");
        return false;
    }
    
    // Test 2: Erase-Write-Read test on first sector
    const uint32_t testAddress = 0;
    const uint8_t testPattern[] = {0x55, 0xAA, 0x12, 0x34, 0x78, 0x9F};
    uint8_t readBuffer[sizeof(testPattern)];
    
    Serial.println("Erasing sector 0...");
    if (!eraseSector(testAddress)) {
        Serial.println("Sector erase failed!");
        return false;
    }
    
    Serial.println("Writing test pattern...");
    if (!write(testAddress, testPattern, sizeof(testPattern))) {
        Serial.println("Write failed!");
        return false;
    }
    
    Serial.println("Reading back data...");
    if (!read(testAddress, readBuffer, sizeof(testPattern))) {
        Serial.println("Read failed!");
        return false;
    }
    
    // Verify data
    for (size_t i = 0; i < sizeof(testPattern); i++) {
        if (readBuffer[i] != testPattern[i]) {
            Serial.printf("Data mismatch at offset %d: expected 0x%02X, got 0x%02X\n",
                        i, testPattern[i], readBuffer[i]);
            return false;
        }
    }
    
    Serial.println("Self-test passed successfully!");
    return true;
}

void Flash25Q128JV::printFlashInfo() {
    if (!_initialized) {
        Serial.println("Flash not initialized!");
        return;
    }
    
    uint32_t id = readID();
    uint8_t status = readStatus();
    
    Serial.println("\nFlash Memory Information:");
    Serial.printf("Manufacturer ID: 0x%06X\n", id);
    Serial.printf("Status Register: 0x%02X\n", status);
    Serial.println("Capacity: 16MB (128Mbit)");
    Serial.printf("Page Size: %d bytes\n", FLASH_PAGE_SIZE);
    Serial.printf("Sector Size: %d bytes\n", FLASH_SECTOR_SIZE);
    Serial.println("Write Protected: " + String((status & 0x0C) ? "Yes" : "No"));
} 