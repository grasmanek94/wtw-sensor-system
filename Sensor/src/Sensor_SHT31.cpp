#include "Sensor_SHT31.hpp"

#include <Wire.h>
#include <SHT31.h>

Sensor_SHT31::Sensor_SHT31(int i2c_sda_pin, int i2c_scl_pin):
	i2c_sda_pin{ i2c_sda_pin }, i2c_scl_pin{ i2c_scl_pin }, sht31{}, 
	last_measured_rh_value{ -1.0f }, last_measured_temp{ -273.15f },
	new_measurement_available {false}
{
}

Sensor_SHT31::~Sensor_SHT31()
{
}

void Sensor_SHT31::setup()
{
	Wire.begin(i2c_sda_pin, i2c_scl_pin);
	sht31.begin(SHT31_ADDRESS);
	Wire.setClock(100000);

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
