#pragma once

enum requested_ventilation_state {
    requested_ventilation_state_undefined,
    requested_ventilation_state_low,
    requested_ventilation_state_medium,
    requested_ventilation_state_high
};

extern requested_ventilation_state old_ventilation_state;
const unsigned long max_ventilation_status_http_request_timeout_s = 5;

requested_ventilation_state get_highest_ventilation_state();
void check_measurements();
