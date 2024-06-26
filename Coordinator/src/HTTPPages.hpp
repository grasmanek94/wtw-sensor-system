#pragma once
#include <ESPAsyncWebSrv.h>

const int HTTP_OK = 200;
const int HTTP_OK_NO_CONTENT = 204;
const int HTTP_BAD_REQUEST = 400;
const int HTTP_FORBIDDEN = 403;
const int HTTP_CONFLICT = 409;
const int HTTP_UNPROCESSABLE_CONTENT = 422;
const int HTTP_INSUFFICIENT_STORAGE = 507;

void http_page_not_found(AsyncWebServerRequest* request);
void http_api_update(AsyncWebServerRequest* request);
void http_page_very_short_data(AsyncWebServerRequest* request);
void http_page_short_data(AsyncWebServerRequest* request);
void http_page_long_data(AsyncWebServerRequest* request);
void http_page_devices(AsyncWebServerRequest* request);
void http_page_flash(AsyncWebServerRequest* request);
void http_api_flash(AsyncWebServerRequest* request);
void http_api_flash_part(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
void http_api_config(AsyncWebServerRequest* request);
void http_page_config(AsyncWebServerRequest* request);
void check_http_pages_deferred_reset();
