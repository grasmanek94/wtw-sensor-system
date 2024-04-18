#pragma once

#define GPS_TIME_ENABLED 0

#if GPS_TIME_ENABLED
#include "ESP32TimeFixed.hpp"

#include <Timezone_Generic.h>
#include <Timezone_Generic.hpp>
#include <TimeLib.h>
#include <TinyGPS++.h>
#include <TinyGPSPlus.h>

class gps_time {
private:
	TinyGPSPlus* gps;
	HardwareSerial* hw_serial;
	ESP32Time* rtc;

	void reset();

public:
	gps_time();
	virtual ~gps_time();

	void setup();
	void update();
};
#endif