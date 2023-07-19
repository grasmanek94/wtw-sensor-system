#include <Adafruit_SHT4x.h>
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

//#define RH_SENSOR_ENABLED
//#define CO2_SENSOR_ENABLED
#define BETTER_CO2_SENSOR_ENABLED
#ifdef BETTER_CO2_SENSOR_ENABLED
#undef CO2_SENSOR_ENABLED
#endif


//////////////////////////////////////////////////////////////

#ifdef CO2_SENSOR_ENABLED
#include <MHZ19.h>
MHZ19 mhz;
#elif defined(BETTER_CO2_SENSOR_ENABLED)
#include <s8_uart.h>

S8_UART* sensor_S8;
S8_sensor sensor_S8_data;
#endif

#if defined(CO2_SENSOR_ENABLED) || defined(BETTER_CO2_SENSOR_ENABLED)
HardwareSerial ss(2);

const unsigned long co2_ppm_meassurement_elapsed_millis = 4000;
unsigned long last_co2_measurement_time = 0;
const unsigned long co2_perform_manual_calibration_after_ms = 20 * 60 * 1000; // 20 min
bool manual_calibration_failure = false;
#endif

int last_measured_co2_ppm = 0;
float last_measured_temp = -273.15f;
unsigned char meter_status = 0;

void co2_sensor_setup() {
#ifdef CO2_SENSOR_ENABLED
    ss.begin(9600);
    mhz.begin(ss);
    mhz.autoCalibration(false);
#elif defined(BETTER_CO2_SENSOR_ENABLED)
    // Initialize S8 sensor
    ss.begin(S8_BAUDRATE);
    sensor_S8 = new S8_UART(ss);

    // Check if S8 is available
    sensor_S8->get_firmware_version(sensor_S8_data.firm_version);
    int len = strlen(sensor_S8_data.firm_version);
    if (len == 0) {
        Serial.println("SenseAir S8 CO2 sensor not found!");
        return;
    }

    // Show basic S8 sensor info
    Serial.println(">>> SenseAir S8 NDIR CO2 sensor <<<");
    printf("Firmware version: %s\n", sensor_S8_data.firm_version);
    sensor_S8_data.sensor_type_id = sensor_S8->get_sensor_type_ID();
    Serial.print("Sensor type: 0x"); printIntToHex(sensor_S8_data.sensor_type_id, 3); Serial.println("");

    // Setting ABC period
    Serial.println("Setting ABC period set to 180 hours...");
    sensor_S8->set_ABC_period(180);
    delay(1000);
    sensor_S8_data.abc_period = sensor_S8->get_ABC_period();
    if (sensor_S8_data.abc_period == 180) {
        Serial.println("ABC period set succesfully");
    }
    else {
        Serial.println("Error: ABC period doesn't set!");
    }

    if (!global_config_data.manual_calibration_performed) {
        last_measured_temp = -1.0f; // temporarily abuse this value because S8 doesn't have temp sensor readout
    }
#endif
}

void co2_sensor_print_measurement() {
#ifdef CO2_SENSOR_ENABLED
    Serial.print(F("CO2: "));
    Serial.println(last_measured_co2_ppm);
    Serial.print(F("Temperature: "));
    Serial.println(last_measured_temp);
#elif defined(BETTER_CO2_SENSOR_ENABLED)
    Serial.print(F("CO2: "));
    Serial.println(last_measured_co2_ppm);
#endif
}

void co2_sensor_measure() {
#ifdef CO2_SENSOR_ENABLED
    unsigned long now = millis();
    unsigned long elapsed = now - last_co2_measurement_time;

    if (elapsed > co2_ppm_meassurement_elapsed_millis) {
        last_co2_measurement_time = now;
        last_measured_co2_ppm = mhz.getCO2();
        last_measured_temp = mhz.getTemperature();
    }
#elif defined(BETTER_CO2_SENSOR_ENABLED)
    unsigned long now = millis();
    unsigned long elapsed = now - last_co2_measurement_time;

    if (!global_config_data.manual_calibration_performed && !manual_calibration_failure) {
        if (now > co2_perform_manual_calibration_after_ms) {
            Serial.println("Performing calibration..");
            global_config_data.manual_calibration_performed = sensor_S8->manual_calibration();

            if (global_config_data.manual_calibration_performed) {
                littlefs_write_config();
                littlefs_read_config();
                if (!global_config_data.manual_calibration_performed) {
                    Serial.println("Cannot save calibration result!");
                    last_measured_temp = -200.0f;
                }
                else {    
                    last_measured_temp = -273.15f;
                    Serial.println("Calibration success!");
                }
            }
            else {
                last_measured_temp = -100.0f;
                manual_calibration_failure = true;
                Serial.println("Calibration failure!");
            }
        }
    }

    if (elapsed > co2_ppm_meassurement_elapsed_millis) {
        last_co2_measurement_time = now;

        sensor_S8_data.co2 = sensor_S8->get_co2();
        last_measured_co2_ppm = sensor_S8_data.co2;

        sensor_S8_data.meter_status = sensor_S8->get_meter_status();
        meter_status = sensor_S8_data.meter_status;

        if (meter_status & S8_MASK_METER_ANY_ERROR) {
            Serial.println("One or more errors detected!");

            if (meter_status & S8_MASK_METER_FATAL_ERROR) {
                Serial.println("Fatal error in sensor!");
            }

            if (meter_status & S8_MASK_METER_OFFSET_REGULATION_ERROR) {
                Serial.println("Offset regulation error in sensor!");
            }

            if (meter_status & S8_MASK_METER_ALGORITHM_ERROR) {
                Serial.println("Algorithm error in sensor!");
            }

            if (meter_status & S8_MASK_METER_OUTPUT_ERROR) {
                Serial.println("Output error in sensor!");
            }

            if (meter_status & S8_MASK_METER_SELF_DIAG_ERROR) {
                Serial.println("Self diagnostics error in sensor!");
            }

            if (meter_status & S8_MASK_METER_OUT_OF_RANGE) {
                Serial.println("Out of range in sensor!");
            }

            if (meter_status & S8_MASK_METER_MEMORY_ERROR) {
                Serial.println("Memory error in sensor!");
            }
        }
    }
#endif
}

//////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////

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
    WiFi.begin(global_config_data.wifi_ssid, global_config_data.wifi_password);

    Serial.print("Connecting to WiFi '");
    Serial.print(global_config_data.wifi_ssid);
    Serial.print("' ..");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }

    Serial.println(WiFi.localIP());

    if (!MDNS.begin("sr-" + mac + ".local")) {
        Serial.println("Error setting up MDNS responder!");
    }
}

void setup() {
	Serial.begin(115200);
	Serial.println(F("Starting..."));

    if (littlefs_read_config()) {
        init_wifi();
        co2_sensor_setup();
        sht31_setup();
    }
    else {
        while (true) { delay(1000); }
    }

    if (global_config_data.interval < 1) {
        global_config_data.interval = 1;
    }
}

void perform_measurements() {
    co2_sensor_measure();
    sht31_measure();
}

void print_measurements() {
    if (!Serial)
        return;

    co2_sensor_print_measurement();
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
        "&temp=" + last_measured_temp +
        "&status=" + meter_status;
    
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
