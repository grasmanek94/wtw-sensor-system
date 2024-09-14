namespace CoordinatorViewer
{
    internal class Utils
    {
        public static bool CopyIfChanged(CoordinatorDeviceEntry dest, CoordinatorDeviceEntry src)
        {
            bool changed = false;
            if (src.sensor_location_id != dest.sensor_location_id)
            {
                dest.sensor_location_id = src.sensor_location_id;
                changed = true;
            }
            if (src.device_id != dest.device_id)
            {
                dest.device_id = src.device_id;
                changed = true;
            }
            if (src.current_ventilation_state_co2 != dest.current_ventilation_state_co2)
            {
                dest.current_ventilation_state_co2 = src.current_ventilation_state_co2;
                changed = true;
            }
            if (src.current_ventilation_state_rh != dest.current_ventilation_state_rh)
            {
                dest.current_ventilation_state_rh = src.current_ventilation_state_rh;
                changed = true;
            }
            if (src.is_associated != dest.is_associated)
            {
                dest.is_associated = src.is_associated;
                changed = true;
            }
            if (src.has_recent_data != dest.has_recent_data)
            {
                dest.has_recent_data = src.has_recent_data;
                changed = true;
            }
            if (src.very_short_count != dest.very_short_count)
            {
                dest.very_short_count = src.very_short_count;
                changed = true;
            }
            if (src.short_count != dest.short_count)
            {
                dest.short_count = src.short_count;
                changed = true;
            }
            if (src.long_count != dest.long_count)
            {
                dest.long_count = src.long_count;
                changed = true;
            }
            if (src.calculated_co2_ppm_low != dest.calculated_co2_ppm_low)
            {
                dest.calculated_co2_ppm_low = src.calculated_co2_ppm_low;
                changed = true;
            }
            if (src.calculated_co2_ppm_medium != dest.calculated_co2_ppm_medium)
            {
                dest.calculated_co2_ppm_medium = src.calculated_co2_ppm_medium;
                changed = true;
            }
            if (src.calculated_co2_ppm_high != dest.calculated_co2_ppm_high)
            {
                dest.calculated_co2_ppm_high = src.calculated_co2_ppm_high;
                changed = true;
            }
            if (src.relative_time != dest.relative_time)
            {
                dest.relative_time = src.relative_time;
                changed = true;
            }
            if (src.co2_ppm != dest.co2_ppm)
            {
                dest.co2_ppm = src.co2_ppm;
                changed = true;
            }
            if (src.rh != dest.rh)
            {
                dest.rh = src.rh;
                changed = true;
            }
            if (src.attainable_rh != dest.attainable_rh)
            {
                dest.attainable_rh = src.attainable_rh;
                changed = true;
            }
            if (src.temp_c != dest.temp_c)
            {
                dest.temp_c = src.temp_c;
                changed = true;
            }
            if (src.sensor_status != dest.sensor_status)
            {
                dest.sensor_status = src.sensor_status;
                changed = true;
            }
            if (src.sequence_number != dest.sequence_number)
            {
                dest.sequence_number = src.sequence_number;
                changed = true;
            }
            if (src.state_at_this_time != dest.state_at_this_time)
            {
                dest.state_at_this_time = src.state_at_this_time;
                changed = true;
            }

            return changed;
        }

    }
}
