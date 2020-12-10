/**
 * @file temperature_sensor.h
 * @author Paun Stefan
 * @brief C++ wrapper for the dht22 library
 * @version 0.1
 * @date 2020-11-22
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#pragma once
#include <utility>
#include <optional>

/**
 * @brief Temperature sensor class wrapper for low level library
 * 
 */
class temperature_sensor{
private:
    struct gpiod_line *line;

public:
    /**
     * @brief Construct a new temperature sensor object
     * 
     * @param p Pin number
     */
    temperature_sensor(struct gpiod_line *l);

    /**
     * @brief Read from sensor
     * 
     * @return optional<pair<float, float>> - A pair of temperature and humidity,
     *          if read fails returns nullopt/false.
     */
    std::optional<std::pair<float, float>> read_data();

};