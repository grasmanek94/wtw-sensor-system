#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "FS.h"

#include "src/LittleFS.hpp"

void initWiFi() {
    WiFi.persistent(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(global_config_data.wifi_ssid, global_config_data.wifi_password);
    Serial.print("Connecting to WiFi '");
    Serial.print(global_config_data.wifi_ssid);
    Serial.print("' ..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }
    Serial.println(WiFi.localIP());
}

void setup() {
	Serial.begin(115200);
	Serial.println(F("Starting..."));

    if (littlefs_read_config()) {
        initWiFi();
    }

    if (global_config_data.interval < 1) {
        global_config_data.interval = 1;
    }
}

void send_measurements() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(global_config_data.destination_address);
        http.setAuthorization(global_config_data.auth_user.c_str(), global_config_data.auth_password.c_str());
        http.setTimeout(5);

        int co2_ppm = 0;
        float rel_hum_perc = 0.0f;

#ifdef CO2_SENSOR_ENABLED
        co2_ppm = last_measured_co2_ppm;
#endif
#ifdef RH_SENSOR_ENABLED
        rel_hum_perc = last_measured_rh_value;
#endif

        int httpResponseCode = http.POST(
            "deviceId=" + WiFi.macAddress() + 
            "&co2=" + co2_ppm +
            "&rh=" + rel_hum_perc + 
            "&deviceName=" + global_config_data.device_custom_name
        );

        if (Serial) {
            if (httpResponseCode > 0) {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                String payload = http.getString();
                Serial.println(payload);
            }
            else {
                Serial.print("Error code: ");
                Serial.println(httpResponseCode);
            }
        }

        http.end();
    }
}

const unsigned long max_wifi_timeout_until_reconnect = 15000;
unsigned long require_wifi_reconnect_time = 0;

void check_wifi() {
    unsigned long now = millis();

    bool connected = (WiFi.status() == WL_CONNECTED);
    bool wifi_reconnect_elapsed = (now > require_wifi_reconnect_time);
    if (connected || wifi_reconnect_elapsed) {
        require_wifi_reconnect_time = now + max_wifi_timeout_until_reconnect;

        if (!connected && wifi_reconnect_elapsed) {
            WiFi.disconnect();
            WiFi.reconnect();
        }
    }
}

unsigned long next_measurements_send_time = 0;

void check_measurements() {

    unsigned long now = millis();
    
    if (now > next_measurements_send_time) {
        next_measurements_send_time = now + (global_config_data.interval * 1000);

        send_measurements();
    }
}

void loop() {
    check_wifi(); 
    check_measurements();
}
