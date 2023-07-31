#pragma once

/* IMPORTANT
  This code uses the built in LittleFS library in the ESP32 library. Please verify that the ESP32 library is R2.0.4 or newer for
  this code to run. To do so, go to Tools -> Board -> Boards Manager and search for "ESP32". You should see the esp32 library from
  Espressif Systems. Make sure the version is 2.0.4 or newer. */

#include "FS.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <functional>

struct global_config {
    String get_wifi_ssid() const;
    void set_wifi_ssid(const String& wifi_ssid);

    String get_wifi_password() const;
    void set_wifi_password(const String& wifi_password);

    String get_sensors() const;
    void set_sensors(const String& sensors);

    // frequently used stuff = cached
    String destination_address;
    String auth_user;
    String auth_password;
    int interval;
    bool manual_calibration_performed;
    float temp_offset_x;
    float temp_offset_y;
};

// define filename to store config file
const String global_config_filename = "/config.json";
using sensor_entry_callback = std::function<void(const String& type, const JsonVariant& value)>;
extern global_config global_config_data;

bool littlefs_read_config(const sensor_entry_callback& switcher = [](const String& type, const JsonVariant& value) {});
void littlefs_write_config();
