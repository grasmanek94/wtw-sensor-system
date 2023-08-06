#include "GPSTime.hpp"
#include "LittleFS.hpp"

#include <ESP32Time.h>

const int offset_seconds = 7200;

gps_time::gps_time(): gps{nullptr}, hw_serial{nullptr} {}

void gps_time::reset() {
	if (gps != nullptr) {
		delete gps;
		gps = nullptr;
	}

	if (hw_serial != nullptr) {
		delete hw_serial;
		hw_serial = nullptr;
	}

	if (rtc != nullptr) {
		delete rtc;
		rtc = nullptr;
	}
}

gps_time::~gps_time() {
	reset();
}

void gps_time::setup() {
	reset();

	if (!global_config_data.use_gps_time) {
		return;
	}

	hw_serial = new HardwareSerial(global_config_data.get_gps_time_uart_nr());
	gps = new TinyGPSPlus();
	rtc = new ESP32Time(offset_seconds);

	hw_serial->begin(global_config_data.get_gps_baud());

	Serial.println(F("Using GPS based Time"));
}

void gps_time::update() {
	if (!global_config_data.use_gps_time) {
		return;
	}

	// This sketch displays information every time a new sentence is correctly encoded.
	while (hw_serial->available() > 0)
	{
		if (!gps->encode(hw_serial->read()))
		{
			continue;
		}

		if (!gps->date.isValid() || !gps->time.isValid()) {
			continue;
		}

		if (!gps->date.isUpdated() && !gps->time.isUpdated()) {
			continue;
		}

		// date and time valid, date or time is updated
		rtc->setTime(gps->time.second(), gps->time.minute(), gps->time.hour(), gps->date.day(), gps->date.month(), gps->date.year());

		if (!Serial) {
			continue;
		}

		Serial.print(F("  Date/Time: "));

		Serial.print(gps->date.month());
		Serial.print(F("/"));
		Serial.print(gps->date.day());
		Serial.print(F("/"));
		Serial.print(gps->date.year());
		if (gps->time.hour() < 10) Serial.print(F("0"));
		Serial.print(gps->time.hour());
		Serial.print(F(":"));
		if (gps->time.minute() < 10) Serial.print(F("0"));
		Serial.print(gps->time.minute());
		Serial.print(F(":"));
		if (gps->time.second() < 10) Serial.print(F("0"));
		Serial.print(gps->time.second());
		Serial.print(F("."));
		if (gps->time.centisecond() < 10) Serial.print(F("0"));
		Serial.print(gps->time.centisecond());
	}
}
