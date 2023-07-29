#pragma once

#include "MeasurementEntry.hpp"
#include "SensorLocation.hpp"
#include "VentilationState.hpp"
#include "RingBuf.h"

const size_t SENSORS_COUNT = (size_t)SENSOR_LOCATION::NUM_LOCATIONS;
const size_t SENSOR_EXPECTED_INTERVAL = 15 * 1000; // 15 s
const size_t SENSOR_VERY_SHORT_MEASUREMENT_PERIOD = 1 * 60 * 1000; // 1 min
const size_t SENSOR_SHORT_MEASUREMENT_PERIOD = 15 * 60 * 1000; // 15 min
const size_t SENSOR_LONG_MEASUREMENT_PERIOD = 7 * 24 * 60 * 60 * 1000; // 24h

const size_t SENSOR_VERY_SHORT_MEASUREMENT_COUNT = SENSOR_VERY_SHORT_MEASUREMENT_PERIOD / SENSOR_EXPECTED_INTERVAL;
const size_t SENSOR_SHORT_MEASUREMENT_COUNT = SENSOR_SHORT_MEASUREMENT_PERIOD / SENSOR_VERY_SHORT_MEASUREMENT_PERIOD;
const size_t SENSOR_LONG_MEASUREMENT_COUNT = SENSOR_LONG_MEASUREMENT_PERIOD / SENSOR_SHORT_MEASUREMENT_PERIOD;

struct device_data {
    String id;
    SENSOR_LOCATION loc;
    size_t current_very_short_push_count;
    size_t current_short_push_count;
    RingBuf<measurement_entry, SENSOR_VERY_SHORT_MEASUREMENT_COUNT> very_short_data;
    RingBuf<measurement_entry, SENSOR_SHORT_MEASUREMENT_COUNT> short_data;
    RingBuf<measurement_entry, SENSOR_LONG_MEASUREMENT_COUNT> long_data;
    requested_ventilation_state current_ventilation_state_co2;
    requested_ventilation_state current_ventilation_state_rh;
    measurement_entry latest_measurement;
    measurement_entry average_measurement;
    measurement_entry_avg _tmp_avg;

    device_data();

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

    void push(int co2_ppm, float rh, float temp_c, int sensor_status, unsigned long sequence_number);

    bool is_associated() const;
    void associate(String identifier, SENSOR_LOCATION location = SENSOR_LOCATION::BATHROOM);

    bool has_recent_data() const;

    requested_ventilation_state get_highest_ventilation_state() const;
    requested_ventilation_state get_highest_ventilation_state(requested_ventilation_state a, requested_ventilation_state b) const;

    String toString(int index) const;
    String getHeaders() const;
};

extern device_data sensors[SENSORS_COUNT];

// try to keep this below ~16KiB, shall we?
const size_t TOTAL_SENSORS_DATA_SIZE = sizeof(sensors);
const size_t MAX_LOCATIONS_PER_DEVICE = 3;
