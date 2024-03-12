#include "LittleFS.hpp"

#include "FS.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

/* You only need to format LITTLEFS the first time you run a
   test or else use the LITTLEFS plugin to create a partition
   https://github.com/lorol/arduino-esp32littlefs-plugin */

global_config global_config_data;
static JsonDocument doc;

template<typename T>
static T get_or_default(const JsonVariant& json, const char* key, T default_value) {
	if (json.containsKey(key)) {
		return json[key].as<T>();
	}
	return default_value;
}

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

// added in v2.5
int global_config::get_gps_time_uart_nr() const {
	return get_or_default(doc, "gps_time_uart_nr", 2);
}

void global_config::set_gps_time_uart_nr(int gps_time_uart_nr) {
	doc["gps_time_uart_nr"] = gps_time_uart_nr;
}

int global_config::get_gps_baud() const {
	return get_or_default(doc, "baud", 9600);
}

void global_config::set_gps_baud(int baud) {
	doc["gps_baud"] = baud;
}

const global_config::co2_ppm_state_s& global_config::get_co2_ppm_data(float measured_temp, float air_inlet_temp) const {
	const int half_range = (CO2_MATRIX_SIDE_LENGTH - 1) / 2;
	const int half_state = (CO2_STATES_COUNT - 1) / 2;

	// adjust [-10,10] to [0,20]
	int inlet_set_diff = min(max((int)round(air_inlet_temp - temp_setpoint_c), -half_range), half_range) + half_range;
	int meas_set_diff = min(max((int)round(measured_temp - temp_setpoint_c), -half_range), half_range) + half_range;

	// adjust [-4,4] to [0, 8]
	int wanted_state = co2_matrix[inlet_set_diff][meas_set_diff] + half_state;
	if(wanted_state > 0 && wanted_state < CO2_STATES_COUNT) {
		return co2_states[wanted_state];
	}

	// fallback to middle state
	return co2_states[half_state];
}

static bool writeFile(String filename, String message) {
	File file = LittleFS.open(filename, "w");
	if (!file) {
		Serial.println("writeFile -> failed to open file for writing");
		return false;
	}

	if (file.print(message)) {
		Serial.println("File written");
		file.close();
		return true;
	}

	Serial.println("Write failed");
	file.close();
	return false;
}

static bool readFile(String filename, String& contents) {
	File file = LittleFS.open(filename);
	if (!file) {
		Serial.println("Failed to open file for reading, size: ");
		Serial.println(file.size());
		return false;
	}

	Serial.println("File open OK, size: ");
	Serial.println(file.size());

	contents = "";
	while (file.available()) {
		contents += file.readString();
	}
	file.close();
	Serial.println(file.size());

	return true;
}

static bool readConfig() {
	File file = LittleFS.open(global_config_filename);
	if (!file) {
		Serial.println("Failed to open file for reading, size: ");
		Serial.println(file.size());
		return false;
	}

	auto error = deserializeJson(doc, file);

	file.close();
	if (error) {
		Serial.println("Error interpreting config file");
		Serial.println(error.c_str());
		return false;
	}

	//file = LittleFS.open(global_config_filename);
	//Serial.println("Contents:");
	//Serial.println(file.readString());
	//Serial.println("-----------");
	//file.close();

	global_config_data.destination_address = doc["destination"].as<String>();
	global_config_data.auth_user = doc["auth_user"].as<String>();
	global_config_data.auth_password = doc["auth_pw"].as<String>();
	global_config_data.interval = doc["interval"].as<int>();

	global_config_data.rh_high = doc["rh_high"].as<float>();
	global_config_data.rh_medium = doc["rh_medium"].as<float>();
	global_config_data.rh_low = doc["rh_low"].as<float>();

	// added in v2.1
	global_config_data.use_rh_headroom_mode = get_or_default(doc, "use_rh_headroom_mode", false);
	global_config_data.rh_attainable_headroom_high = get_or_default(doc, "rh_attainable_headroom_high", 45.0f);
	global_config_data.rh_attainable_headroom_medium = get_or_default(doc, "rh_attainable_headroom_medium", 30.0f);
	global_config_data.rh_attainable_headroom_low = get_or_default(doc, "rh_attainable_headroom_low", 10.0f);

	// added in v2.4
	global_config_data.rh_headroom_mode_rh_medium_bound = get_or_default(doc, "rh_headroom_mode_rh_medium_bound", 90.0f);
	global_config_data.rh_headroom_mode_rh_low_bound = get_or_default(doc, "rh_headroom_mode_rh_low_bound", 62.50);

	// added in v2.5
	global_config_data.use_gps_time = get_or_default(doc, "use_gps_time", false);

	// added in v2.6

	for (int i = 0; i < CO2_STATES_COUNT; ++i) {
		const auto& co2_state_data = doc["co2"]["temp_wanted_factor"][i];
		global_config_data.co2_states[i].low = co2_state_data["low"].as<int>();
		global_config_data.co2_states[i].medium = co2_state_data["medium"].as<int>();
		global_config_data.co2_states[i].high = co2_state_data["high"].as<int>();
	}

	global_config_data.temp_setpoint_c = doc["co2"]["temp_setpoint_c"].as<float>();

	ArduinoJson::copyArray(doc["co2"]["matrix"], global_config_data.co2_matrix);

	return true;
}

static bool saveConfig() {
	// write variables to JSON file
	doc["destination"] = global_config_data.destination_address;
	doc["auth_user"] = global_config_data.auth_user;
	doc["auth_pw"] = global_config_data.auth_password;
	doc["interval"] = global_config_data.interval;
	doc["rh_high"] = global_config_data.rh_high;
	doc["rh_medium"] = global_config_data.rh_medium;
	doc["rh_low"] = global_config_data.rh_low;
	doc["use_rh_headroom_mode"] = global_config_data.use_rh_headroom_mode;
	doc["rh_attainable_headroom_high"] = global_config_data.rh_attainable_headroom_high;
	doc["rh_attainable_headroom_medium"] = global_config_data.rh_attainable_headroom_medium;
	doc["rh_attainable_headroom_low"] = global_config_data.rh_attainable_headroom_low;
	doc["rh_headroom_mode_rh_medium_bound"] = global_config_data.rh_headroom_mode_rh_medium_bound;
	doc["rh_headroom_mode_rh_low_bound"] = global_config_data.rh_headroom_mode_rh_low_bound;
	doc["use_gps_time"] = global_config_data.use_gps_time;
	doc["co2"]["temp_setpoint_c"] = global_config_data.temp_setpoint_c;
	
	for (int i = 0; i < CO2_STATES_COUNT; ++i) {
		doc["co2"]["temp_wanted_factor"][i]["low"] = global_config_data.co2_states[i].low;
		doc["co2"]["temp_wanted_factor"][i]["medium"] = global_config_data.co2_states[i].medium;
		doc["co2"]["temp_wanted_factor"][i]["high"] = global_config_data.co2_states[i].high;
	}

	doc["co2"].remove("matrix");
	ArduinoJson::copyArray(global_config_data.co2_matrix, doc["co2"]["matrix"]);

	File file = LittleFS.open(global_config_filename, "w");
	if (!file) {
		Serial.println("writeFile -> failed to open file for writing");
		return false;
	}

	size_t written = ArduinoJson::serializeJson(doc, file);

	file.close();
	Serial.print("Closed written file, written:");
	Serial.println(written);

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
