#pragma once

#include "VentilationState.hpp"

#include <cstdint>
#include <Arduino.h>

struct measurement_entry {
    uint32_t relative_time;
    uint16_t co2_ppm: 11; // 11 bits 0..2047
    uint16_t relative_humidity: 10; // 10 bits 0..1023 (divide by 10 to get RH%) 
    int16_t temperature_c: 10; // 10 bits 0..1023 each step is 0.1 *C -51.2 to +51.2
    uint16_t sensor_status;
    requested_ventilation_state state_at_this_time : 2;
    unsigned long sequence_number;

    String toString() const;
    String getHeaders() const;

    void set_rh(float rh);
    float get_rh() const;

    void set_co2(int co2);
    int get_co2() const;

    void set_temp(float temp);
    float get_temp() const;
};

struct measurement_entry_avg {
    float co2_ppm;
    float relative_humidity;
    float temperature_c;
    float state_at_this_time;
};
