#include "Sensor_SHT31.hpp"

#include <Wire.h>
#include <SHT31.h>

static bool already_setup_Wire0 = false;

Sensor_SHT31::Sensor_SHT31(int i2c_sda_pin, int i2c_scl_pin, 
	SENSOR_LOCATION location,
	float temp_offset_x, float temp_offset_y,
	int wire):
	Sensor_Interface{}, SensorHasTempOffset{ temp_offset_x, temp_offset_y },
	i2c_sda_pin{ i2c_sda_pin }, i2c_scl_pin{ i2c_scl_pin }, wire{ wire },
	sht31 {}, last_measured_rh_value{ 0.0f }, last_measured_temp{ 0.0f },
	new_measurement_available {false}, location{ location }, i2c_intf{nullptr}
{}

Sensor_SHT31::~Sensor_SHT31()
{
	if (i2c_intf != nullptr) {
		delete i2c_intf;
		i2c_intf = nullptr;
	}
}

void Sensor_SHT31::setup()
{
	if (i2c_intf != nullptr) {
		delete i2c_intf;
		i2c_intf = nullptr;
	}

	i2c_intf = new TwoWire(wire);
	i2c_intf->begin(i2c_sda_pin, i2c_scl_pin, 100000);
	delay(1000);

	sht31.begin(SHT31_ADDRESS, i2c_intf);
	
	uint16_t stat = sht31.readStatus();
	Serial.print(stat, HEX);
	Serial.println();

	sht31.requestData();
}

void Sensor_SHT31::print_measurement() const
{
	Serial.print(F("Relative Humidity: "));
	Serial.println(last_measured_rh_value);
	Serial.print(F("Temperature: "));
	Serial.println(last_measured_temp);
}

void Sensor_SHT31::update()
{
	if (!sht31.dataReady())
	{
		return;
	}

	if (sht31.readData())
	{
		last_measured_rh_value = sht31.getHumidity();
		last_measured_temp = sht31.getTemperature();
		adjust_temp_and_humidity(last_measured_temp, last_measured_rh_value);
		new_measurement_available = true;
	}

	sht31.requestData();
}

bool Sensor_SHT31::has_new_data()
{
	bool ret = new_measurement_available;
	new_measurement_available = false;
	return ret;
}

bool Sensor_SHT31::has_temperature() const
{
	return true;
}

bool Sensor_SHT31::has_relative_humidity() const
{
	return true;
}

float Sensor_SHT31::get_temperature()
{
	return last_measured_temp;
}

float Sensor_SHT31::get_relative_humidity()
{
	return last_measured_rh_value;
}

const char* const Sensor_SHT31::get_name() const
{
	return "SHT31";
}

SENSOR_LOCATION Sensor_SHT31::get_location() const {
	return location;
}
