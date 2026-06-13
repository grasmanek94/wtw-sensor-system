#pragma once

#include "Sensor_Interface.hpp"
#include "MeterStatus.hpp"

#include <HardwareSerial.h>
#include <s8_uart.h>

#ifndef SENSOR_INTERFACE_S8_INCLUDED
#define SENSOR_INTERFACE_S8_INCLUDED 1
#endif

class Sensor_S8 : public Sensor_Interface {
public:
	enum class CALIBRATION_STATUS : uint8_t { // 3 bits but takes 4 bits
		UNKNOWN,
		MANUAL_CALIBRATION_NOT_PERFORMED_YET,
		OK,		
		CALIBRATION_SUCCESS_BUT_FAILED_TO_SAVE_TO_DISK,
		SENSOR_CALIBRATION_FAILED
	};

	enum class ABC_STATUS: uint8_t { // 2 bits but takes 4 bits
		UNKNOWN,
		OK,
		FAIL
	};


private:
	HardwareSerial ss;
	S8_UART* uart;
	S8_sensor data;

	bool found;
	unsigned long last_measurement_time;
	bool new_measurement_available;
	bool manual_calibration_failure;
	CALIBRATION_STATUS calibration_status;
	ABC_STATUS abc_status;
	unsigned long perform_manual_calibration_time;
	SENSOR_LOCATION location;

	const unsigned long meassurement_elapsed_millis = 4000;
	const unsigned long perform_manual_calibration_after_ms = 5 * 60 * 1000; // 5 min
//	const unsigned long perform_manual_calibration_after_ms = 20 * 1000; // 20 s

public:

	Sensor_S8(int hardware_serial_nr = 2, SENSOR_LOCATION location = (SENSOR_LOCATION)0);
	virtual ~Sensor_S8();

	virtual void setup() override;
	virtual void print_measurement() const override;
	virtual void update() override;
	virtual bool has_new_data() override;
	virtual bool has_co2_ppm() const override;
	virtual bool has_meter_status() const override;
	virtual bool is_present() const override;
	virtual int get_co2_ppm() override;
	virtual int get_meter_status() override;

	bool is_calibrated() const;
	bool is_calibration_in_progress() const;
	bool has_calibration_error() const;
	CALIBRATION_STATUS get_calibration_error() const;
	ABC_STATUS get_abc_status() const;

	virtual const char* const get_name() const override;
	virtual SENSOR_LOCATION get_location() const override;
};
