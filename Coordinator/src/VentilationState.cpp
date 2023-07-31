#include "VentilationState.hpp"
#include "DeviceData.hpp"
#include "HumidityOffset.hpp"
#include "LittleFS.hpp"

#include <HTTPClient.h>
#include <UrlEncode.h>
#include <WiFi.h>

struct device_relative_humidity_calculations {
    float attainable_relative_humidity;
    float relative_humidity_headroom;

    device_relative_humidity_calculations() { reset(); }

    void reset() {
        attainable_relative_humidity = 0.0f;
        relative_humidity_headroom = 0.0f;
    }
};

struct relative_humidity_headroom_data {
    float current_outside_temp;
    float current_outside_rh;
    float max_rh_headroom;
    device_relative_humidity_calculations rh_calculations[SENSORS_COUNT];

    relative_humidity_headroom_data() {
        reset();
    }

    void reset() {
        current_outside_temp = 0.0f;
        current_outside_rh = 0.0f;
        max_rh_headroom = 0.0f;
        for (auto& i : rh_calculations) {
            i.reset();
        }
    }
};

static requested_ventilation_state old_ventilation_state = requested_ventilation_state_undefined;
static unsigned long last_send_time = 0;
static relative_humidity_headroom_data rh_headroom;

static requested_ventilation_state get_highest_ventilation_state() {
    // keep in mind that this data may be changed by another thread..
    // but consindering it's just a few bools/ints and the sensors array doesn't 
    // get modified with new entries there should be no real race condition 
    // (I mean, there WILL be a race condition but the negative impact is 'almost' zero)
    // best case a too high ventilation state will be selected
    // worst case medium ventilation state by error..


    // Here will be calculated if it is possible to attain a 'better' relative humidity inside, 
    // i.e. if it makes any sense to ventilate more to attain a lower RH.
    // this allows us to save energy and not ventilate when not needed.
    // For example, if outside if 15 *C 95% RH, and inside it's 22 *C, the best attainable RH is ~61%
    // If it's already <= 61% inside, then ventilating more won't bring it below 61%.. 
    // It's better to not waste energy in such a case
    if (global_config_data.use_rh_headroom_mode) {
        const auto& outside_sensor = sensors[(int)SENSOR_LOCATION::NEW_AIR_INLET];

        if (outside_sensor.is_associated()) {
            rh_headroom.reset();

            const auto& outside_measurement = outside_sensor.latest_measurement;
            rh_headroom.current_outside_temp = outside_measurement.get_temp();
            rh_headroom.current_outside_rh = outside_measurement.get_rh();

            for (int i = 0; i < SENSORS_COUNT; ++i) {
                const auto& sensor = sensors[i];
                const auto& measurement = sensor.latest_measurement;
                auto& humidity_entry = rh_headroom.rh_calculations[i];

                if (i == (int)SENSOR_LOCATION::NEW_AIR_INLET) {
                    humidity_entry.attainable_relative_humidity = rh_headroom.current_outside_rh;
                    humidity_entry.relative_humidity_headroom = 0.0f;
                    continue;
                }

                if (sensor.is_associated()) {
                    humidity_entry.attainable_relative_humidity = offset_relative_humidity(rh_headroom.current_outside_rh, rh_headroom.current_outside_temp, measurement.get_temp());
                }
            }
        }
    }

    requested_ventilation_state state = requested_ventilation_state_low;


    bool has_any_associated_devices = false;
    for (const auto& sensor : sensors) {
        if (sensor.is_associated()) {
            has_any_associated_devices = true;
            state = max(sensor.get_highest_ventilation_state(), state);
        }
    }

    // !! safety !! - Occupants present = medium is required. Low is "no occupants".
    if (!has_any_associated_devices) {
        state = requested_ventilation_state_medium;
    }

    return state;
}

bool send_ventilation_status() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    String URL = "http://" + global_config_data.destination_address + "/api.html"
        "?username=" + urlEncode(global_config_data.auth_user) +
        "&password=" + urlEncode(global_config_data.auth_password) +
        "&vremotecmd=";

    switch (get_highest_ventilation_state()) {
    case requested_ventilation_state_high:
        URL += "high";
        break;

    case requested_ventilation_state_medium:
        URL += "medium";
        break;

    case requested_ventilation_state_low:
        URL += "low";
        break;

    default:
        // !! safety !! - Occupants present = medium is required. Low is "no occupants".
        URL += "medium";
        break;
    }

    http.begin(URL);
    //http.setAuthorization(global_config_data.auth_user.c_str(), global_config_data.auth_password.c_str());
    http.setTimeout(max_ventilation_status_http_request_timeout_s);

    int httpResponseCode = http.GET();

    if (Serial) {
        Serial.println(URL);
        if (httpResponseCode > 0) {
            Serial.print("send_ventilation_status HTTP Response code: ");
            Serial.println(httpResponseCode);
            String payload = http.getString();
            Serial.println(payload);
        }
        else {
            Serial.print("send_ventilation_status Error code: ");
            Serial.println(httpResponseCode);
        }
    }

    http.end();

    return (httpResponseCode == 200);
}

void check_measurements() {
    unsigned long now = millis();
    unsigned long interval = global_config_data.interval * 1000;

    if ((now - last_send_time) > interval) {
        interval = now;

        requested_ventilation_state new_state = get_highest_ventilation_state();
        if (old_ventilation_state != new_state) {
            if (send_ventilation_status()) {
                old_ventilation_state = new_state;
            }
        }
    }
}
