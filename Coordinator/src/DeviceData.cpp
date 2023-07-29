#include "RingBuf.h"
#include "LittleFS.hpp"

#include "VentilationState.hpp"
#include "DeviceData.hpp"
#include "MeterStatus.hpp"

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
    latest_measurement{ 0, 0, 0, 0, 0, requested_ventilation_state_undefined, 0 },
    average_measurement{ 0, 0, 0, 0, 0, requested_ventilation_state_undefined, 0 },
    _tmp_avg{ 0.0f, 0.0f, 0.0f, 0.0f }
{}

void get_average(RingBufInterface<measurement_entry>* container, measurement_entry_avg& tmp_avg, measurement_entry& avg) {
    const size_t size = container->size();
    
    avg.set_co2(0);
    avg.set_rh(0.0f);
    avg.set_temp(0.0f);
    avg.sensor_status = 0;
    avg.sequence_number = 0;

    avg.state_at_this_time = requested_ventilation_state_low;

    tmp_avg.co2_ppm = 0.0f;
    tmp_avg.relative_humidity = 0.0f;
    tmp_avg.state_at_this_time = 0.0f;
    tmp_avg.temperature_c = 0.0f;

    for (int i = 0; i < size; ++i) {
        tmp_avg.co2_ppm += (float)container->at(i).get_co2();
        tmp_avg.relative_humidity += (float)container->at(i).get_rh();
        tmp_avg.temperature_c += (float)container->at(i).get_temp();

        MeterStatusUnion meter_status;
        MeterStatusUnion meter_status_avg;

        meter_status.combined = container->at(i).sensor_status;
        meter_status_avg.combined = avg.sensor_status;

        meter_status_avg.split.meter_status |= meter_status.split.meter_status;
        meter_status_avg.split.calibration_status = max(meter_status_avg.split.calibration_status, meter_status.split.calibration_status);
        meter_status_avg.split.abc_status = max(meter_status_avg.split.abc_status, meter_status.split.abc_status);
        
        avg.sensor_status = meter_status_avg.combined;
        
        tmp_avg.state_at_this_time += (float)container->at(i).state_at_this_time;

        // oldest time = time of all averages (basically when measurement began)
        // when using newest times, graph plot lines like to time travel
        avg.relative_time = min(avg.relative_time, container->at(i).relative_time);
        avg.sequence_number = min(avg.sequence_number, container->at(i).sequence_number);
    }

    avg.state_at_this_time = (requested_ventilation_state)(int)round(tmp_avg.state_at_this_time /= (float)size);
    avg.set_co2(tmp_avg.co2_ppm / (float)size);
    avg.set_rh(tmp_avg.relative_humidity / (float)size);
    avg.set_temp(tmp_avg.temperature_c / (float)size);
}

void device_data::push(int co2_ppm, float rh, float temp_c, int sensor_status, unsigned long sequence_number) {
    unsigned long now = (unsigned long)time(NULL);

    current_ventilation_state_co2 = determine_current_ventilation_state(current_ventilation_state_co2, co2_ppm, global_config_data.co2_ppm_low, global_config_data.co2_ppm_medium, global_config_data.co2_ppm_high);
    current_ventilation_state_rh = determine_current_ventilation_state(current_ventilation_state_rh, rh, global_config_data.rh_low, global_config_data.rh_medium, global_config_data.rh_high);

    latest_measurement.relative_time = now;
    latest_measurement.set_co2(co2_ppm);
    latest_measurement.set_rh(rh);
    latest_measurement.set_temp(temp_c);
    latest_measurement.sensor_status = sensor_status;
    latest_measurement.sequence_number = sequence_number;
    latest_measurement.state_at_this_time = get_highest_ventilation_state(current_ventilation_state_co2, current_ventilation_state_rh);

    very_short_data.pushOverwrite(latest_measurement);
    ++current_very_short_push_count;

    if (current_very_short_push_count == SENSOR_VERY_SHORT_MEASUREMENT_COUNT) {
        current_very_short_push_count = 0;

        get_average(&very_short_data, _tmp_avg, average_measurement);
        short_data.pushOverwrite(average_measurement);
        ++current_short_push_count;
    }

    if (current_short_push_count == SENSOR_SHORT_MEASUREMENT_COUNT) {
        current_short_push_count = 0;

        get_average(&short_data, _tmp_avg, average_measurement);
        long_data.pushOverwrite(average_measurement);
    }
}

bool device_data::is_associated() const {
    return id.length() > 0;
}

void device_data::associate(String identifier, SENSOR_LOCATION location)
{
    id = identifier;
    loc = location;
}

bool device_data::has_recent_data() const {
    long difference = (unsigned long)time(NULL) - latest_measurement.relative_time;
    const size_t seconds_timeout = SENSOR_SHORT_MEASUREMENT_PERIOD / 1000;

    return difference <= seconds_timeout;
}

requested_ventilation_state device_data::get_highest_ventilation_state(requested_ventilation_state a, requested_ventilation_state b) const {
    return max(a, b);
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
        String("sensor_location_id,\t"
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
        + String(get_co2())             + ",\t"
        + String(get_rh())              + ",\t"
        + String(get_temp())            + ",\t"
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
