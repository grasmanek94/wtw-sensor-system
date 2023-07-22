#include "RingBuf.h"
#include "LittleFS.hpp"

#include "VentilationState.hpp"
#include "DeviceData.hpp"

#include <time.h>

device_data sensors[SENSORS_COUNT];

device_data::device_data() :
    id{ "" },
    current_very_short_push_count{ 0 },
    current_short_push_count{ 0 },
    short_data{},
    long_data{},
    // !! safety !! - Occupants present = medium is required. Low is "no occupants".
    current_ventilation_state_co2{ requested_ventilation_state_medium },
    current_ventilation_state_rh{ requested_ventilation_state_medium },
    latest_measurement{ 0, 0, 0.0f, 0.0f },
    _tmp_avg{ 0, 0, 0.0f, 0.0f }
{}

void device_data::push(int co2_ppm, float rh, float temp_c, int sensor_status, long sequence_number) {
    unsigned long now = (unsigned long)time(NULL);

    current_ventilation_state_co2 = determine_current_ventilation_state(current_ventilation_state_co2, co2_ppm, global_config_data.co2_ppm_low, global_config_data.co2_ppm_medium, global_config_data.co2_ppm_high);
    current_ventilation_state_rh = determine_current_ventilation_state(current_ventilation_state_rh, rh, global_config_data.rh_low, global_config_data.rh_medium, global_config_data.rh_high);

    latest_measurement.relative_time = now;
    latest_measurement.co2_ppm = co2_ppm;
    latest_measurement.rh = rh;
    latest_measurement.temp_c = temp_c;
    latest_measurement.sensor_status = sensor_status;
    latest_measurement.sequence_number = sequence_number;
    latest_measurement.state_at_this_time = get_highest_ventilation_state(current_ventilation_state_co2, current_ventilation_state_rh);

    very_short_data.pushOverwrite(latest_measurement);
    ++current_very_short_push_count;

    if (current_very_short_push_count == SENSOR_VERY_SHORT_MEASUREMENT_COUNT) {
        current_very_short_push_count = 0;

        get_average(very_short_data, _tmp_avg);
        short_data.pushOverwrite(_tmp_avg);
        ++current_short_push_count;
    }

    if (current_short_push_count == SENSOR_SHORT_MEASUREMENT_COUNT) {
        current_short_push_count = 0;

        get_average(short_data, _tmp_avg);
        long_data.pushOverwrite(_tmp_avg);
    }
}

bool device_data::is_associated() const {
    return id.length() > 0;
}

void device_data::associate(String identifier)
{
    id = identifier;
}

bool device_data::has_recent_data() const {
    long difference = (unsigned long)time(NULL) - latest_measurement.relative_time;
    const size_t seconds_timeout = SENSOR_SHORT_MEASUREMENT_PERIOD / 1000;

    return difference <= seconds_timeout;
}

requested_ventilation_state device_data::get_highest_ventilation_state(requested_ventilation_state a, requested_ventilation_state b) const {
    if (a > b) {
        return a;
    }

    return b;
}


requested_ventilation_state device_data::get_highest_ventilation_state() const {
    // if we didn't hear back from the sensor in 15m
    if (!has_recent_data()) {
        // !! safety !! - Occupants present = medium is required. Low is "no occupants".
        return requested_ventilation_state_medium;
    }

    return get_highest_ventilation_state(current_ventilation_state_co2, current_ventilation_state_rh);
}

String device_data::toString(int index) const {
    return
        String(index)                           + ",\t"
        + id                                    + ",\t"
        + String(current_ventilation_state_co2) + ",\t"
        + String(current_ventilation_state_rh)  + ",\t"
        + String(is_associated())               + ",\t"
        + String(has_recent_data())             + ",\t"
        + String(very_short_data.size())        + ",\t"
        + String(short_data.size())             + ",\t"
        + String(long_data.size())              + ",\t"
        + latest_measurement.toString();
}

String device_data::getHeaders() const {
    return
        String("device_index,\t"
        "device_id,\t"
        "current_ventilation_state_co2,\t"
        "current_ventilation_state_rh,\t"
        "is_associated,\t"
        "has_recent_data,\t"
        "very_short_count,\t"
        "short_count,\t"
        "long_count,\t")
        + latest_measurement.getHeaders();
}

String measurement_entry::toString() const
{
    return
        String(relative_time)           + ",\t"
        + String(co2_ppm)               + ",\t"
        + String(rh)                    + ",\t"
        + String(temp_c)                + ",\t"
        + String(sensor_status)         + ",\t"
        + String(sequence_number)       + ",\t"
        + String(state_at_this_time)    + "\n";
}

String measurement_entry::getHeaders() const {
    return
        "relative_time,\t"
        "co2_ppm,\t"
        "rh,\t"
        "temp_c,\t"
        "sensor_status,\t"
        "sequence_number,\t"
        "state_at_this_time\n";
}
