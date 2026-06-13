#pragma once

#ifndef SENSOR_HAS_TEMP_OFFSET_INCLUDED
#define SENSOR_HAS_TEMP_OFFSET_INCLUDED 1
#endif

class SensorHasTempOffset  {
private:
	float offset_x;
	float offset_y;

protected:
	SensorHasTempOffset(float offset_x, float offset_y);
	virtual ~SensorHasTempOffset();

	void adjust_temp_and_humidity(float& in_out_temperature, float& in_out_humidity);
	float adjust_temp(float in_temperature);
};
