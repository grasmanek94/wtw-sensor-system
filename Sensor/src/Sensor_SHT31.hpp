#pragma once

#include "Sensor_Interface.hpp"
#include "SensorHasTempOffset.hpp"

#include <SHT31.h>

#ifndef SENSOR_INTERFACE_SHT31_INCLUDED
#define SENSOR_INTERFACE_SHT31_INCLUDED 1
#endif

class Sensor_SHT31 : public Sensor_Interface, public SensorHasTempOffset {
private:
	const int SHT31_ADDRESS = 0x44;

	int i2c_sda_pin;
	int i2c_scl_pin;
	int wire;
	SHT31 sht31;
	float last_measured_rh_value;
	float last_measured_temp;
	bool new_measurement_available;
	SENSOR_LOCATION location;
	TwoWire* i2c_intf;

public:
	Sensor_SHT31(int i2c_sda_pin = 23, int i2c_scl_pin = 22, 
		SENSOR_LOCATION location = (SENSOR_LOCATION)0,
		float temp_offset_x = 1.0f, float temp_offset_y = 0.0f,
		int wire = 0);

	virtual ~Sensor_SHT31();

	virtual void setup() override;
	virtual void print_measurement() const override;
	virtual void update() override;
	virtual bool has_new_data() override;
	virtual bool has_temperature() const override;
	virtual bool has_relative_humidity() const override;
	virtual float get_temperature() override;
	virtual float get_relative_humidity() override;

	virtual const char* const get_name() const override;
	virtual SENSOR_LOCATION get_location() const override;
};
