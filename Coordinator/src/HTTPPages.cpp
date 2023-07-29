#include "HTTPPages.hpp"

#include "DeviceData.hpp"
#include "LittleFS.hpp"

#include <ESPAsyncWebSrv.h>
#include <Update.h>

#include <functional>

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
    return true;
    if (!request->authenticate(global_config_data.auth_user.c_str(), global_config_data.auth_password.c_str())) {
        return false;
    }
    return true;
}

void http_page_not_found(AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
}

static bool process_location_request_data(AsyncWebServerRequest* request, String device_id, unsigned long sequence_number, String location) {
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

    return true;
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

static void dump_measurements_data(AsyncWebServerRequest* request, std::function<RingBufInterface<measurement_entry>*(device_data* sensor)> getter) {
    if (!check_auth(request)) {
        return request->requestAuthentication();
    }

    int location_id = get_sensor_index(request);
    if (location_id == INVALID_DEVICE_ID) {
        return request->send(HTTP_BAD_REQUEST, "text/plain");
    }

    device_data* sensor = &sensors[location_id];
    auto* input_data = getter(sensor);

    if (input_data->isEmpty()) {
        return request->send(HTTP_OK_NO_CONTENT, "text/plain");
    }

    String data;
    for (int i = 0; i < input_data->size(); ++i) {
        if (i == 0) {
            data += input_data->at(i).getHeaders();
        }

        data += input_data->at(i).toString();
    }

    request->send(HTTP_OK, "text/plain", data);
}

void http_page_very_short_data(AsyncWebServerRequest* request) {   
    dump_measurements_data(request, [&](device_data* sensor) {
        return &sensor->very_short_data;
    });
}

void http_page_short_data(AsyncWebServerRequest* request) {
    dump_measurements_data(request, [&](device_data* sensor) {
        return &sensor->short_data;
    });
}

void http_page_long_data(AsyncWebServerRequest* request) {
    dump_measurements_data(request, [&](device_data* sensor) {
        return &sensor->long_data;
    });
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

   ADD_OPTION(wifi_ssid, "WiFi SSID");
   ADD_OPTION(wifi_password, "WiFi Password");
   ADD_OPTION(device_custom_hostname, "Custom Hostname");
   ADD_OPTION(destination_address, "Destination Address");
   ADD_OPTION(auth_user, "API User");
   ADD_OPTION(auth_password, "API Password");
   ADD_OPTION(interval, "Update Interval (s)");
   ADD_OPTION(co2_ppm_high, "co2_ppm_high");
   ADD_OPTION(co2_ppm_medium, "co2_ppm_medium");
   ADD_OPTION(co2_ppm_low, "co2_ppm_low");
   ADD_OPTION(rh_high, "rh_high");
   ADD_OPTION(rh_medium, "rh_medium");
   ADD_OPTION(rh_low, "rh_low");
   ADD_OPTION(static_ip, "Static IP Address");
   ADD_OPTION(gateway_ip, "Gateway IP Address");
   ADD_OPTION(subnet, "Subnet Mask");
   ADD_OPTION(primary_dns, "Primary DNS IP Address");
   ADD_OPTION(secondary_dns, "Secondary DNS IP Address");

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
        return request->send(HTTP_BAD_REQUEST, "text/html", "!wifi_ssid"); \
    } \
    global_config_data. name = param->value() conversion; \
    Serial.println(global_config_data. name)

#define PARSE_ENTRY_STR(name) PARSE_ENTRY(name, )
#define PARSE_ENTRY_INT(name) PARSE_ENTRY(name, .toInt())
#define PARSE_ENTRY_FLOAT(name) PARSE_ENTRY(name, .toFloat())

    AsyncWebParameter* param = nullptr;
    PARSE_ENTRY_STR(wifi_ssid);
    PARSE_ENTRY_STR(wifi_password);
    PARSE_ENTRY_STR(device_custom_hostname);
    PARSE_ENTRY_STR(destination_address);
    PARSE_ENTRY_STR(auth_user);
    PARSE_ENTRY_STR(auth_password);
    PARSE_ENTRY_INT(interval);
    PARSE_ENTRY_INT(co2_ppm_high);
    PARSE_ENTRY_INT(co2_ppm_medium);
    PARSE_ENTRY_INT(co2_ppm_low);
    PARSE_ENTRY_FLOAT(rh_high);
    PARSE_ENTRY_FLOAT(rh_medium);
    PARSE_ENTRY_FLOAT(rh_low);
    PARSE_ENTRY_STR(static_ip);
    PARSE_ENTRY_STR(gateway_ip);
    PARSE_ENTRY_STR(subnet);
    PARSE_ENTRY_STR(primary_dns);
    PARSE_ENTRY_STR(secondary_dns);

#undef PARSE_ENTRY_FLOAT
#undef PARSE_ENTRY_INT
#undef PARSE_ENTRY_STR
#undef PARSE_ENTRY

    littlefs_write_config();
    if (!littlefs_read_config()) {
        return request->send(HTTP_BAD_REQUEST, "text/html");
    }

    request->send(HTTP_OK, "text/html");

    ESP.restart();
}
