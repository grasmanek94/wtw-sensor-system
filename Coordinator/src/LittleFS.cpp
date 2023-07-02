#include "LittleFS.hpp"

#include "FS.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

  /* You only need to format LITTLEFS the first time you run a
     test or else use the LITTLEFS plugin to create a partition
     https://github.com/lorol/arduino-esp32littlefs-plugin */

global_config global_config_data;
const size_t max_document_len = 768;

static void writeFile(String filename, String message) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.println("writeFile -> failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("File written");
    }
    else {
        Serial.println("Write failed");
    }
    file.close();
}

static String readFile(String filename) {
    File file = LittleFS.open(filename);
    if (!file) {
        Serial.println("Failed to open file for reading, size: ");
        Serial.println(file.size());
        return "";
    }

    Serial.println("File open OK, size: ");
    Serial.println(file.size());

    String fileText = "";
    while (file.available()) {
        fileText += file.readString();
    }
    file.close();
    Serial.println(file.size());
    return fileText;
}

static bool readConfig() {

    String file_content = readFile(global_config_filename);

    int config_file_size = file_content.length();
    Serial.println("Config file size: " + String(config_file_size));

    if (config_file_size > max_document_len) {
        Serial.println("Config file too large");
        return false;
    }

    StaticJsonDocument<max_document_len> doc;
    auto error = deserializeJson(doc, file_content);
    if (error) {
        Serial.println("Error interpreting config file");
        Serial.println(error.c_str());
        return false;
    }

    const String wifi_ssid = doc["wifi_id"];
    const String wifi_password = doc["wifi_pw"];
    const String device_custom_hostname = doc["hostname"];
    const String destination_address = doc["destination"];
    const String auth_user = doc["auth_user"];
    const String auth_password = doc["auth_pw"];
    const int interval = doc["interval"];
    const int co2_ppm_high = doc["co2_ppm_high"];
    const int co2_ppm_medium = doc["co2_ppm_medium"];
    const int co2_ppm_low = doc["co2_ppm_low"];
    const float rh_high = doc["rh_high"];
    const float rh_medium = doc["rh_medium"];
    const float rh_low = doc["rh_low"];

    global_config_data.wifi_ssid = wifi_ssid;
    global_config_data.wifi_password = wifi_password;
    global_config_data.device_custom_hostname = device_custom_hostname;
    global_config_data.destination_address = destination_address;
    global_config_data.auth_user = auth_user;
    global_config_data.auth_password = auth_password;
    global_config_data.interval = interval;
    global_config_data.co2_ppm_high = co2_ppm_high;
    global_config_data.co2_ppm_medium = co2_ppm_medium;
    global_config_data.co2_ppm_low = co2_ppm_low;
    global_config_data.rh_high = rh_high;
    global_config_data.rh_medium = rh_medium;
    global_config_data.rh_low = rh_low;

    return true;
}

static bool saveConfig() {
    StaticJsonDocument<max_document_len> doc;

    // write variables to JSON file
    doc["wifi_id"] = global_config_data.wifi_ssid;
    doc["wifi_pw"] = global_config_data.wifi_password;
    doc["hostname"] = global_config_data.device_custom_hostname;
    doc["destination"] = global_config_data.destination_address;
    doc["auth_user"] = global_config_data.auth_user;
    doc["auth_pw"] = global_config_data.auth_password;
    doc["interval"] = global_config_data.interval;
    doc["co2_ppm_high"] = global_config_data.co2_ppm_high;
    doc["co2_ppm_medium"] = global_config_data.co2_ppm_medium;
    doc["co2_ppm_low"] = global_config_data.co2_ppm_low;
    doc["rh_high"] = global_config_data.rh_high;
    doc["rh_medium"] = global_config_data.rh_medium;
    doc["rh_low"] = global_config_data.rh_low ;

    // write config file
    String tmp = "";
    serializeJson(doc, tmp);
    writeFile(global_config_filename, tmp);

    return true;
}

bool littlefs_read_config() {
    // Mount LITTLEFS and read in config file
    if (!LittleFS.begin(false)) {
        Serial.println("LITTLEFS Mount Failed - Formatting...");
        if (!LittleFS.begin(true)) {
            Serial.println("LITTLEFS Mount Failed - Format Failed");
        }
        else {
            Serial.println("LITTLEFS Mount OK - Format Success");
        }
    }
    else {
        Serial.println("LITTLEFS Mount OK");
        Serial.print("Storage bytes used/total: ");
        Serial.print(LittleFS.usedBytes());
        Serial.print('/');
        Serial.println(LittleFS.totalBytes());

        if (readConfig() == false) {
            Serial.println("LITTLEFS Config Load Failed");
        }
        else {
            Serial.println("LITTLEFS Config Load OK");
            return true;
        }
    }

    return false;
}
