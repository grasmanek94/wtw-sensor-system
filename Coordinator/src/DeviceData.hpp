#pragma once

#include "VentilationState.hpp"
#include "RingBuf.h"

const size_t SENSORS_COUNT = 5;
const size_t SENSOR_EXPECTED_INTERVAL = 15 * 1000; // 15 s
const size_t SENSOR_VERY_SHORT_MEASUREMENT_PERIOD = 60 * 1000; // 1 min
const size_t SENSOR_SHORT_MEASUREMENT_PERIOD = 15 * 60 * 1000; // 15 min
const size_t SENSOR_LONG_MEASUREMENT_PERIOD = 24 * 60 * 60 * 1000; // 24 h
const size_t SENSOR_VERY_SHORT_MEASUREMENT_COUNT = SENSOR_VERY_SHORT_MEASUREMENT_PERIOD / SENSOR_EXPECTED_INTERVAL;
const size_t SENSOR_SHORT_MEASUREMENT_COUNT = SENSOR_SHORT_MEASUREMENT_PERIOD / SENSOR_VERY_SHORT_MEASUREMENT_PERIOD;
const size_t SENSOR_LONG_MEASUREMENT_COUNT = SENSOR_LONG_MEASUREMENT_PERIOD / SENSOR_SHORT_MEASUREMENT_PERIOD;

struct measurement_entry {
    uint32_t relative_time;
    uint16_t co2_ppm: 11; // 11 bits 0..2047
    uint16_t relative_humidity: 10; // 10 bits 0..1023 (divide by 10 to get RH%) 
    int16_t temperature_c : 10; // 10 bits 0..1023 each step is 0.1 *C -51.2 to +51.2
    long sensor_status;
    requested_ventilation_state state_at_this_time;
    unsigned long sequence_number;

    String toString() const;
    String getHeaders() const;
};

struct device_data {
    String id;
    size_t current_very_short_push_count;
    size_t current_short_push_count;
    RingBuf<measurement_entry, SENSOR_VERY_SHORT_MEASUREMENT_COUNT> very_short_data;
    RingBuf<measurement_entry, SENSOR_SHORT_MEASUREMENT_COUNT> short_data;
    RingBuf<measurement_entry, SENSOR_LONG_MEASUREMENT_COUNT> long_data;
    requested_ventilation_state current_ventilation_state_co2;
    requested_ventilation_state current_ventilation_state_rh;
    measurement_entry latest_measurement;
    measurement_entry _tmp_avg;

    device_data();

    template<typename T>
    void get_average(T& container, measurement_entry& avg) {
        const size_t size = container.size();

        avg.co2_ppm = 0;
        avg.rh = 0.0f;
        avg.temp_c = 0.0f;
        avg.sensor_status = 0;
        avg.sequence_number = 0;

        avg.state_at_this_time = requested_ventilation_state_low;
        for (int i = 0; i < size; ++i) {
            avg.co2_ppm += container[i].co2_ppm;
            avg.rh += container[i].rh;
            avg.temp_c += container[i].temp_c;
            avg.sensor_status |= container[i].sensor_status;

            // newest time = time of all averages
            avg.relative_time = max(avg.relative_time, container[i].relative_time);
            avg.sequence_number = max(avg.sequence_number, container[i].sequence_number);

            avg.state_at_this_time = get_highest_ventilation_state(container[i].state_at_this_time, avg.state_at_this_time);
        }

        avg.co2_ppm /= size;
        avg.rh /= (float)size;
        avg.temp_c /= (float)size;
    }

    template<typename T>
    requested_ventilation_state determine_current_ventilation_state(requested_ventilation_state current, T measured, T low, T medium, T high) const {
        switch (current) {
        case requested_ventilation_state_low:
            if (measured > high) {
                return requested_ventilation_state_high;
            }

            if (measured > medium) {
                return requested_ventilation_state_medium;
            }

            return requested_ventilation_state_low;
        case requested_ventilation_state_medium:
            if (measured < low) {
                return requested_ventilation_state_low;
            }

            if (measured > high) {
                return requested_ventilation_state_high;
            }

            return requested_ventilation_state_medium;
        case requested_ventilation_state_high:
            if (measured < low) {
                return requested_ventilation_state_low;
            }

            if (measured < medium) {
                return requested_ventilation_state_medium;
            }

            return requested_ventilation_state_high;
        }
        return requested_ventilation_state_low;
    }

    void push(int co2_ppm, float rh, float temp_c, int sensor_status, long sequence_number);

    bool is_associated() const;
    void associate(String identifier);

    bool has_recent_data() const;

    requested_ventilation_state get_highest_ventilation_state() const;
    requested_ventilation_state get_highest_ventilation_state(requested_ventilation_state a, requested_ventilation_state b) const;

    String toString(int index) const;
    String getHeaders() const;
};

extern device_data sensors[SENSORS_COUNT];