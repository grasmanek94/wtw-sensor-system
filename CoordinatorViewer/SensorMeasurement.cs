namespace CoordinatorViewer
{
    internal class SensorMeasurement
    {
        public int relative_time { get; set; }
        public int co2_ppm { get; set; }
        public float rh { get; set; }
        public float attainable_rh { get; set; }
        public float temp_c { get; set; }
        public int sensor_status { get; set; }
        public int sequence_number { get; set; }
        public VentilationState state_at_this_time { get; set; }
    }
}
