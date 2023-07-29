#include <ESPAsyncWebSrv.h>

#include <ESPmDNS.h>
#include <WiFi.h>
#include <time.h>

#include "src/LittleFS.hpp"
#include "src/DeviceData.hpp"
#include "src/HTTPPages.hpp"
#include "src/VentilationState.hpp"
#include "src/WiFiReconnect.hpp"

#define COORDINATOR_VERSION "2.0"

AsyncWebServer server(80);

bool use_static_ip = false;
IPAddress static_ip(INADDR_NONE);
IPAddress static_gateway_ip(INADDR_NONE);
IPAddress static_subnet(INADDR_NONE);
IPAddress static_primary_dns(INADDR_NONE);
IPAddress static_secondary_dns(INADDR_NONE);

void setup_static_ip() {
    if (global_config_data.static_ip.length() > 0) {
        Serial.println("Using static_ip...");
        use_static_ip = true;

        if (!static_ip.fromString(global_config_data.static_ip)) {
            static_ip = INADDR_NONE;
            use_static_ip = false;
            Serial.println("WARN: Cannot parse static_ip");
        }

        if (!static_gateway_ip.fromString(global_config_data.gateway_ip)) {
            static_gateway_ip = INADDR_NONE;
            Serial.println("WARN: Cannot parse gateway_ip");
        }

        if (!static_subnet.fromString(global_config_data.subnet)) {
            static_subnet = INADDR_NONE;
            Serial.println("WARN: Cannot parse subnet");
        }

        if (!static_primary_dns.fromString(global_config_data.primary_dns)) {
            static_primary_dns = INADDR_NONE;
            Serial.println("WARN: Cannot parse primary_dns");
        }

        if (!static_secondary_dns.fromString(global_config_data.secondary_dns)) {
            static_secondary_dns = INADDR_NONE;
            Serial.println("WARN: Cannot parse secondary_dns");
        }
    }
}

void init_wifi() {
    Serial.print("Hostname: ");
    Serial.println(global_config_data.device_custom_hostname);

    WiFi.persistent(false);
    WiFi.disconnect();

    if (use_static_ip) {
        WiFi.config(static_ip, static_gateway_ip, static_subnet, static_primary_dns, static_secondary_dns);
    }
    else {
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    }

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

    if (!littlefs_read_config()) {
        while (true) {
            Serial.println(F("Stalled"));
            delay(10000);
        }
    }

    setup_static_ip();
    init_wifi();

    server.on("/get/devices", HTTP_GET, http_page_devices);
    server.on("/get/very_short", HTTP_GET, http_page_very_short_data);
    server.on("/get/short", HTTP_GET, http_page_short_data);
    server.on("/get/long", HTTP_GET, http_page_long_data);

    server.on("/update", HTTP_POST, http_api_update);
    server.on("/update", HTTP_GET, http_api_update);
    server.onNotFound(http_page_not_found);

    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.on("/now", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", String((unsigned long)time(NULL)));
    });

    server.on("/flash", HTTP_GET, http_page_flash);
    server.on("/flash", HTTP_POST, http_api_flash, http_api_flash_part);

    server.on("/config", HTTP_GET, http_page_config);
    server.on("/config", HTTP_POST, http_api_config);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", 
            "/get/devices (index / id)\n"
            "/get/very_short (index / id)\n"
            "/get/short (index / id)\n"
            "/get/long (index / id)\n"
            "/update\n"
            "/heap\n"
            "/now\n"
            "/flash\n"
            "/config\n"
            "\nVersion: " COORDINATOR_VERSION
            "");
        });

    server.begin();

    if (global_config_data.interval < 1) {
        global_config_data.interval = 1;
    }
}

void loop() {
    check_wifi(); 
    check_measurements();
}
