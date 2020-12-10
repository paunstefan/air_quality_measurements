CXX=g++
CFLAGS=-Wall -Wextra -Werror
OBJ=src/main.cpp src/temperature_sensor.cpp src/sgp30.cpp
INCLUDE=-Iinclude

ifdef DEBUG
	CFLAGS+= -g -DDEBUG
endif

all: air_quality

air_quality: $(OBJ)
	$(CXX) -std=c++17 -o $@ $^ $(INCLUDE) $(CFLAGS) -lgpiod -lpthread

clean:
	rm ./air_quality