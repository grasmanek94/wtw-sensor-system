#pragma once

#include <cmath>

inline float offset_relative_humidity(float measured_humidity, float measured_temp, float offset_temp) {
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
