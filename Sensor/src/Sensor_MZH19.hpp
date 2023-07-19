#pragma once

#include "Sensor_Interface.hpp"

#include <HardwareSerial.h>
#include <MHZ19.h>

class Sensor_MHZ19: public Sensor_Interface {
private:
	HardwareSerial ss;
	MHZ19 mhz;

	bool auto_calibration;
	float last_measured_temp;
	int last_measured_co2_ppm;
	unsigned long last_measurement_time;
	bool new_measurement_available;

	const unsigned long meassurement_elapsed_millis = 4000;

public:
	Sensor_MHZ19(bool auto_calibration = false, int hardware_serial_nr = 2);
	virtual ~Sensor_MHZ19();

	virtual void setup() override;
	virtual void print_measurement() const override;
	virtual void update() override;
	virtual bool has_new_data() override;
	virtual bool has_temperature() const override;
	virtual bool has_co2_ppm() const override;
	virtual float get_temperature() override;
	virtual int get_co2_ppm() override;

	virtual const char* const get_name() const override;
};
