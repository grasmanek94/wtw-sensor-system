#pragma once

#include "Sensor_Interface.hpp"

#include <Adafruit_SHT4x.h>

class Sensor_SHT4X : public Sensor_Interface {
private:
	int i2c_sda_pin;
	int i2c_scl_pin;
	Adafruit_SHT4x sht4;
	sht4x_precision_t precision;
	float last_measured_rh_value;
	float last_measured_temp;
	bool new_measurement_available;
	bool found;

public:
	Sensor_SHT4X(int i2c_sda_pin = 15, int i2c_scl_pin = 4, sht4x_precision_t precision = SHT4X_HIGH_PRECISION);
	virtual ~Sensor_SHT4X();

	virtual void setup() override;
	virtual void print_measurement() const override;
	virtual void update() override;
	virtual bool has_new_data() override;
	virtual bool has_temperature() const override;
	virtual bool has_relative_humidity() const override;
	virtual bool is_present() const override;
	virtual float get_temperature() override;
	virtual float get_relative_humidity() override;

	virtual const char* const get_name() const override;
};
