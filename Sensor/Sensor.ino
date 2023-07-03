#include <UrlEncode.h>

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include <LittleFS.h>
#include <FS.h>
#include <lwip/dns.h>

#include "src/LittleFS.hpp"

#define CO2_SENSOR_ENABLED
//#define RH_SENSOR_ENABLED


#ifdef CO2_SENSOR_ENABLED
#include <MHZ19.h>
HardwareSerial ss(2);
MHZ19 mhz;

const unsigned long co2_ppm_meassurement_elapsed_millis = 1000;
unsigned long last_co2_measurement_time = 0;
#endif

int last_measured_co2_ppm = 0;
float last_measured_temp = -273.15f;

void mhz19_setup() {
#ifdef CO2_SENSOR_ENABLED
    ss.begin(9600);
    mhz.begin(ss);
    mhz.autoCalibration(false);
#endif
}

void mhz19_print_measurement() {
#ifdef CO2_SENSOR_ENABLED
    Serial.print(F("CO2: "));
    Serial.println(last_measured_co2_ppm);
    Serial.print(F("Temperature: "));
    Serial.println(last_measured_temp);
#endif
}

void mhz19_measure() {
#ifdef CO2_SENSOR_ENABLED
    unsigned long now = millis();
    unsigned long elapsed = now - last_co2_measurement_time;

    if (elapsed > co2_ppm_meassurement_elapsed_millis) {
        last_co2_measurement_time = now;
        last_measured_co2_ppm = mhz.getCO2();
        last_measured_temp = mhz.getTemperature();
    }
#endif
}

#ifdef RH_SENSOR_ENABLED
#include "Wire.h"
#include <SHT31.h>

#define SHT31_ADDRESS   0x44
#define I2C_SDA 23
#define I2C_SCL 22

SHT31 sht31;

#endif

float last_measured_rh_value = 0.0f;

void sht31_setup() {
#ifdef RH_SENSOR_ENABLED
    Wire.begin(I2C_SDA, I2C_SCL);
    sht31.begin(SHT31_ADDRESS);
    Wire.setClock(100000);

    uint16_t stat = sht31.readStatus();
    Serial.print(stat, HEX);
    Serial.println();

    sht31.requestData();
#endif
}

void sht31_print_measurement() {
#ifdef RH_SENSOR_ENABLED
    Serial.print(F("Relative Humidity: "));
    Serial.println(last_measured_rh_value);
    Serial.print(F("Temperature: "));
    Serial.println(last_measured_temp);
#endif
}

void sht31_measure() {
#ifdef RH_SENSOR_ENABLED
    if (sht31.dataReady())
    {
        bool success = sht31.readData();   

        if (success)
        {
            last_measured_rh_value = sht31.getHumidity();
            last_measured_temp = sht31.getTemperature();
        }

        sht31.requestData();
    }
#endif
}

void init_wifi() {
    WiFi.persistent(false);
    WiFi.disconnect();
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

    String mac = WiFi.macAddress();
    mac.replace(":", "");

    Serial.println("sensor-" + mac);

    if (!MDNS.begin("sensor-" + mac)) {
        Serial.println("Error setting up MDNS responder!");
    }
}

void setup() {
	Serial.begin(115200);
	Serial.println(F("Starting..."));

    mhz19_setup();
    sht31_setup();

    if (littlefs_read_config()) {
        init_wifi();
    }

    if (global_config_data.interval < 1) {
        global_config_data.interval = 1;
    }
}

void perform_measurements() {
    mhz19_measure();
    sht31_measure();
}

void print_measurements() {
    if (!Serial)
        return;

    mhz19_print_measurement();
    sht31_print_measurement();
}

void send_measurements() {
    if (WiFi.status() != WL_CONNECTED) {
        if (Serial) {
            Serial.print("send_measurements: WiFi not connected");
        }
        return;
    }

    HTTPClient http;
    
    String params = "?deviceId=" + urlEncode(WiFi.macAddress()) +
        "&co2=" + last_measured_co2_ppm +
        "&rh=" + last_measured_rh_value +
        "&temp=" + last_measured_temp;
    
    http.begin("http://" + global_config_data.destination_address + "/update" + params);
    http.setAuthorization(global_config_data.auth_user.c_str(), global_config_data.auth_password.c_str());
    http.setTimeout(20);

    int httpResponseCode = http.POST("");

    if (Serial) {
        if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String payload = http.getString();
            Serial.println(payload);
        }
        else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
    }

    http.end();
}

const unsigned long max_wifi_timeout_until_reconnect = 15000;
unsigned long require_wifi_reconnect_time = 0;

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

unsigned long next_measurements_send_time = 0;

void check_measurements() {
    perform_measurements();

    unsigned long now = millis();
    
    if (now > next_measurements_send_time) {
        next_measurements_send_time = now + (global_config_data.interval * 1000);

        print_measurements();
        send_measurements();
    }
}

void loop() {
    check_wifi(); 
    check_measurements();
}
