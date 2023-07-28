#include "Sensor_S8.hpp"

#include "LittleFS.hpp"

#include <s8_uart.h>

Sensor_S8::Sensor_S8(int hardware_serial_nr, SENSOR_LOCATION location):
	Sensor_Interface{}, ss{ hardware_serial_nr }, uart{nullptr}, data{},
	found{false}, last_measurement_time{ 0 }, new_measurement_available{ false }, 
    manual_calibration_failure {false}, calibration_status{ CALIBRATION_STATUS::UNKNOWN },
    abc_status{ ABC_STATUS::UNKNOWN }, perform_manual_calibration_time{ 0 }, location{ location }
{
    data.co2 = 0;
    data.meter_status = 0;
}

Sensor_S8::~Sensor_S8()
{
    if (uart != nullptr) {
        delete uart;
        uart = nullptr;
    }
}

void Sensor_S8::setup()
{  
    if (uart != nullptr) {
        ss.end();
        delete uart;
        uart = nullptr;
    }

    ss.begin(S8_BAUDRATE);
    uart = new S8_UART(ss);

    // Check if S8 is available
    uart->get_firmware_version(data.firm_version);
    int len = strlen(data.firm_version);
    if (len == 0) {
        Serial.println("SenseAir S8 CO2 sensor not found!");
        return;
    }

    found = true;

    // Show basic S8 sensor info
    Serial.println(">>> SenseAir S8 NDIR CO2 sensor <<<");
    printf("Firmware version: %s\n", data.firm_version);
    data.sensor_type_id = uart->get_sensor_type_ID();
    Serial.print("Sensor type: 0x"); printIntToHex(data.sensor_type_id, 3); Serial.println("");

    // Setting ABC period
    Serial.println("Setting ABC period set to 180 hours...");
    data.abc_period = uart->get_ABC_period();
    if (data.abc_period != 180) {
        delay(1000);
        uart->set_ABC_period(180);
        delay(1000);
        data.abc_period = uart->get_ABC_period();
        if (data.abc_period == 180) {
            abc_status = ABC_STATUS::OK;
            Serial.println("ABC period set succesfully");
        }
        else {
            abc_status = ABC_STATUS::FAIL;
            Serial.println("Error: ABC period doesn't set!");
        }
    }
    else {
        abc_status = ABC_STATUS::OK;
        Serial.println("ABC period already 180 hours!");
    }

    if (!global_config_data.manual_calibration_performed) {
        calibration_status = CALIBRATION_STATUS::MANUAL_CALIBRATION_NOT_PERFORMED_YET;
    }
    else {
        calibration_status = CALIBRATION_STATUS::OK;
    }

    last_measurement_time = millis();
    perform_manual_calibration_time = millis() + perform_manual_calibration_after_ms;
}

void Sensor_S8::print_measurement() const
{
    Serial.print(F("CO2: "));
    Serial.println(data.co2);
}

void Sensor_S8::update()
{
    unsigned long now = millis();
    unsigned long elapsed = now - last_measurement_time;

    if (!global_config_data.manual_calibration_performed && !manual_calibration_failure) {
        if (now > perform_manual_calibration_time) {
            Serial.println("Performing calibration..");
            global_config_data.manual_calibration_performed = uart->manual_calibration();

            if (global_config_data.manual_calibration_performed) {
                littlefs_write_config();
                littlefs_read_config();
                if (!global_config_data.manual_calibration_performed) {
                    Serial.println("Cannot save calibration result!");
                    calibration_status = CALIBRATION_STATUS::CALIBRATION_SUCCESS_BUT_FAILED_TO_SAVE_TO_DISK;
                }
                else {
                    calibration_status = CALIBRATION_STATUS::OK;
                    Serial.println("Calibration success!");
                }
            }
            else {
                calibration_status = CALIBRATION_STATUS::SENSOR_CALIBRATION_FAILED;
                manual_calibration_failure = true;
                Serial.println("Calibration failure!");
            }
        }
    }

    if (elapsed > meassurement_elapsed_millis) {
        last_measurement_time = now;
        new_measurement_available = true;

        data.co2 = uart->get_co2();
        data.meter_status = uart->get_meter_status();

        if (data.meter_status & S8_MASK_METER_ANY_ERROR) {
            Serial.println("One or more errors detected!");

            if (data.meter_status & S8_MASK_METER_FATAL_ERROR) {
                Serial.println("Fatal error in sensor!");
            }

            if (data.meter_status & S8_MASK_METER_OFFSET_REGULATION_ERROR) {
                Serial.println("Offset regulation error in sensor!");
            }

            if (data.meter_status & S8_MASK_METER_ALGORITHM_ERROR) {
                Serial.println("Algorithm error in sensor!");
            }

            if (data.meter_status & S8_MASK_METER_OUTPUT_ERROR) {
                Serial.println("Output error in sensor!");
            }

            if (data.meter_status & S8_MASK_METER_SELF_DIAG_ERROR) {
                Serial.println("Self diagnostics error in sensor!");
            }

            if (data.meter_status & S8_MASK_METER_OUT_OF_RANGE) {
                Serial.println("Out of range in sensor!");
            }

            if (data.meter_status & S8_MASK_METER_MEMORY_ERROR) {
                Serial.println("Memory error in sensor!");
            }
        }
    }
}

bool Sensor_S8::has_new_data()
{
	bool ret = new_measurement_available;
	new_measurement_available = false;
	return ret;
}

bool Sensor_S8::has_co2_ppm() const
{
	return true;
}

int Sensor_S8::get_co2_ppm()
{
	return data.co2;
}

bool Sensor_S8::has_meter_status() const
{
	return true;
}

int Sensor_S8::get_meter_status()
{
    MeterStatusUnion result;
    result.combined = 0;
    result.split.meter_status = (uint8_t)data.meter_status;
    result.split.calibration_status = (uint8_t)calibration_status;
    result.split.abc_status = (uint8_t)abc_status;

	return result.combined;
}

bool Sensor_S8::is_calibrated() const {
    return calibration_status == CALIBRATION_STATUS::OK;
}

bool Sensor_S8::is_calibration_in_progress() const {
    return calibration_status == CALIBRATION_STATUS::MANUAL_CALIBRATION_NOT_PERFORMED_YET;
}

bool Sensor_S8::has_calibration_error() const {
    switch (calibration_status) {
    case CALIBRATION_STATUS::SENSOR_CALIBRATION_FAILED:
    case CALIBRATION_STATUS::CALIBRATION_SUCCESS_BUT_FAILED_TO_SAVE_TO_DISK:
        return true;

    default:
        return false;
    }

    return false;
}

Sensor_S8::CALIBRATION_STATUS Sensor_S8::get_calibration_error() const {
    return calibration_status;
}

Sensor_S8::ABC_STATUS Sensor_S8::get_abc_status() const {
    return abc_status;
}

const char* const Sensor_S8::get_name() const
{
    return "S8";
}

bool Sensor_S8::is_present() const {
    return found;
}

SENSOR_LOCATION Sensor_S8::get_location() const {
    return location;
}
