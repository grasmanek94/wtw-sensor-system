#include "HTTPPages.hpp"

#include "LittleFS.hpp"

#include <ESPAsyncWebSrv.h>
#include <Update.h>

extern char* SENSOR_VERSION_STR;
extern String SENSORS_LIST_STR;

bool check_auth(AsyncWebServerRequest* request) {
    if (!request->authenticate(global_config_data.auth_user.c_str(), global_config_data.auth_password.c_str())) {
        //request->send(HTTP_FORBIDDEN, "text/plain");
        return false;
    }
    return true;
}

void http_page_not_found(AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
}

const char flash_html[] PROGMEM = R"rawliteral(
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

String http_page_flash_processor(const String& var) {
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
    ESP.restart();
}

void http_api_flash(AsyncWebServerRequest* request) {
    if (!check_auth(request)) {
        return request->requestAuthentication();
    }
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

const char config_html_start[] PROGMEM = R"rawliteral(
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

String html_encode(String data) {
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

String add_form_label(String id, String name, String value) {
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
    ADD_OPTION(wifi_password, "WiFi Passsword");
    ADD_OPTION(destination_address, "Destination Address");
    ADD_OPTION(auth_user, "API User");
    ADD_OPTION(auth_password, "API Password");
    ADD_OPTION(interval, "Update Interval (s)");
    ADD_OPTION(manual_calibration_performed, "Manual Calibration Performed");

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
    PARSE_ENTRY_STR(destination_address);
    PARSE_ENTRY_STR(auth_user);
    PARSE_ENTRY_STR(auth_password);
    PARSE_ENTRY_INT(interval);
    PARSE_ENTRY_INT(manual_calibration_performed);

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