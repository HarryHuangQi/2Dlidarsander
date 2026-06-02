#include "I2Cdev.h"
#include "driver/i2c.h"
#include <string.h>

// Default I2C port used by MPU6050 component. Adjust if needed.
#define I2C_PORT_NUM I2C_NUM_0
#define I2C_TIMEOUT_TICKS pdMS_TO_TICKS(1000)

I2Cdev::I2Cdev() {}

static esp_err_t write_to_register(uint8_t devAddr, uint8_t regAddr, const uint8_t* data, size_t len, uint32_t timeout_ms) {
    // write regAddr then data
    uint8_t buf[1 + 256];
    if (len > 255) return ESP_ERR_INVALID_ARG;
    buf[0] = regAddr;
    if (len) memcpy(&buf[1], data, len);
    return i2c_master_write_to_device(I2C_PORT_NUM, devAddr, buf, 1 + len, timeout_ms / portTICK_PERIOD_MS);
}

static esp_err_t read_from_register(uint8_t devAddr, uint8_t regAddr, uint8_t* out, size_t len, uint32_t timeout_ms) {
    esp_err_t ret = i2c_master_write_to_device(I2C_PORT_NUM, devAddr, &regAddr, 1, timeout_ms / portTICK_PERIOD_MS);
    if (ret != ESP_OK) return ret;
    return i2c_master_read_from_device(I2C_PORT_NUM, devAddr, out, len, timeout_ms / portTICK_PERIOD_MS);
}

int8_t I2Cdev::readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *data, uint16_t timeout) {
    if (!data) return 0;
    esp_err_t ret = read_from_register(devAddr, regAddr, data, length, timeout);
    return (ret == ESP_OK) ? length : 0;
}

int8_t I2Cdev::readWords(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint16_t *data, uint16_t timeout) {
    if (!data) return 0;
    uint8_t buf[256];
    esp_err_t ret = read_from_register(devAddr, regAddr, buf, length * 2, timeout);
    if (ret != ESP_OK) return 0;
    for (int i = 0; i < length; i++) {
        data[i] = ((uint16_t)buf[2*i] << 8) | buf[2*i+1];
    }
    return length;
}

int8_t I2Cdev::readByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint16_t timeout) {
    return readBytes(devAddr, regAddr, 1, data, timeout);
}

int8_t I2Cdev::readWord(uint8_t devAddr, uint8_t regAddr, uint16_t *data, uint16_t timeout) {
    return readWords(devAddr, regAddr, 1, data, timeout);
}

int8_t I2Cdev::readBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t *data, uint16_t timeout) {
    uint8_t b;
    int8_t cnt = readByte(devAddr, regAddr, &b, timeout);
    *data = (b >> bitNum) & 0x01;
    return cnt;
}

int8_t I2Cdev::readBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t *data, uint16_t timeout) {
    uint16_t w;
    int8_t cnt = readWord(devAddr, regAddr, &w, timeout);
    *data = (w >> bitNum) & 0x1;
    return cnt;
}

int8_t I2Cdev::readBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t *data, uint16_t timeout) {
    uint8_t b;
    int8_t cnt = readByte(devAddr, regAddr, &b, timeout);
    if (cnt) {
        uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        b &= mask;
        b >>= (bitStart - length + 1);
        *data = b;
    }
    return cnt;
}

int8_t I2Cdev::readBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t *data, uint16_t timeout) {
    uint16_t w;
    int8_t cnt = readWord(devAddr, regAddr, &w, timeout);
    if (cnt) {
        uint16_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        w &= mask;
        w >>= (bitStart - length + 1);
        *data = w;
    }
    return cnt;
}

bool I2Cdev::writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *data) {
    return write_to_register(devAddr, regAddr, data, length, 1000) == ESP_OK;
}

bool I2Cdev::writeWords(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint16_t *data) {
    uint8_t buf[512];
    for (int i = 0; i < length; i++) {
        buf[2*i] = (data[i] >> 8) & 0xFF;
        buf[2*i+1] = data[i] & 0xFF;
    }
    return write_to_register(devAddr, regAddr, buf, length * 2, 1000) == ESP_OK;
}

bool I2Cdev::writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data) {
    return writeBytes(devAddr, regAddr, 1, &data);
}

bool I2Cdev::writeWord(uint8_t devAddr, uint8_t regAddr, uint16_t data) {
    return writeWords(devAddr, regAddr, 1, &data);
}

bool I2Cdev::writeBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t data) {
    uint8_t b;
    if (!readByte(devAddr, regAddr, &b)) return false;
    b = (data != 0) ? (b | (1 << bitNum)) : (b & ~(1 << bitNum));
    return writeByte(devAddr, regAddr, b);
}

bool I2Cdev::writeBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t data) {
    uint16_t w;
    if (!readWord(devAddr, regAddr, &w)) return false;
    w = (data != 0) ? (w | (1 << bitNum)) : (w & ~(1 << bitNum));
    return writeWord(devAddr, regAddr, w);
}

bool I2Cdev::writeBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data) {
    uint8_t b;
    if (!readByte(devAddr, regAddr, &b)) return false;
    uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
    data <<= (bitStart - length + 1);
    data &= mask;
    b &= ~mask;
    b |= data;
    return writeByte(devAddr, regAddr, b);
}

bool I2Cdev::writeBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t data) {
    uint16_t w;
    if (!readWord(devAddr, regAddr, &w)) return false;
    uint16_t mask = ((1 << length) - 1) << (bitStart - length + 1);
    data <<= (bitStart - length + 1);
    data &= mask;
    w &= ~mask;
    w |= data;
    return writeWord(devAddr, regAddr, w);
}
