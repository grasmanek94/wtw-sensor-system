#pragma once

#include <ESP32Time.h>
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
