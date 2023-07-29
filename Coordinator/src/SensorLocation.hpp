#pragma once

#include <cstdint>

enum class SENSOR_LOCATION : uint8_t {
	LIVING_ROOM,
	BATHROOM,
	BEDROOM_FRONT,
	BEDROOM_LEFT,
	BEDROOM_RIGHT,
	NEW_AIR_INLET,
	NUM_LOCATIONS
};
