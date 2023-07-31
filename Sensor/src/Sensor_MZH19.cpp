#include "Sensor_MZH19.hpp"

Sensor_MHZ19::Sensor_MHZ19(bool auto_calibration, int hardware_serial_nr, SENSOR_LOCATION location,
	float temp_offset_x, float temp_offset_y) :
	Sensor_Interface{}, SensorHasTempOffset{ temp_offset_x, temp_offset_y }, 
	auto_calibration{ auto_calibration},
	ss{ hardware_serial_nr }, mhz{}, last_measured_temp{ -273.15f },
	last_measured_co2_ppm{ -1 }, last_measurement_time{ 0 },
	new_measurement_available{ false }, location{ location }
{}

Sensor_MHZ19::~Sensor_MHZ19()
{}

void Sensor_MHZ19::setup()
{
	ss.begin(9600);
	mhz.begin(ss);
	mhz.autoCalibration(auto_calibration);
	last_measurement_time = millis();
}

void Sensor_MHZ19::print_measurement() const
{
	Serial.print(F("CO2: "));
	Serial.println(last_measured_co2_ppm);
	Serial.print(F("Temperature: "));
	Serial.println(last_measured_temp);
}

void Sensor_MHZ19::update()
{
	unsigned long now = millis();
	unsigned long elapsed = now - last_measurement_time;

	if (elapsed > meassurement_elapsed_millis) {
		last_measurement_time = now;
		last_measured_co2_ppm = mhz.getCO2();
		last_measured_temp = adjust_temp(mhz.getTemperature());
		new_measurement_available = true;
	}
}

bool Sensor_MHZ19::has_new_data()
{
	bool ret = new_measurement_available;
	new_measurement_available = false;
	return ret;
}

bool Sensor_MHZ19::has_temperature() const
{
	return true;
}

bool Sensor_MHZ19::has_co2_ppm() const
{
	return true;
}

float Sensor_MHZ19::get_temperature()
{
	return last_measured_temp;
}

int Sensor_MHZ19::get_co2_ppm()
{
	return last_measured_co2_ppm;
}

const char* const Sensor_MHZ19::get_name() const {
	return "MHZ19C";
}

SENSOR_LOCATION Sensor_MHZ19::get_location() const {
	return location;
}
