#ifndef FLASH_25Q128JV_H
#define FLASH_25Q128JV_H

#include <Arduino.h>
#include <SPI.h>

// Flash memory commands
#define FLASH_CMD_READ_ID          0x9F
#define FLASH_CMD_READ_STATUS      0x05
#define FLASH_CMD_WRITE_ENABLE     0x06
#define FLASH_CMD_WRITE_DISABLE    0x04
#define FLASH_CMD_READ_DATA        0x03
#define FLASH_CMD_PAGE_PROGRAM     0x02
#define FLASH_CMD_SECTOR_ERASE     0x20
#define FLASH_CMD_CHIP_ERASE       0xC7

// Flash memory specifications
#define FLASH_PAGE_SIZE            256
#define FLASH_SECTOR_SIZE          4096
#define FLASH_CHIP_SIZE           (16 * 1024 * 1024)  // 16MB (128Mbit)

// Pin definitions from the image
#define FLASH_CS_PIN              19  // SCS/CMD
#define FLASH_MOSI_PIN           22   // SWP/SD3
#define FLASH_MISO_PIN           21   // SHD/SD2
#define FLASH_SCK_PIN            20   // SCK/CLK

class Flash25Q128JV {
public:
    Flash25Q128JV();
    
    // Initialization
    bool begin();
    
    // Basic operations
    uint32_t readID();
    uint8_t readStatus();
    bool isBusy();
    void waitUntilReady();
    
    // Read/Write operations
    bool read(uint32_t address, uint8_t* buffer, uint32_t length);
    bool write(uint32_t address, const uint8_t* buffer, uint32_t length);
    bool eraseSector(uint32_t address);
    bool eraseChip();
    
    // Test functions
    bool performSelfTest();
    void printFlashInfo();

private:
    SPIClass* _spi;
    bool _initialized;
    
    void writeEnable();
    void writeDisable();
    void select();
    void deselect();
    
    bool writePageInternal(uint32_t address, const uint8_t* buffer, uint32_t length);
};

#endif 