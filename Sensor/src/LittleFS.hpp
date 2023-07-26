#pragma once

/* IMPORTANT
  This code uses the built in LittleFS library in the ESP32 library. Please verify that the ESP32 library is R2.0.4 or newer for
  this code to run. To do so, go to Tools -> Board -> Boards Manager and search for "ESP32". You should see the esp32 library from
  Espressif Systems. Make sure the version is 2.0.4 or newer. */

#include "FS.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

struct global_config {
    String wifi_ssid;
    String wifi_password;
    String destination_address;
    String auth_user;
    String auth_password;
    int interval;
    int manual_calibration_performed;
    float temp_offset_x;
    float temp_offset_y;
};

// define filename to store config file
const String global_config_filename = "/config.json";

extern global_config global_config_data;
bool littlefs_read_config();
void littlefs_write_config();