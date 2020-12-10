# air_quality_measurements
Air quality measurements interface using DHT22 (temperature and humidity) and SGP30(CO2 and TVOC).

## Usage

After starting the program the web interface will be started on port 8080 on the address of the device. By accessing the webpage you can get instant readings from the sensor.

A log in the .csv file is also written every 1.5 minutes by averaging the last 6 measurements (done at 15 seconds intervals).