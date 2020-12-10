/**
 * @file sgp30.h
 * @author Paun Stefan
 * @brief C++ library for reading SGP30 sensor
 * @version 0.1
 * @date 2020-12-09
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#pragma once
#include <utility>
#include <optional>
#include <cstdint>

class sgp30{
public:
    /**
     * @brief Construct a new sgp30 object
     * 
     */
    sgp30();

    /**
     * @brief Destroy the sgp30 object
     * 
     */
    ~sgp30();

    /**
     * @brief Initialize sensor
     * 
     * @return true if init succedded, false otherwise
     */
    bool init_air_quality();

    /**
     * @brief Reads air quality data from the sensor
     * 
     * @return std::optional<std::pair<uint16_t, uint16_t>> A pair of CO2 contents(ppm) and TVOC(ppb)
     */
    std::optional<std::pair<uint16_t, uint16_t>> measure_air_quality();

    bool set_baseline(uint16_t co2, uint16_t voc);

    std::optional<std::pair<uint16_t, uint16_t>> get_baseline();

private:
    int fd;

    uint8_t calc_crc8(uint8_t *data, uint8_t len);
};