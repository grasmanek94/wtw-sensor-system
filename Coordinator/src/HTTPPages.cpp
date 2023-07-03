#include "HTTPPages.hpp"

#include "DeviceData.hpp"
#include "LittleFS.hpp"

#include <ESPAsyncWebSrv.h>

const int INVALID_DEVICE_ID = -1;

int get_device_index(String identifier) {
    for (int i = SENSORS_COUNT; i >= 0; --i) {
        if (sensors[i].id == identifier) {
            return i;
        }
    }
    return INVALID_DEVICE_ID;
}

int get_device_index(AsyncWebServerRequest* request) {
    int device_index = INVALID_DEVICE_ID;
    bool success = false;

    AsyncWebParameter* param = request->getParam("id");
    if (param) {
        device_index = get_device_index(param->value());
        if (device_index >= 0 && device_index < SENSORS_COUNT) {
            return device_index;
        }
    }

    if (!success) {
        param = request->getParam("index");
        if (param) {
            device_index = param->value().toInt();
            if (device_index >= 0 && device_index < SENSORS_COUNT) {
                return device_index;
            }
        }
    }

    return INVALID_DEVICE_ID;
}

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

void http_api_update(AsyncWebServerRequest* request) {
    if (!check_auth(request)) {
        return;
    }

    int device_index = INVALID_DEVICE_ID;

    AsyncWebParameter* param = request->getParam("deviceId");
    if (!param) {
        request->send(HTTP_BAD_REQUEST, "text/plain");
        return;
    }

    String device_id = param->value();
    int free_id = INVALID_DEVICE_ID;

    for (int i = SENSORS_COUNT; i >= 0; --i) {
        if (!sensors[i].is_associated()) {
            free_id = i;
        }
        else if (sensors[i].id == device_id) {
            device_index = i;
            break;
        }
    }

    // device not found, check if we can add it, if yes do so
    if ((device_index == INVALID_DEVICE_ID) && (free_id != INVALID_DEVICE_ID)) {
        device_index = free_id;
        sensors[device_index].associate(device_id);
    }

    if (device_index == INVALID_DEVICE_ID) {
        request->send(HTTP_INSUFFICIENT_STORAGE, "text/plain");
        return;
    }

    float rh = 0.0f;
    float temp = 0.0f;
    int co2_ppm = 0;

    param = request->getParam("rh");
    if (param) {
        rh = param->value().toFloat();
    }

    param = request->getParam("temp");
    if (param) {
        temp = param->value().toFloat();
    }

    param = request->getParam("co2");
    if (param) {
        co2_ppm = param->value().toInt();
    }

    sensors[device_index].push(co2_ppm, rh, temp);

    request->send(HTTP_OK, "text/plain");
}

void http_page_devices(AsyncWebServerRequest* request) {
    if (!check_auth(request)) {
        return request->requestAuthentication();;
    }

    String devices = sensors[0].getHeaders();

    int device_index = get_device_index(request);
    if (device_index != INVALID_DEVICE_ID) {
        devices += sensors[device_index].toString(device_index);
        return request->send(HTTP_OK, "text/plain", devices);
    }
   
    for (int i = 0; i < SENSORS_COUNT; ++i) {
        auto& sensor = sensors[i];
        devices += sensors[i].toString(i);
    }

    request->send(HTTP_OK, "text/plain", devices);
}

void http_page_very_short_data(AsyncWebServerRequest* request) {   
    if (!check_auth(request)) {
        return request->requestAuthentication();;
    }

    int device_index = get_device_index(request);
    if (device_index == INVALID_DEVICE_ID) {
        return request->send(HTTP_BAD_REQUEST, "text/plain");
    }

    device_data& sensor = sensors[device_index];
    auto& input_data = sensor.very_short_data;

    if (input_data.isEmpty()) {
        return request->send(HTTP_OK_NO_CONTENT, "text/plain");
    }

    String data;
    for (int i = 0; i < input_data.size(); ++i) {
        if (i == 0) {
            data += input_data[i].getHeaders();
        }

        data += input_data[i].toString();
    }
    
    request->send(HTTP_OK, "text/plain", data);

    /*AsyncWebServerResponse* response = request->beginChunkedResponse("text/plain", [sensor](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
        String data;
        if (index == 0) {
            data += sensor->very_short_data[index].getHeaders();
        }

        data += sensor->very_short_data[index].toString();
        int size = max(data.length(), maxLen);

        memcpy(buffer, data.c_str(), size);
        //buffer[size - 1] = 0;

        return size;
    });

    response->setCode(HTTP_OK);
    response->setContentType("text/plain");
    request->send(response);*/
}

void http_page_short_data(AsyncWebServerRequest* request) {
    if (!check_auth(request)) {
        return request->requestAuthentication();;
    }

    int device_index = get_device_index(request);
    if (device_index == INVALID_DEVICE_ID) {
        return request->send(HTTP_BAD_REQUEST, "text/plain");
    }

    device_data* sensor = &sensors[device_index];
    auto input_data = sensor->short_data;

    if (input_data.isEmpty()) {
        return request->send(HTTP_OK_NO_CONTENT, "text/plain");
    }

    String data;
    for (int i = 0; i < input_data.size(); ++i) {
        if (i == 0) {
            data += input_data[i].getHeaders();
        }

        data += input_data[i].toString();
    }

    request->send(HTTP_OK, "text/plain", data);

}

void http_page_long_data(AsyncWebServerRequest* request) {
    if (!check_auth(request)) {
        return request->requestAuthentication();;
    }

    int device_index = get_device_index(request);
    if (device_index == INVALID_DEVICE_ID) {
        return request->send(HTTP_BAD_REQUEST, "text/plain");
    }

    device_data* sensor = &sensors[device_index];
    auto input_data = sensor->long_data;

    if (input_data.isEmpty()) {
        return request->send(HTTP_OK_NO_CONTENT, "text/plain");
    }

    String data;
    for (int i = 0; i < input_data.size(); ++i) {
        if (i == 0) {
            data += input_data[i].getHeaders();
        }

        data += input_data[i].toString();
    }

    request->send(HTTP_OK, "text/plain", data);
}
