#include "Sensor_SHT4X.hpp"

#include <Adafruit_SHT4x.h>
#include <Wire.h>

Sensor_SHT4X::Sensor_SHT4X(int i2c_sda_pin, int i2c_scl_pin, sht4x_precision_t precision, SENSOR_LOCATION location) :
	i2c_sda_pin{ i2c_sda_pin }, i2c_scl_pin{ i2c_scl_pin }, sht4{}, precision{precision},
    last_measured_rh_value{0.0f}, last_measured_temp{0.0f}, 
    new_measurement_available{false}, found{false}, location{ location }
{}

Sensor_SHT4X::~Sensor_SHT4X()
{}

void Sensor_SHT4X::setup()
{
    Wire1.setPins(i2c_sda_pin, i2c_scl_pin);
    Wire1.setClock(10000);
    delay(1000);
    if (!sht4.begin(&Wire1)) {
        found = false;
        Serial.println("SHT4x sensor NOT FOUND");
        return;
    }
    found = true;

    Serial.println("Found SHT4x sensor");
    Serial.print("Serial number 0x");
    Serial.println(sht4.readSerial(), HEX);

    // You can have 3 different precisions, higher precision takes longer
    sht4.setPrecision(precision);
    switch (sht4.getPrecision()) {
    case SHT4X_HIGH_PRECISION:
        Serial.println("High precision");
        break;
    case SHT4X_MED_PRECISION:
        Serial.println("Med precision");
        break;
    case SHT4X_LOW_PRECISION:
        Serial.println("Low precision");
        break;
    }

    // You can have 6 different heater settings
    // higher heat and longer times uses more power
    // and reads will take longer too!
    sht4.setHeater(SHT4X_NO_HEATER);
    switch (sht4.getHeater()) {
    case SHT4X_NO_HEATER:
        Serial.println("No heater");
        break;
    case SHT4X_HIGH_HEATER_1S:
        Serial.println("High heat for 1 second");
        break;
    case SHT4X_HIGH_HEATER_100MS:
        Serial.println("High heat for 0.1 second");
        break;
    case SHT4X_MED_HEATER_1S:
        Serial.println("Medium heat for 1 second");
        break;
    case SHT4X_MED_HEATER_100MS:
        Serial.println("Medium heat for 0.1 second");
        break;
    case SHT4X_LOW_HEATER_1S:
        Serial.println("Low heat for 1 second");
        break;
    case SHT4X_LOW_HEATER_100MS:
        Serial.println("Low heat for 0.1 second");
        break;
    }
}

void Sensor_SHT4X::print_measurement() const
{
    Serial.print(F("Relative Humidity: "));
    Serial.println(last_measured_rh_value);
    Serial.print(F("Temperature: "));
    Serial.println(last_measured_temp);
}

void Sensor_SHT4X::update()
{
    sensors_event_t humidity, temp;

    if (sht4.getEvent(&humidity, &temp)) {
        last_measured_rh_value = humidity.relative_humidity;
        last_measured_temp = temp.temperature;
        new_measurement_available = true;
    }
}

bool Sensor_SHT4X::has_new_data()
{
    bool ret = new_measurement_available;
    new_measurement_available = false;
    return ret;
}

bool Sensor_SHT4X::has_temperature() const
{
	return true;
}

bool Sensor_SHT4X::has_relative_humidity() const
{
	return true;
}

float Sensor_SHT4X::get_temperature()
{
	return last_measured_temp;
}

float Sensor_SHT4X::get_relative_humidity()
{
	return last_measured_rh_value;
}

const char* const Sensor_SHT4X::get_name() const
{
    return "SHT40";
}

bool Sensor_SHT4X::is_present() const {
    return found;
}

SENSOR_LOCATION Sensor_SHT4X::get_location() const {
    return location;
}
