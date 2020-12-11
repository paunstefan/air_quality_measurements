/**
 * @file main.cpp
 * @author Paun Stefan
 * @brief Entry point for the air_quality program
 * @version 0.1
 * @date 2020-12-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <iostream>
#include <string>
#include <utility>
#include <optional>
#include <mutex>
#include <thread>
#include <chrono>
#include <gpiod.h>
#include <fstream>
#include <ctime>
#include "httplib.h"
#include "temperature_sensor.h"
#include "sgp30.h"

#define PER_LOG 6        /* how many measurements to average per each log */

constexpr uint16_t baselines[2] = { 36758, 38310 };

std::mutex temp_mtx;
std::mutex co2_mtx;

/**
 * @brief Write air quality data to air_log.csv
 *        Data is averaged over multiple measurements
 *
 * @param temp_s Pointer to temperature sensor object
 * @param co2_s Pointer to temperature co2 object
 */
void log_loop(std::shared_ptr<temperature_sensor> temp_s,  std::shared_ptr<sgp30> co2_s){
    using namespace std::chrono_literals;

    std::ofstream log_file;
    log_file.open("air_log.csv", std::ios::app);
    if(!log_file.is_open()){
        std::cout << "Couldn't open file\n";
        exit(1);
    }

    int32_t count = 0;

    float temp_avg = 0.0;
    float humi_avg = 0.0;
    float co2_avg = 0.0;
    float voc_avg = 0.0;

    uint32_t looper = 0;

    while(true){
        /* CO2 sensor must be read every second for best functioning */
        co2_mtx.lock();
        auto co2_data = co2_s->measure_air_quality();
        co2_mtx.unlock();

        if(looper >= 15){
            temp_mtx.lock();
            auto temp_data = temp_s->read_data();
            temp_mtx.unlock();

            if(temp_data and co2_data){
#ifdef DEBUG
                std::cout << "T:" + std::to_string((*temp_data).first) + "  H:" + 
                        std::to_string((*temp_data).second) + "\n";
#endif

                count++;
                temp_avg += temp_data->first;
                humi_avg += temp_data->second;
                co2_avg += co2_data->first;
                voc_avg += co2_data->second;


                if( PER_LOG == count ){
                    temp_avg /= PER_LOG;
                    humi_avg /= PER_LOG;
                    co2_avg /= PER_LOG;
                    voc_avg /= PER_LOG;
#ifdef DEBUG
                    std::cout << "avg_T:" + std::to_string(temp_avg) + "  avg_H:" + 
                        std::to_string(humi_avg) + "\n";
#endif
                    log_file << std::to_string(std::time(nullptr)) << ",";
                    log_file << std::to_string(temp_avg) << ",";
                    log_file << std::to_string(humi_avg) << ",";
                    log_file << std::to_string(co2_avg) << ",";
                    log_file << std::to_string(voc_avg) << "\n";

                    log_file.flush();

                    temp_avg = 0.0;
                    humi_avg = 0.0;
                    co2_avg = 0.0;
                    voc_avg = 0.0;
                    count = 0;
                }
            }
            else{
#ifdef DEBUG
                std::cout << "Error\n";
#endif
                continue;
            }
            looper = 0;
        }
        std::this_thread::sleep_for(1s);
        looper++;
    }

    log_file.close();
}

int main(){
    using namespace std::chrono_literals;

    struct gpiod_chip *chip;
	struct gpiod_line *line;

    chip = gpiod_chip_open("/dev/gpiochip0");
	if (!chip){
		std::cout << "Error opening GPIO chip\n";
		return -1;
	}

	line = gpiod_chip_get_line(chip, 4);
	if (!line) {
        std::cout << "Error acquiring GPIO line\n";
		gpiod_chip_close(chip);
		return -1;
	}

    auto temp_s = std::make_shared<temperature_sensor>(line);

    auto co2_s = std::make_shared<sgp30>();

    co2_s->init_air_quality();
    co2_s->set_baseline(baselines[0], baselines[1]);
    std::cout << "Waiting for 16 seconds for SGP30 sensor to warm up\n";
    for(int i = 0; i < 16; i++){
        std::cout << ".";
        std::cout.flush();
        std::this_thread::sleep_for(1s);
    }
    std::cout << " Done\n";

    httplib::Server svr;

    std::cout << "Starting server\n";

    std::thread logging_thread(log_loop, temp_s, co2_s);

    svr.Get("/", [&temp_s, &co2_s](const httplib::Request &, httplib::Response &res) {
        temp_mtx.lock();
        auto temp_data = temp_s->read_data();
        temp_mtx.unlock();

        co2_mtx.lock();
        auto co2_data = co2_s->measure_air_quality();
        co2_mtx.unlock();


        if(temp_data and co2_data){

            std::string html_resp = "<h2>Air quality</h2><h3>Temperature: " + 
                            std::to_string(temp_data->first) + " C</h3><h3>Humidity: " 
                            + std::to_string(temp_data->second) + " %</h3><h3>CO2: "
                            + std::to_string(co2_data->first) + " ppm</h3><h3>TVOC: "
                            + std::to_string(co2_data->second) + " ppb</h3>";

            res.set_content(html_resp, "text/html");
        }
        else{
            res.set_content("<h2>Error</h2>", "text/html");
        }
    });

    svr.Get("/baseline", [&co2_s](const httplib::Request &, httplib::Response &res) {
        co2_mtx.lock();
        auto bl = co2_s->get_baseline();
        co2_mtx.unlock();

        if(bl){
            std::string html_resp = "<h2>Baselines</h2><h3>CO2: "
                                + std::to_string(bl->first) + "</h3><h3>TVOC: "
                                + std::to_string(bl->second) + "</h3>";

            res.set_content(html_resp, "text/html");
        }
        else{
            res.set_content("<h2>Error</h2>", "text/html");
        }
    });

    svr.listen("0.0.0.0", 8080);

    logging_thread.join();

    gpiod_chip_close(chip);

    return 0;
}