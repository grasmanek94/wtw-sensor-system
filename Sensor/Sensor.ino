#include "src/constexpr_hash.hpp"
#include "src/HTTPPages.hpp"
#include "src/HumidityOffset.hpp"
#include "src/LittleFS.hpp"
#include "src/MyWifi.hpp"

// Select sensors which can be configured in json here
///////////////////////////////////////////////////////
#include "src/Sensor_S8.hpp"
#include "src/Sensor_SHT4X.hpp"
//#include "src/Sensor_MZH19.hpp"
#include "src/Sensor_SHT31.hpp"
///////////////////////////////////////////////////////

#include <ESPAsyncWebSrv.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <WiFi.h>
#include <Vector.h>

#include <Wire.h>
#define SENSOR_VERSION "1.6"

char* SENSOR_VERSION_STR = SENSOR_VERSION;
String SENSORS_LIST_STR("");
AsyncWebServer server(80);

static unsigned long uptime_ticks = 0;
static unsigned long next_measurements_send_time = 0;
static char data_measurements_string[2048]; // 2048 max http GET len

struct LocationMeasurement {
    bool present;
    int last_measured_co2_ppm;
    float last_measured_rh_value;
    float last_measured_temp ;
    int meter_status;
    bool temp_present;

    LocationMeasurement() :
        present{ false },
        last_measured_co2_ppm{ 0 },
        last_measured_rh_value{ 0.0f },
        last_measured_temp{ 0.0f },
        meter_status{ 0 },
        temp_present{ false }
    {}
};

const int MAX_SENSOR_INTERFACES_COUNT = 2;
Sensor_Interface* sensors_array[MAX_SENSOR_INTERFACES_COUNT];
Vector<Sensor_Interface*> sensors(sensors_array);

// inefficient but easier and faster
LocationMeasurement measurements[(int)SENSOR_LOCATION::NUM_LOCATIONS];

sensor_entry_callback switcher{
    [&](const String& type, const JsonVariant& value) {
        switch (constexpr_hash::hash(type)) {
#ifdef SENSOR_INTERFACE_S8_INCLUDED
        case "S8"_hash: {
            int location = value["location"];
            int hw_uart = value["hw_uart_nr"];

            sensors.push_back(new Sensor_S8(hw_uart, (SENSOR_LOCATION)location));
            break;
        }
#endif
#ifdef SENSOR_INTERFACE_SHT4X_INCLUDED
        case "SHT4X"_hash: {
            int location = value["location"];
            int sda = value["sda"];
            int scl = value["scl"];
            int precision = value["precision"];

            sensors.push_back(new Sensor_SHT4X(sda, scl, (sht4x_precision_t)precision, (SENSOR_LOCATION)location));
            break;
        }
#endif
#ifdef SENSOR_INTERFACE_SHT31_INCLUDED
        case "SHT31"_hash: {
            int location = value["location"];
            int sda = value["sda"];
            int scl = value["scl"];

            sensors.push_back(new Sensor_SHT31(sda, scl, (SENSOR_LOCATION)location));
            break;
        }
#endif
#ifdef SENSOR_INTERFACE_MHZ19_INCLUDED
        case "MHZ19"_hash: {
            int location = value["location"];
            int hw_uart = value["hw_uart_nr"];

            sensors.push_back(new Sensor_MHZ19(hw_uart, (SENSOR_LOCATION)location));
            break;
        }
#endif
        }
    }
};

void setup() {
	Serial.begin(115200);
	Serial.println(F("Starting..."));

    if (!littlefs_read_config(switcher)) {
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

void process_sensor_data(Sensor_Interface* sensor) {
    if (sensor->has_co2_ppm()) {
        measurements[(int)sensor->get_location()].present = true;
        measurements[(int)sensor->get_location()].last_measured_co2_ppm = sensor->get_co2_ppm();
    }

    if (sensor->has_temperature()) {
        measurements[(int)sensor->get_location()].present = true;
        measurements[(int)sensor->get_location()].temp_present = true;
        measurements[(int)sensor->get_location()].last_measured_temp = sensor->get_temperature();
    }

    if (sensor->has_relative_humidity()) {
        measurements[(int)sensor->get_location()].present = true;
        measurements[(int)sensor->get_location()].last_measured_rh_value = sensor->get_relative_humidity();
    }

    if (sensor->has_meter_status()) {
        measurements[(int)sensor->get_location()].present = true;
        measurements[(int)sensor->get_location()].meter_status = sensor->get_meter_status();
    }
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

void perform_http_request(const char* request) {
    HTTPClient http;

    http.begin(request);
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

bool fill_location_data(int location_index, int data_index, char* buffer, size_t buffer_size) {
    bool send = false;

    snprintf(buffer, buffer_size, "%s&loc=%d", buffer, data_index, (int)sensors.front()->get_location());

    float result_temp = measurements[location_index].temp_present ?
        global_config_data.temp_offset_x * measurements[location_index].last_measured_temp + global_config_data.temp_offset_y :
        0.0f;

    float result_rh = measurements[location_index].temp_present ?
        offset_relative_humidity(measurements[location_index].last_measured_rh_value, measurements[location_index].last_measured_temp, result_temp) :
        measurements[location_index].last_measured_rh_value;

    for (auto& sensor : sensors) {
        if (sensor->is_present()) {
            if (sensor->get_location() == (SENSOR_LOCATION)location_index) {
                if (sensor->has_co2_ppm()) {
                    send = true;
                    snprintf(buffer, buffer_size, "%s&co2[%d]=%d", buffer, data_index, measurements[location_index].last_measured_co2_ppm);
                }

                if (sensor->has_temperature()) {
                    send = true;
                    snprintf(buffer, buffer_size, "%s&temp[%d]=%.1f", buffer, data_index, result_temp);
                }

                if (sensor->has_relative_humidity()) {
                    send = true;
                    snprintf(buffer, buffer_size, "%s&rh[%d]=%.1f", buffer, data_index, result_rh);
                }

                if (sensor->has_meter_status()) {
                    send = true;
                    snprintf(buffer, buffer_size, "%s&status[%d]=%d", buffer, data_index, measurements[location_index].meter_status);
                }
            }
        }
    }

    return send;
}

void send_measurements() {
    if (!is_wifi_connected_debug()) {
        return;
    }

    uint8_t mac[6];
    bool send = false;
    int data_index = 0;

    ++uptime_ticks;

    WiFi.macAddress(mac);

    snprintf(data_measurements_string, sizeof(data_measurements_string), "http://%s/update?deviceId=%02X%02X%02X%02X%02X%02X&seqnr=%d", global_config_data.destination_address.c_str(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], uptime_ticks);

    for (int i = 0; i < (int)SENSOR_LOCATION::NUM_LOCATIONS; ++i) {
        if (!measurements[i].present) {
            continue;
        }

        if (fill_location_data(i, data_index, data_measurements_string, sizeof(data_measurements_string))) {
            send = true;
            measurements[i].present = false;
        }

        ++data_index;
    }
    
    Serial.println(data_measurements_string);

    if (!send) {
        return;
    }

    perform_http_request(data_measurements_string);
}

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
