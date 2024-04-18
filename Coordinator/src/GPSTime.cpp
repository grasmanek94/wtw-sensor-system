#include "GPSTime.hpp"

#if GPS_TIME_ENABLED
#include "ESP32TimeFixed.hpp"
#include "LittleFS.hpp"

#include <Timezone_Generic.hpp>
#include <TimeLib.h>


// Europe/Amsterdam Time Zone
TimeChangeRule EU_AMS_CET = { "CET", Last, Sun, Mar, 2, 120 };   // UTC + 2 hours (daylight saving time)
TimeChangeRule EU_AMS_CEST = { "CEST", Last, Sun, Oct, 3, 60 };    // UTC + 1 hours (standard)
// from https://www.timeanddate.com/time/change/netherlands/amsterdam?year=2024

Timezone EU_AMS(EU_AMS_CET, EU_AMS_CEST);

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
	rtc = new ESP32Time(0);

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
		struct tm t = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };      // Initalize to all 0's
		t.tm_year = gps->date.year() - 1900;    // This is year-1900, so 121 = 2021
		t.tm_mon = gps->date.month() - 1;
		t.tm_mday = gps->date.day();
		t.tm_hour = gps->time.hour();
		t.tm_min = gps->time.minute();
		t.tm_sec = gps->time.second();
		rtc->setTime(EU_AMS.toLocal(mktime(&t)), gps->time.centisecond() * 10);
		
		if (!Serial) {
			continue;
		}

		Serial.print(F("  Date/Time: "));
		Serial.println(rtc->getDateTime());
	}
}
#endif