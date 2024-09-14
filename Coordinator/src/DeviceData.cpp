#include "RingBuf.h"
#include "LittleFS.hpp"

#include "DeviceData.hpp"
#include "HumidityOffset.hpp"
#include "MeterStatus.hpp"
#include "VentilationState.hpp"

#include <time.h>

#include <array>

std::array<device_data,SENSORS_COUNT> sensors;

device_data::device_data() :
    id{ "" },
    current_very_short_push_count{ 0 },
    current_short_push_count{ 0 },
    short_data{},
    long_data{},
    // !! safety !! - Occupants present = medium is required. Low is "no occupants".
    current_ventilation_state_co2{ requested_ventilation_state_medium },
    current_ventilation_state_rh{ requested_ventilation_state_medium },
    latest_measurement{},
    average_measurement{},
    _tmp_avg{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    calculated_co2_ppm_low{0},
    calculated_co2_ppm_medium{0},
    calculated_co2_ppm_high{0}
{}

static void get_average(RingBufInterface<measurement_entry>* container, measurement_entry_avg& tmp_avg, measurement_entry& avg, unsigned long time_span) {
    const size_t size = container->size();
    
    avg.set_co2(0);
    avg.set_rh(0.0f);
    avg.set_attainable_rh(0.0f);
    avg.set_temp(0.0f);
    avg.sensor_status = 0;
    avg.sequence_number = 0;

    avg.set_state_at_this_time(requested_ventilation_state_low);

    tmp_avg.co2_ppm = 0.0f;
    tmp_avg.relative_humidity = 0.0f;
    tmp_avg.attainable_humidity = 0.0f;
    tmp_avg.state_at_this_time = 0.0f;
    tmp_avg.temperature_c = 0.0f;
    tmp_avg.ventilation_aggressiveness = 0.0f;
     
    if (size > 0) {
        // initial value better be not 0 because then it will always stay at 0
        avg.sequence_number = container->at(0).sequence_number;
        avg.relative_time = container->at(0).relative_time;
    }

    for (int i = 0; i < size; ++i) {
        tmp_avg.co2_ppm += (float)container->at(i).get_co2();
        tmp_avg.relative_humidity += (float)container->at(i).get_rh();
        tmp_avg.attainable_humidity += (float)container->at(i).get_attainable_rh();
        tmp_avg.temperature_c += (float)container->at(i).get_temp();
        tmp_avg.ventilation_aggressiveness += (float)container->at(i).get_ventilation_aggressiveness();

        MeterStatusUnion meter_status;
        MeterStatusUnion meter_status_avg;

        meter_status.combined = container->at(i).sensor_status;
        meter_status_avg.combined = avg.sensor_status;

        meter_status_avg.split.meter_status |= meter_status.split.meter_status;
        meter_status_avg.split.calibration_status = max(meter_status_avg.split.calibration_status, meter_status.split.calibration_status);
        meter_status_avg.split.abc_status = max(meter_status_avg.split.abc_status, meter_status.split.abc_status);
        
        avg.sensor_status = meter_status_avg.combined;
        
        tmp_avg.state_at_this_time += (float)container->at(i).get_state_at_this_time();

        // use min instead of max so that we don't get time traveling plots
        // when max then possible that short data is overlapping with long data etc
        avg.sequence_number = max(avg.sequence_number, container->at(i).sequence_number);
        avg.relative_time = min(avg.relative_time, container->at(i).relative_time);
    }

    avg.set_state_at_this_time((requested_ventilation_state)(int)round(tmp_avg.state_at_this_time / (float)size));
    avg.set_co2(tmp_avg.co2_ppm / (float)size);
    avg.set_rh(tmp_avg.relative_humidity / (float)size);
    avg.set_attainable_rh(tmp_avg.attainable_humidity / (float)size);
    avg.set_temp(tmp_avg.temperature_c / (float)size);
    avg.set_ventilation_aggressiveness(round(tmp_avg.ventilation_aggressiveness / (float)size));
}

void device_data::push(int co2_ppm, float rh, float temp_c, int sensor_status, unsigned long sequence_number) {
    unsigned long now = (unsigned long)time(NULL);
    const float ERROR_PROBABLY_TEMP_MIN = -30.0f;
    const float ERROR_PROBABLY_TEMP_MAX = 60.0f;
    const int DEFAULT_VENTILATION_AGGRESSIVENESS = 0;

    latest_measurement.relative_time = now;
    latest_measurement.set_co2(co2_ppm);
    latest_measurement.set_rh(rh);
    latest_measurement.set_temp(temp_c);
    latest_measurement.sensor_status = sensor_status;
    latest_measurement.sequence_number = sequence_number;
    latest_measurement.set_ventilation_aggressiveness(DEFAULT_VENTILATION_AGGRESSIVENESS);

    if (loc != SENSOR_LOCATION::NEW_AIR_INLET) {
        float average_temp_inside_for_co2 = 0.0f;
        int average_temps_measured_for_co2 = 0;

        if(global_config_data.use_average_temp_for_co2) {
            for(const auto& sensor: sensors) {
                if(sensor.is_associated() && sensor.has_recent_data()) {
                    if(sensor.loc == SENSOR_LOCATION::NEW_AIR_INLET) {
                        continue;
                    } 

                    if(sensor.latest_measurement.get_co2() != 0) {
                        continue;
                    }

                    if(sensor.latest_measurement.get_temp() > ERROR_PROBABLY_TEMP_MAX) {
                        continue;
                    }

                    if(sensor.latest_measurement.get_temp() < ERROR_PROBABLY_TEMP_MIN) {
                        continue;
                    }

                    ++average_temps_measured_for_co2;
                    average_temp_inside_for_co2 += sensor.latest_measurement.get_temp();
                }
            }
        }

        if(average_temps_measured_for_co2 != 0) {
            average_temp_inside_for_co2 /= (float)average_temps_measured_for_co2;
        } else {
            average_temp_inside_for_co2 = temp_c;
        }

        bool use_full_rh_calculation = true;
        float measured_inlet_temp = global_config_data.temp_setpoint_c;
        const auto& outside_sensor = sensors[(int)SENSOR_LOCATION::NEW_AIR_INLET];
        bool has_air_inlet_data = outside_sensor.is_associated() && outside_sensor.has_recent_data();

        if (has_air_inlet_data) {
            measured_inlet_temp = outside_sensor.latest_measurement.get_temp();
        }

        if (global_config_data.use_rh_headroom_mode) {
            // Here will be calculated if it is possible to attain a 'better' relative humidity inside, 
            // i.e. if it makes any sense to ventilate more to attain a lower RH.
            // this allows us to save energy and not ventilate when not needed.
            // For example, if outside if 15 *C 95% RH, and inside it's 22 *C, the best attainable RH is ~61%
            // If it's already <= 61% inside, then ventilating more won't bring it below 61%.. 
            // It's better to not waste energy in such a case

            if (has_air_inlet_data) {
                const auto& outside_measurement = outside_sensor.latest_measurement;

                latest_measurement.set_attainable_rh(offset_relative_humidity(outside_measurement.get_rh(), outside_measurement.get_temp(), temp_c));
                float relative_humidity_headroom = max(rh - latest_measurement.get_attainable_rh(), 0.0f);

                use_full_rh_calculation = false;
                current_ventilation_state_rh = determine_current_ventilation_state(current_ventilation_state_rh, relative_humidity_headroom, global_config_data.rh_attainable_headroom_low, global_config_data.rh_attainable_headroom_medium, global_config_data.rh_attainable_headroom_high);
            }

            // let's say we can attain a RH difference of 30%.. going down to 20% (e.g. in winter times).
            // But we're already between 40-60%, then it's wasteful of energy, and bad for health to ventilate RH until it's below 40%.
            // So allow some configuration to cut off higher ventilation states to some lowerbound RH value.
            if (current_ventilation_state_rh >= requested_ventilation_state_high && rh < global_config_data.rh_headroom_mode_rh_medium_bound) {
                current_ventilation_state_rh = requested_ventilation_state_medium;
            }

            if (current_ventilation_state_rh >= requested_ventilation_state_medium && rh < global_config_data.rh_headroom_mode_rh_low_bound) {
                current_ventilation_state_rh = requested_ventilation_state_low;
            }
        }

        float ventilation_aggressiveness = 0.0f;
        const float convert_to_numeric_state = 16.0f / 2.0f;
        const auto& co2_data = global_config_data.get_co2_ppm_data(average_temp_inside_for_co2, measured_inlet_temp, ventilation_aggressiveness);
        latest_measurement.set_ventilation_aggressiveness((int)(ventilation_aggressiveness * convert_to_numeric_state));
        calculated_co2_ppm_low = co2_data.low;
        calculated_co2_ppm_medium = co2_data.medium;
        calculated_co2_ppm_high = co2_data.high;
        
        current_ventilation_state_co2 = determine_current_ventilation_state(current_ventilation_state_co2, co2_ppm, co2_data.low, co2_data.medium, co2_data.high);

        if (use_full_rh_calculation) {
            latest_measurement.set_attainable_rh(0.0f);
            current_ventilation_state_rh = determine_current_ventilation_state(current_ventilation_state_rh, rh, global_config_data.rh_low, global_config_data.rh_medium, global_config_data.rh_high);
        }
    } else {
        latest_measurement.set_attainable_rh(rh);

        // don't let ventilation state be determined by outside values...
        current_ventilation_state_co2 = requested_ventilation_state_low; // altough I don't expect anyone to connect a co2 sensor to the outside
        current_ventilation_state_rh = requested_ventilation_state_low;
    }

    latest_measurement.set_state_at_this_time(get_highest_ventilation_state(current_ventilation_state_co2, current_ventilation_state_rh));

    very_short_data.pushOverwrite(latest_measurement);
    ++current_very_short_push_count;

    if (current_very_short_push_count == SENSOR_VERY_SHORT_MEASUREMENT_COUNT) {
        current_very_short_push_count = 0;

        get_average(&very_short_data, _tmp_avg, average_measurement, SENSOR_VERY_SHORT_MEASUREMENT_PERIOD);
        short_data.pushOverwrite(average_measurement);
        ++current_short_push_count;
    }

    if (current_short_push_count == SENSOR_SHORT_MEASUREMENT_COUNT) {
        current_short_push_count = 0;

        get_average(&short_data, _tmp_avg, average_measurement, SENSOR_SHORT_MEASUREMENT_PERIOD);
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
        + String(calculated_co2_ppm_low)        + ",\t"
        + String(calculated_co2_ppm_medium)     + ",\t"
        + String(calculated_co2_ppm_high)       + ",\t"
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
        "long_count,\t"
        "calculated_co2_ppm_low,\t"
        "calculated_co2_ppm_medium,\t"
        "calculated_co2_ppm_high,\t")
        + latest_measurement.getHeaders();
}
