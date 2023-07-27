#include <ESPAsyncWebSrv.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <WiFi.h>

#include "src/HTTPPages.hpp"
#include "src/LittleFS.hpp"
#include "src/MyWifi.hpp"

#include "src/Sensor_Null.hpp"

// Select connected sensors here
//#include "src/Sensor_S8.hpp"
//#include "src/Sensor_SHT4X.hpp"
//#include "src/Sensor_MZH19.hpp"
//#include "src/Sensor_SHT31.hpp"

#define SENSOR_VERSION "1.5"

char* SENSOR_VERSION_STR = SENSOR_VERSION;
String SENSORS_LIST_STR("");
AsyncWebServer server(80);

Sensor_Interface* sensors[] = 
{ 
#ifdef SENSOR_INTERFACE_S8_INCLUDED
    new Sensor_S8(), 
#endif
#ifdef SENSOR_INTERFACE_SHT4X_INCLUDED
    new Sensor_SHT4X(),
#endif
#ifdef SENSOR_INTERFACE_SHT31_INCLUDED
    new Sensor_SHT31(),
#endif
#ifdef SENSOR_INTERFACE_MHZ19_INCLUDED
    new Sensor_MHZ19(),
#endif
    new Sensor_Null()
};

void setup() {
	Serial.begin(115200);
	Serial.println(F("Starting..."));

    if (!littlefs_read_config()) {
        while (true) {
            Serial.println(F("Stalled"));
            delay(10000);
        }
    }

    init_wifi();
    for (auto& sensor : sensors) {
        sensor->setup();
        String name = sensor->get_name();
        if (name.length() > 0) {
            if (SENSORS_LIST_STR.length() > 0) {
                SENSORS_LIST_STR += "/";
            }
            SENSORS_LIST_STR += sensor->get_name();
        }
    }

    server.onNotFound(http_page_not_found);
    server.on("/", HTTP_GET, http_page_flash);
    server.on("/", HTTP_POST, http_api_flash, http_api_flash_part);

    server.on("/config", HTTP_GET, http_page_config);
    server.on("/config", HTTP_POST, http_api_config);

    server.begin();

    if (global_config_data.interval < 1) {
        global_config_data.interval = 1;
    }
}

void print_measurements() {
    if (!Serial)
        return;

    for (auto& sensor : sensors) {
        sensor->print_measurement();
    }
}

int last_measured_co2_ppm = 0;
float last_measured_rh_value = 0.0f;
float last_measured_temp = 0.0f;
int meter_status =
                    #ifdef SENSOR_INTERFACE_S8_INCLUDED
                    0xFF
                    #else
                    0x00
                    #endif
                    ;

void process_sensor_data(Sensor_Interface* sensor) {
    if (sensor->has_co2_ppm()) {
        last_measured_co2_ppm = sensor->get_co2_ppm();
    }

    if (sensor->has_temperature()) {
        last_measured_temp = sensor->get_temperature();
    }

    if (sensor->has_relative_humidity()) {
        last_measured_rh_value = sensor->get_relative_humidity();
    }

    if (sensor->has_meter_status()) {
        meter_status = sensor->get_meter_status();
    }

#ifdef SENSOR_INTERFACE_S8_INCLUDED
    if (sensor->get_name() == "S8") { // dynamic_cast unavailable
        Sensor_S8* sensor_s8 = static_cast<Sensor_S8*>(sensor);
        if (sensor_s8 != nullptr) {
            auto calibration_status = sensor_s8->get_calibration_error();
            if (sensor_s8->get_abc_status() == Sensor_S8::ABC_STATUS::FAIL) {
                meter_status |= 0x10000000;
            }
            switch (calibration_status) {
            case Sensor_S8::CALIBRATION_STATUS::CALIBRATION_SUCCESS_BUT_FAILED_TO_SAVE_TO_DISK:
                meter_status |= 0x20000000;
                break;

            case Sensor_S8::CALIBRATION_STATUS::MANUAL_CALIBRATION_NOT_PERFORMED_YET:
                meter_status |= 0x40000000;
                break;

            case Sensor_S8::CALIBRATION_STATUS::SENSOR_CALIBRATION_FAILED:
                meter_status |= 0x80000000;
                break;
            }
        }
    }
#endif
}

void perform_measurements() {
    for (auto& sensor : sensors) {
        if (sensor->is_present()) {
            sensor->update();
            if (sensor->has_new_data()) {
                process_sensor_data(sensor);

            }
        }
    }
}

float offset_relative_humidity(float measured_humidity, float measured_temp, float offset_temp) {
    // https://engineering.stackexchange.com/a/49581/27370
    const float T1 = measured_temp;
    const float RH1 = measured_humidity;
    const float T2 = offset_temp;

    if ((T2 + 273.15f) == 0.0f || 
        (T1 + 273.15f) == 0.0f || 
        (T2 + 243.5f) == 0.0f || 
        (T2 + 273.15f) == 0.0f || 
        measured_temp == offset_temp) {
        return RH1;
    }

    const float M1 = 6.112f * std::exp((17.67f * T1) / (T1 + 243.5f)) * RH1 * 18.02f / ((273.15f + T1) * 100.0f * 0.08314f);
    const float M2 = M1 / ((T2 + 273.15f) / (T1 + 273.15f));
    const float RH2 = M2 * std::exp(-(17.67f * T2) / (T2 + 243.5f)) * (0.075487f * T2 + 20.6193f);

    return RH2;
}

unsigned long uptime_ticks = 0;
void send_measurements() {
    if (!is_wifi_connected_debug()) {
        return;
    }

    HTTPClient http;
    
    ++uptime_ticks;

    float result_temp = global_config_data.temp_offset_x * last_measured_temp + global_config_data.temp_offset_y;
    float result_rh = offset_relative_humidity(last_measured_rh_value, last_measured_temp, result_temp);

    String params = "?deviceId=" + urlEncode(WiFi.macAddress()) +
        "&co2=" + last_measured_co2_ppm +
        "&rh=" + result_rh +
        "&temp=" + result_temp +
        "&status=" + meter_status + 
        "&seqnr=" + uptime_ticks;
    
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

static unsigned long next_measurements_send_time = 0;

void check_measurements() {
    unsigned long now = millis();
    
    if (now > next_measurements_send_time) {
        next_measurements_send_time = now + (global_config_data.interval * 1000);

        print_measurements();
        send_measurements();
    }
}

void loop() {
    check_wifi(); 
    perform_measurements();
    check_measurements();
}
