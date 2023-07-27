#pragma once

enum SENSOR_LOCATION {
	SENSOR_LOCATION_UNKNOWN,
	SENSOR_LOCATION_FLOOR_0_LIVING_ROOM,
	SENSOR_LOCATION_FLOOR_1_BATHROOM,
	SENSOR_LOCATION_FLOOR_1_BEDROOM_FRONT,
	SENSOR_LOCATION_FLOOR_1_BEDROOM_LEFT,
	SENSOR_LOCATION_FLOOR_1_BEDROOM_RIGHT,
	SENSOR_LOCATION_FLOOR_2_FRESH_AIR_INLET,
	SENSOR_LOCATION_FLOOR_2_USED_AIR_OUTLET,
	SENSOR_LOCATION_OUTSIDE
};

class Sensor_Interface {
public:
	Sensor_Interface() {}
	virtual ~Sensor_Interface() {}

	virtual const char* const get_name() const { return ""; }
	virtual void setup() {}
	virtual void print_measurement() const {}
	virtual void update() {}
	virtual bool has_new_data() { return false; }
	virtual bool has_temperature() const { return false; }
	virtual bool has_relative_humidity() const { return false; }
	virtual bool has_co2_ppm() const { return false; }
	virtual bool has_meter_status() const { return false; }
	virtual bool is_present() const { return true; }
	virtual float get_temperature() { return 0.0f; }
	virtual float get_relative_humidity() { return 0.0f; }
	virtual int get_co2_ppm() { return 0; }
	virtual int get_meter_status() { return 0; }
	virtual SENSOR_LOCATION get_location() { return SENSOR_LOCATION_UNKNOWN; }
};
