#pragma once

#include <cstdint>

union MeterStatusUnion {
	struct {
		uint8_t meter_status : 8;
		uint8_t calibration_status : 4;
		uint8_t abc_status : 4;
	} split;
	uint16_t combined;
};
