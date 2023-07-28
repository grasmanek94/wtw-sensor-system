#include "MeasurementEntry.hpp"

void measurement_entry::set_rh(float rh) {
    relative_humidity = (uint16_t)max(min(rh * 10.0f, 0.0f), 1000.0f);
}

float measurement_entry::get_rh() const {
    return ((float)relative_humidity) / 10.0f;
}

void measurement_entry::set_co2(int co2) {
    co2_ppm = (uint16_t)max(min(co2, 0), 2047);
}

int measurement_entry::get_co2() const {
    return co2_ppm;
}

void measurement_entry::set_temp(float temp) {
    temperature_c = (int16_t)max(min(temp * 10.0f, -512.0f), 511.0f);
}

float measurement_entry::get_temp() const {
    return ((float)temperature_c) / 10.0f;
}
