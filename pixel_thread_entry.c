#include "pixel_thread.h"
#include <stdio.h>
#include "neopixel.h"

void pixel_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
    int delayTimeMs = 100;

    // Define the specifics for the 12 pixel ring
    struct pixelData_t ringNeoPixels = {.i2c_address = 0x60, .num_pixels = 12};

    init_seesaw_device(&ringNeoPixels);
    clearAllPixels(&ringNeoPixels);

    playPulser(&ringNeoPixels, false, false, true,  0x15,     0x40 ,     0x02,     50,   10,       true);
    playTracer(&ringNeoPixels, 0x00, 0x30, 0x00, 75, 2, 4, false);


    while(1){

        //                         red  green  blue   minValue   maxValue stepSize  delayMs passes
        playPulser(&ringNeoPixels, false, false, true,  0x15,     0x40 ,     0x02,     50,   1,    false );
    }
}

