#pragma once

/* IMPORTANT
  This code uses the built in LittleFS library in the ESP32 library. Please verify that the ESP32 library is R2.0.4 or newer for
  this code to run. To do so, go to Tools -> Board -> Boards Manager and search for "ESP32". You should see the esp32 library from
  Espressif Systems. Make sure the version is 2.0.4 or newer. */

#include "FS.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

const int CO2_STATES_COUNT = 9;
const int CO2_MATRIX_SIDE_LENGTH = 21;

struct global_config {

    String get_wifi_ssid() const;
    void set_wifi_ssid(const String& wifi_ssid);

    String get_wifi_password() const;
    void set_wifi_password(const String& wifi_password);

    String get_device_custom_hostname() const;
    void set_device_custom_hostname(const String& device_custom_hostname);

    String get_static_ip() const;
    void set_static_ip(const String& static_ip);

    String get_gateway_ip() const;
    void set_gateway_ip(const String& gateway_ip);

    String get_subnet() const;
    void set_subnet(const String& subnet);

    String get_primary_dns() const;
    void set_primary_dns(const String& primary_dns);

    String get_secondary_dns() const;
    void set_secondary_dns(const String& secondary_dns);

    // frequently used stuff = cached
    String destination_address;
    String auth_user;
    String auth_password;
    int interval;

    float rh_high;
    float rh_medium;
    float rh_low;

    // added in v2.1
    bool use_rh_headroom_mode; // requires presence of outside/inlet temp & RH sensor (mapped to location SENSOR_LOCATION::NEW_AIR_INLET)
    float rh_attainable_headroom_high;
    float rh_attainable_headroom_medium;
    float rh_attainable_headroom_low;
    // added in v2.4
    float rh_headroom_mode_rh_medium_bound;
    float rh_headroom_mode_rh_low_bound;

    // added in v2.5
    bool use_gps_time;

    int get_gps_time_uart_nr() const;
    void set_gps_time_uart_nr(int gps_time_uart_nr);

    int get_gps_baud() const;
    void set_gps_baud(int baud);

    // added in v2.6
    struct co2_ppm_state_s {
        int high;
        int medium;
        int low;
    };

    int current_selected_co2_state;
    co2_ppm_state_s co2_states[CO2_STATES_COUNT];
    float temp_setpoint_c;
    int8_t co2_matrix[CO2_MATRIX_SIDE_LENGTH][CO2_MATRIX_SIDE_LENGTH];
    bool use_average_temp_for_co2;

    const co2_ppm_state_s& get_co2_ppm_data(float measured_temp, float air_inlet_temp, int& co2_state_matrix_entry) const;
};

// define filename to store config file
const String global_config_filename = "/config.json";
extern global_config global_config_data;

bool littlefs_read_config();
void littlefs_write_config();
