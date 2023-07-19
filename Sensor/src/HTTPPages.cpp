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
