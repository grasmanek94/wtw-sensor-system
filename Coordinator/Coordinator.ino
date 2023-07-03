#include <ESPAsyncWebSrv.h>

#include <ESPmDNS.h>
#include <WiFi.h>
#include <time.h>

#include "src/LittleFS.hpp"
#include "src/DeviceData.hpp"
#include "src/HTTPPages.hpp"
#include "src/VentilationState.hpp"
#include "src/WiFiReconnect.hpp"

AsyncWebServer server(80);

void init_wifi() {
    Serial.print("Hostname: ");
    Serial.println(global_config_data.device_custom_hostname);

    WiFi.persistent(false);
    WiFi.disconnect();

    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.setHostname(global_config_data.device_custom_hostname.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(global_config_data.wifi_ssid, global_config_data.wifi_password);

    Serial.print("Connecting to WiFi '");
    Serial.print(global_config_data.wifi_ssid);
    Serial.print("' ..");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }
    
    Serial.println(WiFi.localIP());

    if (!MDNS.begin(global_config_data.device_custom_hostname)) {
        Serial.println("Error setting up MDNS responder!");
    }
}

void setup() {
	Serial.begin(115200);
	Serial.println(F("Starting..."));

    if (littlefs_read_config()) {
        init_wifi();

        server.on("/get/devices", HTTP_GET, http_page_devices);
        server.on("/get/very_short", HTTP_GET, http_page_very_short_data);
        server.on("/get/short", HTTP_GET, http_page_short_data);
        server.on("/get/long", HTTP_GET, http_page_long_data);

        server.on("/update", HTTP_POST, http_api_update);
        server.onNotFound(http_page_not_found);

        server.on("/heap", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "text/plain", String(ESP.getFreeHeap()));
        });

        server.on("/now", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "text/plain", String((unsigned long)time(NULL)));
        });

        server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "text/plain", 
                "/get/devices (index / id)\n"
                "/get/very_short (index / id)\n"
                "/get/short (index / id)\n"
                "/get/long (index / id)\n"
                "/update\n"
                "/heap\n"
                "/now\n"
                "");
            });

        server.begin();
    }

    if (global_config_data.interval < 1) {
        global_config_data.interval = 1;
    }
}

void loop() {
    check_wifi(); 
    check_measurements();
}
