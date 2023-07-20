#pragma once

#include "Sensor_Interface.hpp"

#ifndef SENSOR_INTERFACE_NULL_INCLUDED
#define SENSOR_INTERFACE_NULL_INCLUDED 1
#endif

class Sensor_Null : public Sensor_Interface {

public:
	Sensor_Null() {};
	virtual ~Sensor_Null() {};

	virtual bool is_present() const override { return false; }
};
