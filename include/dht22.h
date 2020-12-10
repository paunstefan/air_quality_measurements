/**
 * @file dht22.h
 * @author Paun Stefan
 * @brief Read function for the DHT22 temperature/humidity sensor,
 *          using the gpiod library.
 *        Implementation adapted from Tony DiCola's code 
 *          (Copyright (c) 2014 Adafruit Industries).
 * @version 0.1
 * @date 2020-11-21
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef DHT22_H
#define DHT22_H

#include <gpiod.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DHT22_PULSES 41
#define DHT22_MAXCOUNT 36000

/**
 * @brief Return codes for the DHT22 library.
 * 
 */
typedef enum DHT22_RC{
    DHT22_SUCCESS = 0,
    DHT22_TIMEOUT,
    DHT22_CHECKSUM_FAIL,
    DHT22_GPIO_FAIL,
    DHT22_NULL_POINTER
} DHT22_RC;

/**
 * @brief Read function for DHT22 sensor
 * 
 * @param line gpiod line structure, needs to be initialized
 * @param temperature output variable for temperature
 * @param humidity output variable for humidity
 * @return DHT22_RC return code
 */
DHT22_RC dht22_read(struct gpiod_line *line, float *temperature, float *humidity){
    if( NULL == temperature || NULL == humidity ){
        return DHT22_NULL_POINTER;
    }
    *temperature = 0.0f;
    *humidity = 0.0f;

    uint32_t count = 0;

    // Save pulse states in this array
    // 1 pulse is low, signaling bit start
    // 1 pulse is high, short or long (0 or 1)
    uint32_t pulse_counts[DHT22_PULSES*2] = {0};


    gpiod_line_release(line);

    if( gpiod_line_request_output(line, "dht22", 0) == -1 ){
        return DHT22_GPIO_FAIL;
    } 

    // Timing critical section begin

    gpiod_line_set_value(line, 1);
    usleep(500000);

    gpiod_line_set_value(line, 0);
    usleep(20000);

    gpiod_line_release(line);

    if( gpiod_line_request_input(line, "dht22") == -1 ){
        return DHT22_GPIO_FAIL;
    } 

    // Very short delay before reading
    for (volatile int i = 0; i < 50; ++i) { }

    // Wait for sensor pin to pull low
    while( gpiod_line_get_value(line) ){
        if( ++count >= DHT22_MAXCOUNT ){
            return DHT22_TIMEOUT;
        }
    }

    for(int i = 0; i < DHT22_PULSES * 2; i += 2){
        // Count how long pin is low
        while( !gpiod_line_get_value(line) ){
            if( ++pulse_counts[i] >= DHT22_MAXCOUNT ){
                return DHT22_TIMEOUT;
            }
        }
        // Count how long pin is high
        while( gpiod_line_get_value(line) ){
            if( ++pulse_counts[i + 1] >= DHT22_MAXCOUNT ){
                return DHT22_TIMEOUT;
            }
        }
    }
    // Timing critical section end

    // Compute average low pulse length (should be ~50us)
    uint32_t threshold = 0;
    for(int i = 0; i < DHT22_PULSES * 2; i += 2){
        threshold += pulse_counts[i];
    }
    threshold /= DHT22_PULSES - 1 ;

    // Interpret pulses as 0 or 1
    // Short pulse (~28us) = 0
    // Long pulse (~70us) = 1
    uint8_t data[5] = { 0 };
    for(int i = 3; i < DHT22_PULSES * 2; i += 2){
        int index = (i - 3) / 16;
        data[index] <<= 1;

        // Set bit to 1, otherwise it remains 0
        if( pulse_counts[i] >= threshold ){
            data[index] |= 1;
        }
    }

    // Calculate final values
    if( data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)){
        *humidity = (data[0] * 256 + data[1]) / 10.0f;
        *temperature = ((data[2] & 0x7F) * 256 + data[3]) / 10.0f;
        if( data[2] & 0x80 ){
            *temperature *= -1.0f;
        }
    }
    else{
        return DHT22_CHECKSUM_FAIL;
    }

    return DHT22_SUCCESS;
}

#ifdef __cplusplus
}
#endif

#endif