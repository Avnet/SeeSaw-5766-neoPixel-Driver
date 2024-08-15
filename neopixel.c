/*

 * neopixel.c
 *
 *  Created on: Aug 13, 2024
 *      Author: Brian Willess
 */

#include "neopixel.h"

// This implementation was written for a Renesas RA6M4 MCU and the Adafruit NeoDriver i2c board PN 5766

// Global status used with the I2C callback
static i2c_master_event_t i2c_event;
static bool isActionActive;

void init_seesaw_device(struct pixelData_t* deviceData){

    fsp_err_t status;
    uint8_t i2c_tx_buffer[32] = {0};

    isActionActive = false;

    // Open the i2c interface
    status = R_IIC_MASTER_Open(&g_i2c_master_pixel_ctrl, &g_i2c_master_pixel_cfg);
    if(FSP_SUCCESS != status){
        printf("Master_Open() failed with status %d\n", status);
    }

    status = R_IIC_MASTER_SlaveAddressSet(&g_i2c_master_pixel_ctrl, deviceData->i2c_address, I2C_MASTER_ADDR_MODE_7BIT);
    if(FSP_SUCCESS != status){
        printf("MASTER_SlaveAddressSet() failed with status %d\n", status);
    }

    // Reset the I2C bus
    // write to 0x60 ack data: 0x00 0x7F 0xFF
    i2c_tx_buffer[HEADER_BASE_REG] = 0x00;
    i2c_tx_buffer[HEADER_REGISTER_VALUE] = I2C_RESET >> 8;
    i2c_tx_buffer[2] = I2C_RESET & 0x00ff;
    sendPixelData(deviceData, i2c_tx_buffer, 3);
    vTaskDelay (pdMS_TO_TICKS(1000UL));

    // Set the protocol speed to 400hkz
    //    write to 0x60 ack data: 0x00 0x02
    i2c_tx_buffer[HEADER_BASE_REG] = 0x00;
    i2c_tx_buffer[HEADER_REGISTER_VALUE] = SEESAW_NEOPIXEL_SPEED;
    sendPixelData(deviceData, i2c_tx_buffer, 2);

    // Set the SeeSaw device pin number for NeoPixel output
    // write to 0x60 data: 0x0E 0x01 0x0F
    i2c_tx_buffer[HEADER_BASE_REG] = BASE_REG_ADDRESS;
    i2c_tx_buffer[HEADER_REGISTER_VALUE] = SEESAW_NEOPIXEL_PIN;
    i2c_tx_buffer[2] = SEESAW_NEOPIXEL_DATA_PIN;
    sendPixelData(deviceData, i2c_tx_buffer, 3);

    // Set the buffer length, this is based on the number of Pixels * BYTES_PER_PIXEL (3)
    // write to 0x60 data: 0x0E 0x03 0x00 0x24
    i2c_tx_buffer[HEADER_BASE_REG] = BASE_REG_ADDRESS;
    i2c_tx_buffer[HEADER_REGISTER_VALUE] = SEESAW_NEOPIXEL_BUF_LENGTH;
    i2c_tx_buffer[HEADER_PIXEL_ADDR_HIGH] = (deviceData->num_pixels * BYTES_PER_PIXEL) >> 8;
    i2c_tx_buffer[HEADER_PIXEL_ADDR_LOW] = (deviceData->num_pixels * BYTES_PER_PIXEL) & 0x00ff;
    sendPixelData(deviceData, i2c_tx_buffer, 4);
}

// Sends the pixelData in buf to the i2c device specified in deviceData
void sendPixelData(struct pixelData_t* deviceData, uint8_t* buf, uint8_t len){

    int  timeout_ms = I2C_TRANSACTION_BUSY_DELAY;
    fsp_err_t status = 0;
    i2c_event = 0;

    status = R_IIC_MASTER_SlaveAddressSet(&g_i2c_master_pixel_ctrl, deviceData->i2c_address, I2C_MASTER_ADDR_MODE_7BIT);
    if(FSP_SUCCESS != status){
        printf("MASTER_SlaveAddressSet() failed with status %d\n", status);
    }

    status = R_IIC_MASTER_Write(&g_i2c_master_pixel_ctrl, buf, len, false);
    if(FSP_SUCCESS != status){
        printf("Master_Open() failed with status %d\n", status);
    }

    /* Since there is nothing else to do, block until Callback triggers*/
    while ((I2C_MASTER_EVENT_TX_COMPLETE != i2c_event) && timeout_ms)
    {
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
        timeout_ms--;;
    }
    if (I2C_MASTER_EVENT_ABORTED == i2c_event)
    {
        __BKPT(0);
    }

}


// Callback function that gets called when the i2c Write function completes
void i2c_pixel_callback(i2c_master_callback_args_t *p_args)
{
    i2c_event = p_args->event;
}


// Set a single pixel (pixelNum) the colors defined in the reg, green, blue arguments
void set_pixel(struct pixelData_t* deviceData, uint8_t pixelNum, uint8_t red, uint8_t green, uint8_t blue){

    uint8_t tx_buffer[(BYTES_PER_PIXEL)+4] = {0x00};
    if(deviceData->num_pixels >= pixelNum){

        tx_buffer[0] = BASE_REG_ADDRESS;
        tx_buffer[1] = SEESAW_NEOPIXEL_BUF;
        tx_buffer[2] = 0; // Assume that pixelNum < 256
        tx_buffer[3] = pixelNum*BYTES_PER_PIXEL; // Index into the Pixel Buffer for this pixel
        tx_buffer[4] = red;
        tx_buffer[5] = green;
        tx_buffer[6] = blue;
        sendPixelData(deviceData, tx_buffer, (BYTES_PER_PIXEL)+4);

        // Show the new pixel configuration
        showPixels(deviceData);
    }
}

// Tells the device to send the pixel data over the single wire to the LEDs
void showPixels(struct pixelData_t* deviceData){

    uint8_t tx_buffer[2] = {BASE_REG_ADDRESS, SEESAW_NEOPIXEL_SHOW};
    sendPixelData(deviceData, tx_buffer, 2);
}

// Turns off all pixels defined in the pixelData struct
void clearAllPixels(struct pixelData_t* deviceData){

    // Turn all pixels off
    setSingleColorAllPixels(deviceData, 0x00, 0x00,0x00);

}

// Plays a tracer (led's turn on one after another
// PixelData: The device to play the tracer on
// red, green, blue: Defines the single color to play
// delayMs: The time between turning on the next pixel
// numPixelsOn: How many pixels will participate in the tracer (should be at least 1 less than the number of pixels on the device)
// passes: How many times to play the tracer
void playTracer(struct pixelData_t* deviceData, uint8_t red, uint8_t green, uint8_t blue,
                                              int delayMs, uint8_t numPixelsOn, int passes,  bool clearPixels){

    uint8_t pixelToTurnOff = deviceData->num_pixels - numPixelsOn;
    int numPasses = 0;

    if(clearPixels){

        clearAllPixels(deviceData);
    }

    while(1){

        if(numPasses++ >= passes){
            return;
        }

        for(int i = 0; i < deviceData->num_pixels; i++){

            // Turn the next pixel on tracked by the i variable
            set_pixel(deviceData, i, green, red, blue);
            // Turn off the last pixel in the chain, tracked by the pixelToTurnOff variable
            set_pixel(deviceData, pixelToTurnOff, 0x00, 0x00, 0x00);

            // Roll over the pixel to turn off when it matches the number of pixels
            if(++pixelToTurnOff == deviceData->num_pixels){
                   pixelToTurnOff = 0;
            }

            vTaskDelay (pdMS_TO_TICKS(delayMs));
        }
    }
}

// Plays a single color pulse action in a loop from minValue to maxValue
void playPulser(struct pixelData_t* deviceData, bool red, bool green, bool blue,
                                             uint8_t minValue, uint8_t maxValue,
                                             uint8_t stepSize, int delayMs,
                                             int passes, bool clearPixels){

    int numPasses = 0;
    int16_t i = 0;

    if(clearPixels){

        clearAllPixels(deviceData);
    }

    while(1){

        if(numPasses++ >= passes){
            return;
        }

        // Fade brighter
        for(i = minValue; i <= maxValue; i+=stepSize){

            // Set the incomming colors
            setSingleColorAllPixels(deviceData, (green ? i: 0x00), (red ? i: 0x00) ,(blue ? i: 0x00));
            vTaskDelay (pdMS_TO_TICKS(delayMs));
        }

        // Fade darker
        for(i = maxValue; i >= minValue; i-=stepSize){

            // Set the incomming colors
            setSingleColorAllPixels(deviceData, (green ? i: 0x00), (red ? i: 0x00) ,(blue ? i: 0x00));
            vTaskDelay (pdMS_TO_TICKS(delayMs));
        }

    }
}

// Sets all pixels to the same color
void setSingleColorAllPixels(struct pixelData_t* deviceData, uint8_t red, uint8_t green,uint8_t blue){

    // Define a buffer large enough for all pixels
    uint8_t pixel_buffer[4 + (deviceData->num_pixels * BYTES_PER_PIXEL)];
    int16_t i, j = 0;
    uint8_t pixelsInBuffer = 0;
    int16_t pixelsLeftToWrite = 0;
    uint8_t pixelsWritten = 0;

    // Fill the entire buffer with the incomming pixel data, we'll overwite the first 4 bytes in the next loop
    for(i = 0; i <= deviceData->num_pixels; i++){

        // Start after the SeeSaw header bytes
        j = (i * BYTES_PER_PIXEL) + HEADER_SIZE;

        pixel_buffer[j + RED_PIXEL]= (red);
        pixel_buffer[j + GREEN_PIXEL]= (green);
        pixel_buffer[j + BLUE_PIXEL]= (blue);
    }

    // Now send as many i2c messages necessary to light all the pixels.
    pixelsLeftToWrite = deviceData->num_pixels;

    i = 0;
    while(pixelsLeftToWrite > 0){

        pixelsInBuffer = (pixelsLeftToWrite >= MAX_PIXELS_PER_I2C_PAYLOAD)? MAX_PIXELS_PER_I2C_PAYLOAD: pixelsLeftToWrite;

        pixel_buffer[(i * BYTES_PER_PIXEL * pixelsWritten) + HEADER_BASE_REG] = BASE_REG_ADDRESS;
        pixel_buffer[(i * BYTES_PER_PIXEL * pixelsWritten) + HEADER_REGISTER_VALUE] = SEESAW_NEOPIXEL_BUF;
        pixel_buffer[(i * BYTES_PER_PIXEL * pixelsWritten) + HEADER_PIXEL_ADDR_HIGH] = 0; // Assume that pixelNum < 256
        pixel_buffer[(i * BYTES_PER_PIXEL * pixelsWritten) + HEADER_PIXEL_ADDR_LOW] = i * BYTES_PER_PIXEL * pixelsWritten;  // Address of first pixel

        sendPixelData(deviceData, &pixel_buffer[(i * BYTES_PER_PIXEL * pixelsWritten)], HEADER_SIZE + (BYTES_PER_PIXEL * pixelsInBuffer));
        showPixels(deviceData);

        pixelsLeftToWrite -= pixelsInBuffer;
        pixelsWritten += pixelsInBuffer;

        i++;
    }
}

