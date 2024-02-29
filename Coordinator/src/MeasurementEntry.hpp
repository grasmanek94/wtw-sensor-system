#pragma once

#include "VentilationState.hpp"

#include <cstdint>
#include <Arduino.h>

#pragma pack(push, 1)
struct measurement_entry {
public:
    uint32_t relative_time;
private:
    uint16_t co2_ppm: 12; // 12 bits 0..4095
    uint16_t relative_humidity: 10; // 10 bits 0..1023 (divide by 10 to get RH%) 
    int16_t temperature_c: 10; // 10 bits 0..1023 each step is 0.1 *C -51.2 to +51.2
    uint16_t attainable_humidity : 10;
    requested_ventilation_state state_at_this_time : 2;
public:

    uint16_t sensor_status;
    unsigned long sequence_number;

    String toString() const;
    String getHeaders() const;

    void set_rh(float rh);
    float get_rh() const;

    void set_co2(int co2);
    int get_co2() const;

    void set_temp(float temp);
    float get_temp() const;

    void set_attainable_rh(float rh);
    float get_attainable_rh() const;

    void set_state_at_this_time(requested_ventilation_state state);
    requested_ventilation_state get_state_at_this_time() const;

    measurement_entry();
};
#pragma pack(pop)

const unsigned int measurement_entry_size = sizeof(measurement_entry);
static_assert(measurement_entry_size == 16);

struct measurement_entry_avg {
    float co2_ppm;
    float relative_humidity;
    float temperature_c;
    float state_at_this_time;
    float attainable_humidity;
};
