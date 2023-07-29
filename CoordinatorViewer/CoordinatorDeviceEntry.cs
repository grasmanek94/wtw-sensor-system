namespace CoordinatorViewer
{
    internal class CoordinatorDeviceEntry
    {
        public SensorLocation sensor_location_id { get; set; }
        public string device_id { get; set; }
        public VentilationState current_ventilation_state_co2 { get; set; }
        public VentilationState current_ventilation_state_rh { get; set; }
        public bool is_associated { get; set; }
        public bool has_recent_data { get; set; }
        public int very_short_count { get; set; }
        public int short_count { get; set; }
        public int long_count { get; set; }
        public int relative_time { get; set; }
        public int co2_ppm { get; set; }
        public float rh { get; set; }
        public float temp_c { get; set; }
        public int sensor_status { get; set; }
        public int sequence_number { get; set; }
        public VentilationState state_at_this_time { get; set; }

        public override int GetHashCode()
        {
            return (int)sensor_location_id;
        }
    }
}
