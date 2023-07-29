#include "HTTPPages.hpp"

#include "LittleFS.hpp"

#include <ESPAsyncWebSrv.h>
#include <Update.h>

extern char* SENSOR_VERSION_STR;
extern String SENSORS_LIST_STR;

static const char flash_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
</head>
<body>
  <p><h1>Firmware Upload - Current Version: %SENSOR_VERSION% - %SENSORS_LIST%</h1></p>
  <form method="POST" action="/" enctype="multipart/form-data"><input type="file" name="data"/><input type="submit" name="upload" value="Upload" title="Upload File"></form>
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

static const char config_html_end[] PROGMEM = R"rawliteral(
      <input type="submit" value="Save + Reset ESP32">
    </form>
</body>
</html>
)rawliteral";

static bool check_auth(AsyncWebServerRequest* request) {
    if (!request->authenticate(global_config_data.auth_user.c_str(), global_config_data.auth_password.c_str())) {
        //request->send(HTTP_FORBIDDEN, "text/plain");
        return false;
    }
    return true;
}

void http_page_not_found(AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
}

static String http_page_flash_processor(const String& var) {
    if (var == "SENSOR_VERSION") {
        return SENSOR_VERSION_STR;
    }
    else if (var == "SENSORS_LIST") {
        return SENSORS_LIST_STR;
    }
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
            request->redirect("/?status=error&message=" + String(Update.errorString()));
        }
    }

    if (len) {
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
            request->redirect("/?status=error&message=" + String(Update.errorString()));
        }
    }

    if (final) {
        if (Update.end(true)) { //true to set the size to the current progress
            request->redirect("/?status=success");
        }
        else {
            Update.printError(Serial);
            request->redirect("/?status=error&message=" + String(Update.errorString()));
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

static String add_form_label_textarea(String id, String name, String value) {
    return "<label for=\"" + id + "\">" + name + ":</label><br>\n"
        "<textarea id=\"" + id + "\" name=\"" + id + "\" cols=\"100\" rows=\"50\">" + html_encode(value) + "</textarea><br>\n";
}

void http_page_config(AsyncWebServerRequest* request) {
    if (!check_auth(request)) {
        return request->requestAuthentication();
    }

    String html(config_html_start);

#define ADD_OPTION(name, description)  html += add_form_label(#name, description, String(global_config_data. name))
#define ADD_OPTION_FUNC(name, description)  html += add_form_label(#name, description, String(global_config_data.get_##name()))
#define ADD_OPTION_FUNC_ML(name, description)  html += add_form_label_textarea(#name, description, String(global_config_data.get_##name()))

    ADD_OPTION_FUNC(wifi_ssid, "WiFi SSID");
    ADD_OPTION_FUNC(wifi_password, "WiFi Passsword");
    ADD_OPTION(destination_address, "Destination Address");
    ADD_OPTION(auth_user, "API User");
    ADD_OPTION(auth_password, "API Password");
    ADD_OPTION(interval, "Update Interval (s)");
    ADD_OPTION(manual_calibration_performed, "Manual Calibration Performed");
    ADD_OPTION(temp_offset_x, "X temp offset (Tc = X * Ts + Y)");
    ADD_OPTION(temp_offset_y, "Y temp offset (Tc = X * Ts + Y)");
    ADD_OPTION_FUNC_ML(sensors, "Sensors Configuration");

#undef ADD_OPTION_FUNC_ML
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

#define PARSE_ENTRY_STR_FUNC(name) PARSE_ENTRY_FUNC(name, )
#define PARSE_ENTRY_STR(name) PARSE_ENTRY(name, )
#define PARSE_ENTRY_INT(name) PARSE_ENTRY(name, .toInt())
#define PARSE_ENTRY_FLOAT(name) PARSE_ENTRY(name, .toFloat())

    AsyncWebParameter* param = nullptr;
    PARSE_ENTRY_STR_FUNC(wifi_ssid);
    PARSE_ENTRY_STR_FUNC(wifi_password);
    PARSE_ENTRY_STR(destination_address);
    PARSE_ENTRY_STR(auth_user);
    PARSE_ENTRY_STR(auth_password);
    PARSE_ENTRY_INT(interval);
    PARSE_ENTRY_INT(manual_calibration_performed);
    PARSE_ENTRY_FLOAT(temp_offset_x);
    PARSE_ENTRY_FLOAT(temp_offset_y);
    PARSE_ENTRY_STR_FUNC(sensors);

#undef PARSE_ENTRY_FLOAT
#undef PARSE_ENTRY_INT
#undef PARSE_ENTRY_STR
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
