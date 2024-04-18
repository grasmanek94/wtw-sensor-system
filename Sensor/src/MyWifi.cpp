#include "LittleFS.hpp"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <lwip/dns.h>

static const unsigned long max_wifi_timeout_until_reconnect = 15000;
static unsigned long require_wifi_reconnect_time = 0;

void init_wifi() {

    String mac = WiFi.macAddress();
    mac.replace(":", "");

    Serial.println("sr-" + mac + ".local");

    WiFi.persistent(false);
    WiFi.disconnect();

    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.setHostname(mac.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(global_config_data.get_wifi_ssid(), global_config_data.get_wifi_password());

    Serial.print("Connecting to WiFi '");
    Serial.print(global_config_data.get_wifi_ssid());
    Serial.print("' ..");

    int try_count = 0;
    while (WiFi.status() != WL_CONNECTED) {
        if(++try_count > 30) {
            try_count = 0;
            Serial.println("Reconnect");
            WiFi.reconnect();
        } else {
            Serial.print('.');
            delay(1000);
        }
    }

    Serial.println(WiFi.localIP());

    if (!MDNS.begin("sr-" + mac + ".local")) {
        Serial.println("Error setting up MDNS responder!");
    }
}

void check_wifi() {
    unsigned long now = millis();

    bool connected = (WiFi.status() == WL_CONNECTED);
    bool wifi_reconnect_elapsed = (now > require_wifi_reconnect_time);
    if (connected || wifi_reconnect_elapsed) {
        require_wifi_reconnect_time = now + max_wifi_timeout_until_reconnect;

        if (!connected && wifi_reconnect_elapsed) {
            WiFi.disconnect();
            WiFi.reconnect();
        }
    }
}

bool is_wifi_connected_debug() {
    if (WiFi.status() != WL_CONNECTED) {
        if (Serial) {
            Serial.print("WiFi not connected");
        }
        return false;
    }
    return true;
}