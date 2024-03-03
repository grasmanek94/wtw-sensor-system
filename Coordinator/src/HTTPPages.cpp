#include "HTTPPages.hpp"

#include "DeviceData.hpp"
#include "LittleFS.hpp"

#include <ESPAsyncWebSrv.h>
#include <Update.h>

static const int INVALID_DEVICE_ID = -1;

static const char flash_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
</head>
<body>
  <form method="POST" action="/flash" enctype="multipart/form-data"><input type="file" name="data"/><input type="submit" name="upload" value="Upload" title="Upload File"></form>
</body>
</html>
)rawliteral";

static const char config_html_start[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <style>
	table:not(.js) [value^="-4"] {background-color: rgb(209, 54 , 0  );}
	table:not(.js) [value^="-3"] {background-color: rgb(255, 92 , 38 );}
	table:not(.js) [value^="-2"] {background-color: rgb(255, 125, 82 );}
	table:not(.js) [value^="-1"] {background-color: rgb(160, 160, 160);}
	table:not(.js) [value^="0"]  {background-color: rgb(200, 200, 200);}
	table:not(.js) [value^="1"]  {background-color: rgb(190, 238, 250);}
	table:not(.js) [value^="2"]  {background-color: rgb(119, 226, 252);}
	table:not(.js) [value^="3"]  {background-color: rgb(43 , 252, 155);}
	table:not(.js) [value^="4"]  {background-color: rgb(4  , 181, 98 );}
	table:not(.js) input:focus   {background-color: rgb(255, 255, 255);}
  </style> 
</head>
<body>
    <form action="/config" method="post">
)rawliteral";

const char config_html_end[] PROGMEM = R"rawliteral(
      <input type="submit" value="Save + Reset ESP32">
    </form>
</body>
</html>
)rawliteral";

static bool check_auth(AsyncWebServerRequest* request) {
	if (!request->authenticate(global_config_data.auth_user.c_str(), global_config_data.auth_password.c_str())) {
		return false;
	}
	return true;
}

void http_page_not_found(AsyncWebServerRequest* request) {
	request->send(404, "text/plain", "Not found");
}

static void process_location_request_data(AsyncWebServerRequest* request, String device_id, unsigned long sequence_number, String location) {
	int location_id = location.toInt();
	float rh = 0.0f;
	float temp = 0.0f;
	int co2_ppm = 0;
	int sensor_status = 0;

	if (!sensors[location_id].is_associated()) {
		sensors[location_id].associate(device_id, (SENSOR_LOCATION)location_id);
	}

	AsyncWebParameter* awp_rh = request->getParam("rh[" + location + "]");
	AsyncWebParameter* awp_temp = request->getParam("temp[" + location + "]");
	AsyncWebParameter* awp_co2 = request->getParam("co2[" + location + "]");
	AsyncWebParameter* awp_status = request->getParam("status[" + location + "]");

	if (awp_rh) {
		rh = awp_rh->value().toFloat();
	}

	if (awp_temp) {
		temp = awp_temp->value().toFloat();
	}

	if (awp_co2) {
		co2_ppm = awp_co2->value().toInt();
	}

	if (awp_status) {
		sensor_status = awp_status->value().toInt();
	}

	sensors[location_id].push(co2_ppm, rh, temp, sensor_status, sequence_number);
}

void http_api_update(AsyncWebServerRequest* request) {
	if (!check_auth(request)) {
		return;
	}

	AsyncWebParameter* param = request->getParam("deviceId");
	if (!param || param->value().length() < 1) {
		request->send(HTTP_BAD_REQUEST, "text/plain", "Missing 'deviceId'");
		return;
	}

	String device_id = param->value();

	param = request->getParam("seqnr");
	if (!param || param->value().length() < 1) {
		request->send(HTTP_BAD_REQUEST, "text/plain", "Missing 'seqnr'");
		return;
	}

	unsigned long sequence_number = param->value().toInt();

	int counter = 0;
	int params = request->params();
	for (int i = 0; i < params; i++) {
		AsyncWebParameter* p = request->getParam(i);
		if (p->name() == "loc") {
			if (++counter == MAX_LOCATIONS_PER_DEVICE) {
				request->send(HTTP_UNPROCESSABLE_CONTENT, "text/plain", "Maximum locations per report exceeded");
				return;
			}

			int location_id = p->value().toInt();

			if (location_id < 0 || location_id >= SENSORS_COUNT) {
				request->send(HTTP_UNPROCESSABLE_CONTENT, "text/plain", "Unknown location id: " + p->value());
				return;
			}

			if (sensors[location_id].is_associated() && sensors[location_id].id != device_id) {
				request->send(HTTP_UNPROCESSABLE_CONTENT, "text/plain", "Another sensor has already associated with location " + p->value());
				return;
			}

			if (!request->getParam("rh[" + p->value() + "]") &&
				!request->getParam("temp[" + p->value() + "]") &&
				!request->getParam("co2[" + p->value() + "]") &&
				!request->getParam("status[" + p->value() + "]")) {
				request->send(HTTP_BAD_REQUEST, "text/plain", "Location contains no parameters at all: rh[" + p->value() + "], temp[" + p->value() + "], co2[" + p->value() + "], status[" + p->value() + "]");
				return;
			}
		}
	}

	for (int i = 0; i < params; i++) {
		AsyncWebParameter* p = request->getParam(i);
		if (p->name() == "loc") {
			process_location_request_data(request, device_id, sequence_number, p->value());
		}
	}

	request->send(HTTP_OK, "text/plain");
}

static int get_sensor_index(AsyncWebServerRequest* request) {
	AsyncWebParameter* param = request->getParam("loc");
	if (param && param->value().length() > 0) {
		int location_id = param->value().toInt();
		if (location_id < 0 || location_id >= SENSORS_COUNT) {
			return INVALID_DEVICE_ID;
		}

		return location_id;
	}

	param = request->getParam("index");
	if (param && param->value().length() > 0) {
		int location_id = param->value().toInt();
		if (location_id < 0 || location_id >= SENSORS_COUNT) {
			return INVALID_DEVICE_ID;
		}

		return location_id;
	}

	return INVALID_DEVICE_ID;
}

void http_page_devices(AsyncWebServerRequest* request) {
	if (!check_auth(request)) {
		return request->requestAuthentication();
	}

	String devices = sensors[0].getHeaders();

	int location_id = get_sensor_index(request);
	if (location_id != INVALID_DEVICE_ID) {
		devices += sensors[location_id].toString(location_id);
		return request->send(HTTP_OK, "text/plain", devices);
	}

	for (int i = 0; i < SENSORS_COUNT; ++i) {
		auto& sensor = sensors[i];
		devices += sensors[i].toString(i);
	}

	request->send(HTTP_OK, "text/plain", devices);
}

static size_t process_chunked_response_input_data(
	uint8_t* buffer, size_t maxLen, size_t index,
	int* entry_idx, RingBufInterface<measurement_entry>* input_data,
	bool updates_only_sequence_number, bool updates_only_timestamp,
	unsigned long sequence_number, unsigned long timestamp) {
	if (entry_idx == nullptr) {
		return 0;
	}

	size_t written_len = 0;
	while (true) {
		if ((*entry_idx) >= input_data->size()) {
			delete entry_idx;
			return written_len;
		}

		String data("");

		if (*entry_idx == 0) {
			data += input_data->at(*entry_idx).getHeaders();
		}

		size_t current_len = data.length();

		if ((!updates_only_sequence_number && !updates_only_timestamp) ||
			(updates_only_sequence_number && input_data->at(*entry_idx).sequence_number > sequence_number) ||
			(updates_only_timestamp && input_data->at(*entry_idx).relative_time > timestamp)) {

			data += input_data->at(*entry_idx).toString();
			current_len = data.length();
		}

		size_t total_len = written_len + current_len;

		if (total_len > maxLen) {
			Serial.println("total_len > maxLen");
			// no point in writing more here, this shouldn't be ever called though.
			// logic to make this work will be difficult, and I'm lazy (GoodEnough(TM))
			delete entry_idx;
			return 0;
		}

		if (current_len > 0) {
			memcpy(buffer, data.c_str(), current_len);
			buffer += current_len;
		}
		written_len = total_len;

		++(*entry_idx);

		if ((*entry_idx) < input_data->size()) {
			if ((input_data->at(*entry_idx).toString().length() + written_len) < maxLen) {
				continue;
			}
		}

		if (written_len == 0) {
			delete entry_idx;
		}

		return written_len;
	}
}

static void dump_measurements_data(AsyncWebServerRequest* request, RingBufInterface<measurement_entry>* input_data) {
	if (input_data->isEmpty()) {
		return request->send(HTTP_OK_NO_CONTENT, "text/plain");
	}

	bool updates_only_sequence_number = false;
	unsigned long sequence_number = 0;

	AsyncWebParameter* param = request->getParam("seqnr");
	if (param && param->value().length() > 0) {
		sequence_number = param->value().toInt();
		updates_only_sequence_number = true;
	}

	bool updates_only_timestamp = false;
	unsigned long relative_time = 0;

	param = request->getParam("time");
	if (param && param->value().length() > 0) {
		relative_time = param->value().toInt();
		updates_only_timestamp = true;
	}

	int* entry_idx = new int;
	*entry_idx = 0;

	AsyncWebServerResponse* response = request->beginChunkedResponse("text/plain", [entry_idx, input_data, updates_only_sequence_number, sequence_number, updates_only_timestamp, relative_time](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
		return process_chunked_response_input_data(buffer, maxLen, index, entry_idx, input_data, updates_only_sequence_number, updates_only_timestamp, relative_time, sequence_number);
		});

	response->setCode(HTTP_OK);
	response->setContentType("text/plain");
	request->send(response);
}

void http_page_very_short_data(AsyncWebServerRequest* request) {
	if (!check_auth(request)) {
		return request->requestAuthentication();
	}

	int location_id = get_sensor_index(request);
	if (location_id == INVALID_DEVICE_ID) {
		return request->send(HTTP_BAD_REQUEST, "text/plain");
	}

	dump_measurements_data(request, &(sensors[location_id].very_short_data));
}

void http_page_short_data(AsyncWebServerRequest* request) {
	if (!check_auth(request)) {
		return request->requestAuthentication();
	}

	int location_id = get_sensor_index(request);
	if (location_id == INVALID_DEVICE_ID) {
		return request->send(HTTP_BAD_REQUEST, "text/plain");
	}

	dump_measurements_data(request, &(sensors[location_id].short_data));
}

void http_page_long_data(AsyncWebServerRequest* request) {
	if (!check_auth(request)) {
		return request->requestAuthentication();
	}

	int location_id = get_sensor_index(request);
	if (location_id == INVALID_DEVICE_ID) {
		return request->send(HTTP_BAD_REQUEST, "text/plain");
	}

	dump_measurements_data(request, &(sensors[location_id].long_data));
}

String http_page_flash_processor(const String& var) {
	return String();
}

void http_page_flash(AsyncWebServerRequest* request) {
	if (!check_auth(request)) {
		return request->requestAuthentication();
	}

	request->send_P(200, "text/html", flash_html, http_page_flash_processor);
}

void http_api_flash(AsyncWebServerRequest* request) {
	if (!check_auth(request)) {
		return request->requestAuthentication();
	}

	ESP.restart();
}

void http_api_flash_part(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
	if (!check_auth(request)) {
		return request->requestAuthentication();
	}

	if (index == 0) {
		if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
			Update.printError(Serial);
			request->redirect("/flash?status=error&message=" + String(Update.errorString()));
		}
	}

	if (len) {
		if (Update.write(data, len) != len) {
			Update.printError(Serial);
			request->redirect("/flash?status=error&message=" + String(Update.errorString()));
		}
	}

	if (final) {
		if (Update.end(true)) { //true to set the size to the current progress
			request->redirect("/flash?status=success");
		}
		else {
			Update.printError(Serial);
			request->redirect("/flash?status=error&message=" + String(Update.errorString()));
		}
	}
}

static String html_encode(String data) {
	const char* p = data.c_str();
	String rv = "";
	while (p && *p) {
		char escapeChar = *p++;
		switch (escapeChar) {
		case '&': rv += "&amp;"; break;
		case '<': rv += "&lt;"; break;
		case '>': rv += "&gt;"; break;
		case '"': rv += "&quot;"; break;
		case '\'': rv += "&#x27;"; break;
		case '/': rv += "&#x2F;"; break;
		default: rv += escapeChar; break;
		}
	}
	return rv;
}

static String add_form_label(String id, String name, String value) {
	return "<label for=\"" + id + "\">" + name + ":</label><br>\n"
		"<input type=\"text\" id=\"" + id + "\" name=\"" + id + "\" value=\"" + html_encode(value) + "\"><br>\n";
}

void http_page_config(AsyncWebServerRequest* request) {
	if (!check_auth(request)) {
		return request->requestAuthentication();
	}

	String html(config_html_start);

#define ADD_OPTION(name, description)  html += add_form_label(#name, description, String(global_config_data. name))
#define ADD_OPTION_FUNC(name, description)  html += add_form_label(#name, description, String(global_config_data.get_##name()))
#define ADD_OPTION_CO2(idx, name, state)  html += add_form_label("co2_ppm_" #name #idx, "co2_ppm_" #name " (State " #state ")", String(global_config_data.co2_states[idx]. name))
#define ADD_OPTION_FULL_CO2(idx, state) \
	ADD_OPTION_CO2(idx, high, state); \
	ADD_OPTION_CO2(idx, medium, state); \
	ADD_OPTION_CO2(idx, low, state)

	ADD_OPTION_FUNC(wifi_ssid, "WiFi SSID");
	ADD_OPTION_FUNC(wifi_password, "WiFi Password");
	ADD_OPTION_FUNC(device_custom_hostname, "Custom Hostname");
	ADD_OPTION(destination_address, "Destination Address");
	ADD_OPTION(auth_user, "API User");
	ADD_OPTION(auth_password, "API Password");
	ADD_OPTION(interval, "Update Interval (s)");
	ADD_OPTION(rh_high, "rh_high");
	ADD_OPTION(rh_medium, "rh_medium");
	ADD_OPTION(rh_low, "rh_low");
	ADD_OPTION_FUNC(static_ip, "Static IP Address");
	ADD_OPTION_FUNC(gateway_ip, "Gateway IP Address");
	ADD_OPTION_FUNC(subnet, "Subnet Mask");
	ADD_OPTION_FUNC(primary_dns, "Primary DNS IP Address");
	ADD_OPTION_FUNC(secondary_dns, "Secondary DNS IP Address");
	ADD_OPTION(use_rh_headroom_mode, "Use relative humidity headroom calculations to try to save power, requires presence of outside/inlet temp & RH sensor (mapped to location SENSOR_LOCATION::NEW_AIR_INLET)");
	ADD_OPTION(rh_attainable_headroom_high, "(Current RH - Possible RH) : HIGH");
	ADD_OPTION(rh_attainable_headroom_medium, "(Current RH - Possible RH) : MEDIUM");
	ADD_OPTION(rh_attainable_headroom_low, "(Current RH - Possible RH) : LOW");
	ADD_OPTION(rh_headroom_mode_rh_medium_bound, "When ventilation state is HIGH but RH is below medium bound, force MEDIUM RH ventilationstate");
	ADD_OPTION(rh_headroom_mode_rh_low_bound, "When ventilation state is MEDIUM but RH is below low bound, force LOW RH ventilation state");
	ADD_OPTION(use_gps_time, "Use GPS time");
	ADD_OPTION_FUNC(gps_time_uart_nr, "GPS time UART nr");
	ADD_OPTION_FUNC(gps_baud, "GPS baud rate");

	ADD_OPTION(temp_setpoint_c, "Setpoint Temperature (*C) for CO2 matrix");
	ADD_OPTION_FULL_CO2(0, "-4");
	ADD_OPTION_FULL_CO2(1, "-3");
	ADD_OPTION_FULL_CO2(2, "-2");
	ADD_OPTION_FULL_CO2(3, "-1");
	ADD_OPTION_FULL_CO2(4, "0");
	ADD_OPTION_FULL_CO2(5, "1");
	ADD_OPTION_FULL_CO2(6, "2");
	ADD_OPTION_FULL_CO2(7, "3");
	ADD_OPTION_FULL_CO2(8, "4");

#undef ADD_OPTION_FULL_CO2
#undef ADD_OPTION_CO2
#undef ADD_OPTION_FUNC
#undef ADD_OPTION

	html += config_html_end;
	request->send(200, "text/html", html);
}

void http_api_config(AsyncWebServerRequest* request) {
	if (!check_auth(request)) {
		return request->requestAuthentication();
	}

#define PARSE_ENTRY(name, conversion) param = request->getParam(#name, true); \
    if (!param) { \
        return request->send(HTTP_BAD_REQUEST, "text/html", "!" #name); \
    } \
    global_config_data. name = param->value() conversion; \
    Serial.println(global_config_data. name)

#define PARSE_ENTRY_FUNC(name, conversion) param = request->getParam(#name, true); \
    if (!param) { \
        return request->send(HTTP_BAD_REQUEST, "text/html", "!" #name); \
    } \
    global_config_data.set_##name(param->value() conversion); \
    Serial.println(global_config_data.get_##name())

#define PARSE_ENTRY_CO2_STATE(idx, name) param = request->getParam("co2_ppm_" #name #idx, true); \
    if (!param) { \
        return request->send(HTTP_BAD_REQUEST, "text/html", "!" "co2_ppm_" #name #idx); \
    } \
    global_config_data.co2_states[idx]. name = param->value().toInt(); \
    Serial.println(global_config_data.co2_states[idx]. name)

#define PARSE_ENTRY_CO2_STATE_SET(idx) \
	PARSE_ENTRY_CO2_STATE(idx, low); \
	PARSE_ENTRY_CO2_STATE(idx, medium); \
	PARSE_ENTRY_CO2_STATE(idx, high)

#define PARSE_ENTRY_STR_FUNC(name) PARSE_ENTRY_FUNC(name, )
#define PARSE_ENTRY_INT_FUNC(name) PARSE_ENTRY_FUNC(name, .toInt())
#define PARSE_ENTRY_STR(name) PARSE_ENTRY(name, )
#define PARSE_ENTRY_INT(name) PARSE_ENTRY(name, .toInt())
#define PARSE_ENTRY_FLOAT(name) PARSE_ENTRY(name, .toFloat())

	AsyncWebParameter* param = nullptr;
	PARSE_ENTRY_STR_FUNC(wifi_ssid);
	PARSE_ENTRY_STR_FUNC(wifi_password);
	PARSE_ENTRY_STR_FUNC(device_custom_hostname);
	PARSE_ENTRY_STR(destination_address);
	PARSE_ENTRY_STR(auth_user);
	PARSE_ENTRY_STR(auth_password);
	PARSE_ENTRY_INT(interval);
	PARSE_ENTRY_FLOAT(rh_high);
	PARSE_ENTRY_FLOAT(rh_medium);
	PARSE_ENTRY_FLOAT(rh_low);
	PARSE_ENTRY_STR_FUNC(static_ip);
	PARSE_ENTRY_STR_FUNC(gateway_ip);
	PARSE_ENTRY_STR_FUNC(subnet);
	PARSE_ENTRY_STR_FUNC(primary_dns);
	PARSE_ENTRY_STR_FUNC(secondary_dns);
	PARSE_ENTRY_INT(use_rh_headroom_mode);
	PARSE_ENTRY_FLOAT(rh_attainable_headroom_high);
	PARSE_ENTRY_FLOAT(rh_attainable_headroom_medium);
	PARSE_ENTRY_FLOAT(rh_attainable_headroom_low);
	PARSE_ENTRY_FLOAT(rh_headroom_mode_rh_medium_bound);
	PARSE_ENTRY_FLOAT(rh_headroom_mode_rh_low_bound);
	PARSE_ENTRY_INT(use_gps_time);
	PARSE_ENTRY_INT_FUNC(gps_time_uart_nr);
	PARSE_ENTRY_INT_FUNC(gps_baud);
	PARSE_ENTRY_CO2_STATE_SET(0);
	PARSE_ENTRY_CO2_STATE_SET(1);
	PARSE_ENTRY_CO2_STATE_SET(2);
	PARSE_ENTRY_CO2_STATE_SET(3);
	PARSE_ENTRY_CO2_STATE_SET(4);
	PARSE_ENTRY_CO2_STATE_SET(5);
	PARSE_ENTRY_CO2_STATE_SET(6);
	PARSE_ENTRY_CO2_STATE_SET(7);
	PARSE_ENTRY_CO2_STATE_SET(8);

#undef PARSE_ENTRY_CO2_STATE
#undef PARSE_ENTRY_CO2_STATE_SET
#undef PARSE_ENTRY_FLOAT
#undef PARSE_ENTRY_INT
#undef PARSE_ENTRY_STR
#undef PARSE_ENTRY_INT_FUNC
#undef PARSE_ENTRY_STR_FUNC
#undef PARSE_ENTRY_FUNC
#undef PARSE_ENTRY

	littlefs_write_config();
	if (!littlefs_read_config()) {
		return request->send(HTTP_BAD_REQUEST, "text/html");
	}

	request->send(HTTP_OK, "text/html");

	ESP.restart();
}
