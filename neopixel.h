/*
 * neopixel.h
 *
 *  Created on: Aug 13, 2024
 *      Author: 051520
 */

#ifndef NEOPIXEL_H_
#define NEOPIXEL_H_

#include <stdio.h>
#include "stdbool.h"
#include "pixel_thread.h"
#include "neopixel.h"

#define BYTES_PER_PIXEL 3
#define MAX_PIXELS_PER_I2C_PAYLOAD 7
#define PIXEL_BUFFER_PIXEL_DATA_START_INDEX 4

#define I2C_TRANSACTION_BUSY_DELAY 100
#define SEESAW_NEOPIXEL_DATA_PIN 15

#define I2C_RESET 0x7fff
#define BASE_REG_ADDRESS 0x0e

// Command bytes for the neoPixel device
enum {
  SEESAW_NEOPIXEL_STATUS = 0x00,
  SEESAW_NEOPIXEL_PIN = 0x01,
  SEESAW_NEOPIXEL_SPEED = 0x02,
  SEESAW_NEOPIXEL_BUF_LENGTH = 0x03,
  SEESAW_NEOPIXEL_BUF = 0x04,
  SEESAW_NEOPIXEL_SHOW = 0x05,
};

// Define the i2c header bytes and locations
enum {
    HEADER_BASE_REG = 0,
    HEADER_REGISTER_VALUE,
    HEADER_PIXEL_ADDR_HIGH,
    HEADER_PIXEL_ADDR_LOW,
    HEADER_SIZE
};

// Define enums for the colors
enum {
    RED_PIXEL = 0,
    GREEN_PIXEL,
    BLUE_PIXEL
};

struct pixelData_t {
    uint8_t i2c_address;
    uint8_t num_pixels;
};

void sendPixelData(struct pixelData_t* deviceData, uint8_t* buf, uint8_t len);
void init_seesaw_device(struct pixelData_t* deviceData);
void clearAllPixels(struct pixelData_t* deviceData);
void set_pixel(struct pixelData_t* deviceData, uint8_t pixelNum, uint8_t red, uint8_t green, uint8_t blue);
void showPixels(struct pixelData_t* deviceData);
void setSingleColorAllPixels(struct pixelData_t* deviceData,uint8_t red, uint8_t green,uint8_t blue);

void playTracer(struct pixelData_t* deviceData, uint8_t red, uint8_t green, uint8_t blue, int delayMs, uint8_t numPixelsOn, int passes, bool clearPixels);
void playPulser(struct pixelData_t* deviceData, bool red, bool green, bool blue, uint8_t minValue, uint8_t maxValue, uint8_t stepSize,
                int delayMs, int passes, bool clearPixels);
#endif /* NEOPIXEL_H_ */
