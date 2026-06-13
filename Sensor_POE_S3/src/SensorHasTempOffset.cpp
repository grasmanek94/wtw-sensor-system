#include "HumidityOffset.hpp"
#include "SensorHasTempOffset.hpp"

SensorHasTempOffset::SensorHasTempOffset(float offset_x, float offset_y): offset_x{ offset_x }, offset_y{ offset_y } {}
SensorHasTempOffset::~SensorHasTempOffset() {}

void SensorHasTempOffset::adjust_temp_and_humidity(float& in_out_temperature, float& in_out_humidity) {
    float input_temp = in_out_temperature;
    float input_humidity = in_out_humidity;

    in_out_temperature = offset_x * input_temp + offset_y;
    in_out_humidity = offset_relative_humidity(input_humidity, input_temp, input_temp);
}

float SensorHasTempOffset::adjust_temp(float in_temperature) {
    return offset_x * in_temperature + offset_y;
}
