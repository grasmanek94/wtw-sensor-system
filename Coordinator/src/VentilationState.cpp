#include "VentilationState.hpp"
#include "DeviceData.hpp"
#include "LittleFS.hpp"

#include <HTTPClient.h>
#include <UrlEncode.h>
#include <WiFi.h>

static requested_ventilation_state old_ventilation_state = requested_ventilation_state_undefined;
static unsigned long last_send_time = 0;

static requested_ventilation_state get_highest_ventilation_state() {
    // keep in mind that this data may be changed by another thread..
    // but consindering it's just a few bools/ints and the sensors array doesn't 
    // get modified with new entries there should be no real race condition 
    // (I mean, there WILL be a race condition but the negative impact is 'almost' zero)
    // best case a too high ventilation state will be selected
    // worst case medium ventilation state by error..
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
