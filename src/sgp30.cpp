/**
 * @file sgp30.c
 * @author Paun Stefan
 * @brief C++ library for reading SGP30 sensor
 * @version 0.1
 * @date 2020-12-09
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <cstddef>
#include <iostream>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <utility>
#include <optional>
#include "sgp30.h"

extern "C" {
    #include <linux/i2c-dev.h>
    #include <i2c/smbus.h>
}

using namespace std::chrono_literals;

#define SGP30_ADDR 0x58

#define CRC8_poly 0x31
#define CRC8_init 0xff

/**
 * @brief Construct a new sgp30 object
 * 
 */
sgp30::sgp30(){
    fd = open("/dev/i2c-1", O_RDWR);
    if( fd < 0 ){
        std::cout << "Error opening i2c dev\n";
    }

    if( ioctl(fd, I2C_SLAVE, SGP30_ADDR) < 0 ){
        std::cout << "Error ioctl\n";
        close(fd);
        exit(1);
    }
}

/**
 * @brief Destroy the sgp30 object
 * 
 */
sgp30::~sgp30(){
    close(fd);
}

/**
 * @brief Initialize sensor
 * 
 * @return true if init succedded, false otherwise
 */
bool sgp30::init_air_quality(){
    uint8_t command[2] = { 0x20, 0x03};

    if( write(fd, command, 2) != 2 ){
        return false;
    }

    std::this_thread::sleep_for(10ms);

    return true;
}

/**
 * @brief Reads air quality data from the sensor
 * 
 * @return std::optional<std::pair<uint16_t, uint16_t>> A pair of CO2 contents(ppm) and TVOC(ppb)
 */
std::optional<std::pair<uint16_t, uint16_t>> sgp30::measure_air_quality(){
    uint8_t command[2] = { 0x20, 0x08};
    uint8_t response[6] = { 0 };

    if( write(fd, command, 2) != 2 ){
        return {};
    }

    std::this_thread::sleep_for(12ms);

    if( read(fd, response, 6) != 6 ){
        return {};
    }

    if( (calc_crc8(response, 2) != response[2]) or (calc_crc8(response + 3, 2) != response[5]) ){
        return {};
    }

    std::pair<uint16_t, uint16_t> ret;
    ret.first = ((uint16_t)response[0] << 8) | (uint16_t)response[1];
    ret.second = ((uint16_t)response[3] << 8) | (uint16_t)response[4];

    return ret;
}

/**
 * @brief Set the sensor baselines
 * 
 * @param co2 CO2 baseline
 * @param voc TVOC baseline
 * @return true if read succedded, false otherwise
 */
bool sgp30::set_baseline(uint16_t co2, uint16_t voc){
    uint8_t baseline[8] = { 0 };

    baseline[0] = 0x20;
    baseline[1] = 0x1e;
    baseline[2] = (uint8_t)((co2 >> 8) & 0xff);
    baseline[3] = (uint8_t)(co2 & 0xff);
    baseline[4] = calc_crc8(baseline + 2, 2);
    baseline[5] = (uint8_t)((voc >> 8) & 0xff);
    baseline[6] = (uint8_t)(voc & 0xff);
    baseline[7] = calc_crc8(baseline + 5, 2);


    if( write(fd, baseline, 8) != 8 ){
        return false;
    }

    std::this_thread::sleep_for(10ms);

    return true;
}

/**
 * @brief Get the sensor baselines
 * 
 * 
 * @return std::optional<std::pair<uint16_t, uint16_t>> Pair of uint16_t baselines
 */
std::optional<std::pair<uint16_t, uint16_t>> sgp30::get_baseline(){
    uint8_t command[2] = { 0x20, 0x15 };
    uint8_t response[6] = { 0 };

    if( write(fd, command, 2) != 2 ){
        return {};
    }

    std::this_thread::sleep_for(10ms);

    if( read(fd, response, 6) != 6 ){
        return {};
    }

    if( (calc_crc8(response, 2) != response[2]) or (calc_crc8(response + 3, 2) != response[5]) ){
        printf("CRC invalid\n");
        return {};
    }

    std::pair<uint16_t, uint16_t> ret;
    ret.first = ((uint16_t)response[0] << 8) | (uint16_t)response[1];
    ret.second = ((uint16_t)response[3] << 8) | (uint16_t)response[4];

    return ret;
}

/**
 * @brief Calculate CRC8
 * 
 * @param data Data array
 * @param len Length to calculate CRC over
 * @return uint8_t CRC value
 */
uint8_t sgp30::calc_crc8(uint8_t *data, uint8_t len){
    uint8_t crc = CRC8_init;

    for(auto i = 0; i < len; ++i){
        crc ^= data[i];
        for(auto bit = 0; bit < 8; ++bit){
            if(crc & 0x80){
                crc = (crc << 1) ^ CRC8_poly;
            }
            else{
                crc = crc << 1;
            }

        }
    }

    return crc;
}