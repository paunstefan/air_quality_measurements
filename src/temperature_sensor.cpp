/**
 * @file temperature_sensor.cpp
 * @author Paun Stefan
 * @brief C++ wrapper for the dht22 library
 * @version 0.1
 * @date 2020-11-22
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <utility>
#include <optional>
#include "temperature_sensor.h"
#include "dht22.h"
#ifdef DEBUG
#include <cstdio>
#endif

using namespace std;

/**
 * @brief Construct a new temperature sensor object
 * 
 * @param p Pin number
 */
temperature_sensor::temperature_sensor(struct gpiod_line *l): line{l}{

}

/**
 * @brief Read from sensor
 * 
 * @return optional<pair<float, float>> - A pair of temperature and humidity,
 *          if read fails returns nullopt/false.
 */
std::optional<std::pair<float, float>> temperature_sensor::read_data(){
    std::pair<float, float> temp_humidity;
    DHT22_RC rc = dht22_read(line, &(temp_humidity.first), &(temp_humidity.second));

    if( DHT22_SUCCESS != rc ){
#ifdef DEBUG
        printf("Error: %d\n", rc);
#endif
        return {};
    }

    return temp_humidity;
}