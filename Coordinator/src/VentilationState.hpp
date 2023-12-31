#pragma once

#include <cstdint>

enum requested_ventilation_state: uint8_t {
    requested_ventilation_state_undefined,
    requested_ventilation_state_low,
    requested_ventilation_state_medium,
    requested_ventilation_state_high
};

const unsigned long max_ventilation_status_http_request_timeout_s = 60;

void check_measurements();
