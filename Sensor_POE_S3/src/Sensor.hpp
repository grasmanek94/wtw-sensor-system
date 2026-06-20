#include "SensorLocation.hpp"

#define SENSOR_VERSION "2.8"

extern const char* SENSOR_VERSION_STR;

struct LocationMeasurement {
    bool present;
    bool has_co2;
    bool has_rh;
    bool has_temp;
    bool has_meter_status;
    int last_measured_co2_ppm;
    float last_measured_rh_value;
    float last_measured_temp ;
    int meter_status;
    bool temp_present;

    LocationMeasurement() :
        present{ false },
        has_co2{false},
        has_rh{false},
        has_temp{false},
        has_meter_status{false},
        last_measured_co2_ppm{ 0 },
        last_measured_rh_value{ 0.0f },
        last_measured_temp{ 0.0f },
        meter_status{ 0 },
        temp_present{ false }
    {}
};

// inefficient but easier and faster
extern LocationMeasurement measurements[(int)SENSOR_LOCATION::NUM_LOCATIONS];
