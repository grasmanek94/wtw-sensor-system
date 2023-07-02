#define CO2_SENSOR_ENABLED
//#define RH_SENSOR_ENABLED

#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "FS.h"

#include "src/LittleFS.hpp"

#ifdef CO2_SENSOR_ENABLED
#include <MHZ19.h>
HardwareSerial ss(2);
MHZ19 mhz;

const unsigned long co2_ppm_meassurement_elapsed_millis = 1000;
unsigned long last_co2_measurement_time = 0;
int last_measured_co2_ppm = 0;
float last_measured_temp = -273.15f;

void mhz19_setup() {
    ss.begin(9600);
    mhz.begin(ss);
    mhz.autoCalibration(false);
}

void mhz19_print_measurement() {
    Serial.print(F("CO2: "));
    Serial.println(last_measured_co2_ppm);
    Serial.print(F("Temperature: "));
    Serial.println(last_measured_temp);
}

void mhz19_measure() {
    unsigned long now = millis();
    unsigned long elapsed = now - last_co2_measurement_time;

    if (elapsed > co2_ppm_meassurement_elapsed_millis) {
        last_co2_measurement_time = now;
        last_measured_co2_ppm = mhz.getCO2();
        last_measured_temp = mhz.getTemperature();
    }
}

#endif

#ifdef RH_SENSOR_ENABLED
#include "Wire.h"
#include <SHT31.h>

#define SHT31_ADDRESS   0x44
#define I2C_SDA 23
#define I2C_SCL 22

SHT31 sht31;
float last_measured_rh_value = 0.0f;

void sht31_setup() {
    Wire.begin(I2C_SDA, I2C_SCL);
    sht31.begin(SHT31_ADDRESS);
    Wire.setClock(100000);

    uint16_t stat = sht31.readStatus();
    Serial.print(stat, HEX);
    Serial.println();

    sht31.requestData();
}

void sht31_print_measurement() {
    Serial.print(F("Relative Humidity: "));
    Serial.println(last_measured_rh_value);
}

void sht31_measure() {
    if (sht31.dataReady())
    {
        bool success = sht31.readData();   
        sht31.requestData();

        if (success == false)
        {
            last_measured_rh_value = sht31.getHumidity();
        }
    }
}

#endif

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
}

void setup() {
	Serial.begin(115200);
	Serial.println(F("Starting..."));

#ifdef CO2_SENSOR_ENABLED
    mhz19_setup();
#endif

#ifdef RH_SENSOR_ENABLED
    sht31_setup();
#endif

    if (littlefs_read_config()) {
        init_wifi();
    }

    if (global_config_data.interval < 1) {
        global_config_data.interval = 1;
    }
}

void perform_measurements() {
#ifdef CO2_SENSOR_ENABLED
    mhz19_measure();
#endif

#ifdef RH_SENSOR_ENABLED
    sht31_measure();
#endif
}

void print_measurements() {
    if (!Serial)
        return;

#ifdef CO2_SENSOR_ENABLED
    mhz19_print_measurement();
#endif

#ifdef RH_SENSOR_ENABLED
    sht31_print_measurement();
#endif
}

void send_measurements() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(global_config_data.destination_address);
        http.setAuthorization(global_config_data.auth_user.c_str(), global_config_data.auth_password.c_str());
        http.setTimeout(5);

        int co2_ppm = 0;
        float rel_hum_perc = 0.0f;

#ifdef CO2_SENSOR_ENABLED
        co2_ppm = last_measured_co2_ppm;
#endif
#ifdef RH_SENSOR_ENABLED
        rel_hum_perc = last_measured_rh_value;
#endif

        int httpResponseCode = http.POST(
            "deviceId=" + WiFi.macAddress() + 
            "&co2=" + co2_ppm +
            "&rh=" + rel_hum_perc + 
            "&deviceName=" + global_config_data.device_custom_name
        );

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
