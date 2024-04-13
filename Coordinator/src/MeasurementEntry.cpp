#include "MeasurementEntry.hpp"

measurement_entry::measurement_entry() :
    relative_time{ 0 },
    co2_ppm{ 0 },
    relative_humidity{ 0 },
    temperature_c{ 0 },
    attainable_humidity{ 0 },
    state_at_this_time{ requested_ventilation_state_undefined },
    sensor_status{ 0 },
    sequence_number{ 0 },
    co2_matrix_state{ 0 }
{}

void measurement_entry::set_rh(float rh) {
    relative_humidity = (uint16_t)min(max(rh * 10.0f, 0.0f), 1000.0f);
}

float measurement_entry::get_rh() const {
    return ((float)relative_humidity) / 10.0f;
}

void measurement_entry::set_co2(int co2) {
    co2_ppm = (uint16_t)min(max(co2, 0), 4095);
}

int measurement_entry::get_co2() const {
    return co2_ppm;
}

void measurement_entry::set_temp(float temp) {
    temperature_c = (int16_t)min(max(temp * 10.0f, -512.0f), 511.0f);
}

float measurement_entry::get_temp() const {
    return ((float)temperature_c) / 10.0f;
}

void measurement_entry::set_attainable_rh(float rh) {
    attainable_humidity = (uint16_t)min(max(rh * 10.0f, 0.0f), 1000.0f);
}

float measurement_entry::get_attainable_rh() const {
    return ((float)attainable_humidity) / 10.0f;
}

void measurement_entry::set_state_at_this_time(requested_ventilation_state state) {
    state_at_this_time = state;
}

requested_ventilation_state measurement_entry::get_state_at_this_time() const {
    return state_at_this_time;
}

void measurement_entry::set_co2_matrix_state(int state) {
    co2_matrix_state = (uint8_t)min(max(state, 0), 9);
}

unsigned int measurement_entry::get_co2_matrix_state() const {
    return co2_matrix_state;
}

String measurement_entry::toString() const
{
    return
        String(relative_time) + ",\t"
        + String(get_co2()) + ",\t"
        + String(get_rh()) + ",\t"
        + String(get_attainable_rh()) + ",\t"
        + String(get_temp()) + ",\t"
        + String(sensor_status) + ",\t"
        + String(sequence_number) + ",\t"
        + String(state_at_this_time) + ",\t"
        + String(co2_matrix_state) + "\n";
}

String measurement_entry::getHeaders() const {
    return
        "relative_time,\t"
        "co2_ppm,\t"
        "rh,\t"
        "attainable_rh,\t"
        "temp_c,\t"
        "sensor_status,\t"
        "sequence_number,\t"
        "state_at_this_time,\t"
        "co2_matrix_state\n";
}
