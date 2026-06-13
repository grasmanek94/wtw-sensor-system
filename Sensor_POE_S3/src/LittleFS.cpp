#include "LittleFS.hpp"

#include "FS.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

  /* You only need to format LITTLEFS the first time you run a
     test or else use the LITTLEFS plugin to create a partition
     https://github.com/lorol/arduino-esp32littlefs-plugin */

global_config global_config_data;
static JsonDocument doc;

String global_config::get_wifi_ssid() const {
    return doc["wifi_id"].as<String>();
}

void global_config::set_wifi_ssid(const String& wifi_ssid) {
    doc["wifi_id"] = wifi_ssid;
}

String global_config::get_wifi_password() const {
    return doc["wifi_pw"].as<String>();
}

void global_config::set_wifi_password(const String& wifi_password) {
    doc["wifi_pw"] = wifi_password;
}

String global_config::get_sensors() const {
    String result;
    serializeJsonPretty(doc["sensors"], result);
    return result;
}

void global_config::set_sensors(const String& sensors) {
    JsonDocument sensors_json{};

    deserializeJson(sensors_json, sensors);
    doc["sensors"] = sensors_json;
}

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

    Serial.println(fileText);
    file.close();
    
    return fileText;
}

static bool readConfig(const sensor_entry_callback& switcher) {

    String file_content = readFile(global_config_filename);

    int config_file_size = file_content.length();
    Serial.println("Config file size: " + String(config_file_size));

    auto error = deserializeJson(doc, file_content);
    if (error) {
        Serial.println("Error interpreting config file");
        Serial.println(error.c_str());
        return false;
    }

    global_config_data.destination_address = doc["destination"].as<String>();
    global_config_data.auth_user = doc["auth_user"].as<String>();
    global_config_data.auth_password = doc["auth_pw"].as<String>();
    global_config_data.interval = doc["interval"].as<int>();
    global_config_data.manual_calibration_performed = doc["manual_calibration_performed"].as<bool>();

    for (JsonVariant value : doc["sensors"].as<JsonArray>()) {
        switcher(value["type"].as<String>(), value);
    }

    return true;
}

static bool saveConfig() {
    // write variables to JSON file
    doc["destination"] = global_config_data.destination_address;
    doc["auth_user"] = global_config_data.auth_user;
    doc["auth_pw"] = global_config_data.auth_password;
    doc["interval"] = global_config_data.interval;
    doc["manual_calibration_performed"] = global_config_data.manual_calibration_performed;

    // write config file
    String tmp = "";
    serializeJson(doc, tmp);
    writeFile(global_config_filename, tmp);

    return true;
}

bool littlefs_read_config(const sensor_entry_callback& switcher) {
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

        if (readConfig(switcher) == false) {
            Serial.println("LITTLEFS Config Load Failed");
        }
        else {
            Serial.println("LITTLEFS Config Load OK");
            return true;
        }
    }

    return false;
}

void littlefs_write_config() {
    saveConfig();
}
