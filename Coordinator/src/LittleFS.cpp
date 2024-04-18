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
	return get_or_default<String>(doc,"wifi_id", "coordinator-setup");
}

void global_config::set_wifi_ssid(const String& wifi_ssid) {
	doc["wifi_id"] = wifi_ssid;
}

String global_config::get_wifi_password() const {
	return get_or_default<String>(doc,"wifi_pw", "coordinator-setup");
}

void global_config::set_wifi_password(const String& wifi_password) {
	doc["wifi_pw"] = wifi_password;
}

String global_config::get_device_custom_hostname() const {
	return get_or_default<String>(doc,"hostname", "sensor-coordinator.local");
}

void global_config::set_device_custom_hostname(const String& device_custom_hostname) {
	doc["hostname"] = device_custom_hostname;
}

String global_config::get_static_ip() const {
	return get_or_default<String>(doc,"static_ip", "192.168.1.2");
}

void global_config::set_static_ip(const String& static_ip) {
	doc["static_ip"] = static_ip;
}

String global_config::get_gateway_ip() const {
	return get_or_default<String>(doc,"gateway_ip", "192.168.1.1");
}

void global_config::set_gateway_ip(const String& gateway_ip) {
	doc["gateway_ip"] = gateway_ip;
}

String global_config::get_subnet() const {
	return get_or_default<String>(doc,"subnet", "255.255.255.0");
}

void global_config::set_subnet(const String& subnet) {
	doc["subnet"] = subnet;
}

String global_config::get_primary_dns() const {
	return get_or_default<String>(doc,"primary_dns", "192.168.1.1");
}

void global_config::set_primary_dns(const String& primary_dns) {
	doc["primary_dns"] = primary_dns;
}

String global_config::get_secondary_dns() const {
	return get_or_default<String>(doc,"secondary_dns", "");
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

template <typename T> float sign_nonzero(T val) {
	return (T)(T(0) <= val) - (T)(val < T(0)); // change <= to < to return [-1,0,1], now it returns [-1, 1] (on purpose!)
}

float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

const global_config::co2_ppm_state_s global_config::get_co2_ppm_data(float measured_temp, float air_inlet_temp, float& aggressiveness_result) const {
	const float base_aggressiveness = 1.0f;
	//const float local_temp_range = 30.0f;
	const float local_temp_range = 0.03333f;
	const float inlet_skew_factor = -4.0f;
	const float setpoint_skew_factor = 2.0f;
	const float aggressiveness_min = 0.0f;
	const float aggressiveness_max = 2.0f;

	//const float inlet_room_factor = (inlet_temp - room_temp) / local_temp_range;
	//const float inlet_setpoint_factor = (inlet_temp - setpoint_temp_c) / local_temp_range;
	//const float setpoint_room_factor = (room_temp - setpoint_temp_c) / local_temp_range;

	const float inlet_room_factor = (air_inlet_temp - measured_temp) * local_temp_range;
	const float inlet_setpoint_factor = (air_inlet_temp - temp_setpoint_c) * local_temp_range;
	const float setpoint_room_factor = (measured_temp - temp_setpoint_c) * local_temp_range;
	const float inlet_factor = inlet_room_factor + inlet_setpoint_factor;

	const float room_heating_direction = sign_nonzero(setpoint_room_factor);

	const bool cooling_skew = ((measured_temp >= temp_setpoint_c) && (air_inlet_temp < temp_setpoint_c));
	const bool heating_skew = ((measured_temp < temp_setpoint_c) && (air_inlet_temp >= temp_setpoint_c));
	const float skewed_distance_correction = (float)(cooling_skew | heating_skew);

	const float combined_skewed_distance_factor = inlet_skew_factor * inlet_setpoint_factor * room_heating_direction;
	const float total_skewed_distance_factor = (setpoint_skew_factor + combined_skewed_distance_factor) * setpoint_room_factor * skewed_distance_correction;

	const float skewed_heat_direction = sign_nonzero(-1.0f * skewed_distance_correction);

	const float result = std::min(std::max(base_aggressiveness - (inlet_factor + total_skewed_distance_factor) * room_heating_direction * skewed_heat_direction, aggressiveness_min), aggressiveness_max);
	aggressiveness_result = result;

	global_config::co2_ppm_state_s state{
		(int)mapf(result, aggressiveness_min, aggressiveness_max, conservative_co2_state.high, aggressive_co2_state.high),
		(int)mapf(result, aggressiveness_min, aggressiveness_max, conservative_co2_state.medium, aggressive_co2_state.medium),
		(int)mapf(result, aggressiveness_min, aggressiveness_max, conservative_co2_state.low, aggressive_co2_state.low)		
	};

	return state;
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

	global_config_data.destination_address = get_or_default<String>(doc, "destination", "192.168.1.3");
	global_config_data.auth_user = get_or_default<String>(doc, "auth_user", "sensoruser");

	{
		char mac_address[17];
		mac_address[sizeof(mac_address)-1] = '0';
		snprintf(mac_address, sizeof(mac_address), "%llX", ESP.getEfuseMac());

		global_config_data.auth_password = get_or_default<String>(doc, "auth_pw", String(mac_address));
	}

	global_config_data.interval = get_or_default<int>(doc, "interval", 30);

	global_config_data.rh_high = get_or_default<float>(doc, "rh_high", 95.0f);
	global_config_data.rh_medium = get_or_default<float>(doc, "rh_medium", 90.0f);
	global_config_data.rh_low = get_or_default<float>(doc, "rh_low", 65.0f);

	// added in v2.1
	global_config_data.use_rh_headroom_mode = get_or_default(doc, "use_rh_headroom_mode", true);
	global_config_data.rh_attainable_headroom_high = get_or_default(doc, "rh_attainable_headroom_high", 45.0f);
	global_config_data.rh_attainable_headroom_medium = get_or_default(doc, "rh_attainable_headroom_medium", 30.0f);
	global_config_data.rh_attainable_headroom_low = get_or_default(doc, "rh_attainable_headroom_low", 15.0f);

	// added in v2.4
	global_config_data.rh_headroom_mode_rh_medium_bound = get_or_default(doc, "rh_headroom_mode_rh_medium_bound", 90.0f);
	global_config_data.rh_headroom_mode_rh_low_bound = get_or_default(doc, "rh_headroom_mode_rh_low_bound", 62.50);

	// added in v2.5
	global_config_data.use_gps_time = get_or_default(doc, "use_gps_time", false);

	// added in v2.7
	if(doc.containsKey("co2")) {
		global_config_data.temp_setpoint_c = get_or_default<float>(doc["co2"], "temp_setpoint_c", 20.0f);
		global_config_data.use_average_temp_for_co2 = get_or_default<bool>(doc["co2"], "use_average_inside_temp", true);
		global_config_data.conservative_co2_state.low = get_or_default<float>(doc["co2"], "conservative_low", 1500);
		global_config_data.conservative_co2_state.medium = get_or_default<float>(doc["co2"], "conservative_medium", 2000);
		global_config_data.conservative_co2_state.high = get_or_default<float>(doc["co2"], "conservative_high", 2500);
		global_config_data.aggressive_co2_state.low = get_or_default<float>(doc["co2"], "aggressive_low", 400);
		global_config_data.aggressive_co2_state.medium = get_or_default<float>(doc["co2"], "aggressive_medium", 700);
		global_config_data.aggressive_co2_state.high = get_or_default<float>(doc["co2"], "aggressive_high", 1000);
	} else {
		global_config_data.temp_setpoint_c = 20.0f;
		global_config_data.use_average_temp_for_co2 = true;	
		global_config_data.conservative_co2_state.low = 1500;
		global_config_data.conservative_co2_state.medium = 2000;
		global_config_data.conservative_co2_state.high = 2500;
		global_config_data.aggressive_co2_state.low = 400;
		global_config_data.aggressive_co2_state.medium = 700;
		global_config_data.aggressive_co2_state.high = 1000;
	}

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
	doc["co2"]["use_average_inside_temp"] = global_config_data.use_average_temp_for_co2;
	doc["co2"]["conservative_low"] = global_config_data.conservative_co2_state.low;
	doc["co2"]["conservative_medium"] = global_config_data.conservative_co2_state.medium;
	doc["co2"]["conservative_high"] = global_config_data.conservative_co2_state.high;
	doc["co2"]["aggressive_low"] = global_config_data.aggressive_co2_state.low;
	doc["co2"]["aggressive_medium"] = global_config_data.aggressive_co2_state.medium;
	doc["co2"]["aggressive_high"] = global_config_data.aggressive_co2_state.high;
		
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
