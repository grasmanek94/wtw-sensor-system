#include "LittleFS.hpp"

#include "FS.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

/* You only need to format LITTLEFS the first time you run a
   test or else use the LITTLEFS plugin to create a partition
   https://github.com/lorol/arduino-esp32littlefs-plugin */

global_config global_config_data;
const size_t max_document_len = 2048;
static StaticJsonDocument<max_document_len> doc;

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

String global_config::get_device_custom_hostname() const {
	return doc["hostname"].as<String>();
}

void global_config::set_device_custom_hostname(const String& device_custom_hostname) {
	doc["hostname"] = device_custom_hostname;
}

String global_config::get_static_ip() const {
	return doc["static_ip"].as<String>();
}
void global_config::set_static_ip(const String& static_ip) {
	doc["static_ip"] = static_ip;
}

String global_config::get_gateway_ip() const {
	return doc["gateway_ip"].as<String>();
}
void global_config::set_gateway_ip(const String& gateway_ip) {
	doc["gateway_ip"] = gateway_ip;
}

String global_config::get_subnet() const {
	return doc["subnet"].as<String>();
}
void global_config::set_subnet(const String& subnet) {
	doc["subnet"] = subnet;
}

String global_config::get_primary_dns() const {
	return doc["primary_dns"].as<String>();
}
void global_config::set_primary_dns(const String& primary_dns) {
	doc["primary_dns"] = primary_dns;
}

String global_config::get_secondary_dns() const {
	return doc["secondary_dns"].as<String>();
}
void global_config::set_secondary_dns(const String& secondary_dns) {
	doc["secondary_dns"] = secondary_dns;
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
	global_config_data.co2_ppm_high = doc["co2_ppm_high"].as<int>();
	global_config_data.co2_ppm_medium = doc["co2_ppm_medium"].as<int>();
	global_config_data.co2_ppm_low = doc["co2_ppm_low"].as<int>();
	global_config_data.rh_high = doc["rh_high"].as<float>();
	global_config_data.rh_medium = doc["rh_medium"].as<float>();
	global_config_data.rh_low = doc["rh_low"].as<float>();

	// added in v2.1
	global_config_data.use_rh_headroom_mode =
		doc.containsKey("use_rh_headroom_mode") ?
		doc["use_rh_headroom_mode"].as<bool>() :
		false;

	global_config_data.rh_attainable_headroom_high = 
		doc.containsKey("rh_attainable_headroom_high") ? 
		doc["rh_attainable_headroom_high"].as<float>() : 
		40.0f;

	global_config_data.rh_attainable_headroom_medium =
		doc.containsKey("rh_attainable_headroom_medium") ?
		doc["rh_attainable_headroom_medium"].as<float>() :
		25.0f;

	global_config_data.rh_attainable_headroom_low =
		doc.containsKey("rh_attainable_headroom_low") ?
		doc["rh_attainable_headroom_low"].as<float>() :
		10.0f;

	return true;
}

static bool saveConfig() {
	// write variables to JSON file
	doc["destination"] = global_config_data.destination_address;
	doc["auth_user"] = global_config_data.auth_user;
	doc["auth_pw"] = global_config_data.auth_password;
	doc["interval"] = global_config_data.interval;
	doc["co2_ppm_high"] = global_config_data.co2_ppm_high;
	doc["co2_ppm_medium"] = global_config_data.co2_ppm_medium;
	doc["co2_ppm_low"] = global_config_data.co2_ppm_low;
	doc["rh_high"] = global_config_data.rh_high;
	doc["rh_medium"] = global_config_data.rh_medium;
	doc["rh_low"] = global_config_data.rh_low;
	doc["use_rh_headroom_mode"] = global_config_data.use_rh_headroom_mode;
	doc["rh_attainable_headroom_high"] = global_config_data.rh_attainable_headroom_high;
	doc["rh_attainable_headroom_medium"] = global_config_data.rh_attainable_headroom_medium;
	doc["rh_attainable_headroom_low"] = global_config_data.rh_attainable_headroom_low;

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

void littlefs_write_config() {
	saveConfig();
}