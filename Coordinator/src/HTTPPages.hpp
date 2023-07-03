#pragma once
#include <ESPAsyncWebSrv.h>

const int HTTP_OK = 200;
const int HTTP_OK_NO_CONTENT = 204;
const int HTTP_BAD_REQUEST = 400;
const int HTTP_FORBIDDEN = 403;
const int HTTP_INSUFFICIENT_STORAGE = 507;

void http_page_not_found(AsyncWebServerRequest* request);
void http_api_update(AsyncWebServerRequest* request);
void http_page_very_short_data(AsyncWebServerRequest* request);
void http_page_short_data(AsyncWebServerRequest* request);
void http_page_long_data(AsyncWebServerRequest* request);
void http_page_devices(AsyncWebServerRequest* request);
