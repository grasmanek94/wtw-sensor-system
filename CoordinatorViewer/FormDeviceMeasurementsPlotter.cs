using ScottPlot;
using System.ComponentModel;
using System.Diagnostics.Metrics;

namespace CoordinatorViewer
{
    class FormDeviceMeasurementsPlotter
    {
        private static void RunOn(Control which, Action action)
        {
            if (which.InvokeRequired)
            {
                which.Invoke(action);
            }
            else
            {
                action();
            }
        }

        private class FormPlotControlUpdater
        {
            public string sensor_location;
            public Control control;
            public PlotContainerSource container;
            public bool added;

            public FormPlotControlUpdater(SensorLocation sensor_location, Control control, PlotContainerSource container)
            {
                this.sensor_location = sensor_location.ToString();
                this.control = control;
                this.container = container;
                added = false;
            }

            public void CheckAdd(string x_label, string y_label)
            {
                if (!added)
                {
                    added = true;
                    RunOn(control, () =>
                    {
                        container.Get(sensor_location);
                    });
                }

                //if (container?.forms_plot?.Plot?.Axes?.GetAxes()?.Where(axis => axis is IXAxis)?.First()?.Label?.Text != null)
                //{
                //   container.forms_plot.Plot.Axes.GetAxes().Where(axis => axis is IXAxis).First().Label.Text = x_label;
                //}

                
                if(container?.forms_plot?.Plot?.Axes?.Left?.Label?.Text != null)
                {
                    container.forms_plot.Plot.Axes.Left.Label.Text = y_label;
                }
            }
        }

        public string sensor_location;
        public int? measurements_l_teltime;

        private FormPlotControlUpdater control_update_co2_ppm;
        private FormPlotControlUpdater control_update_temperature;
        private FormPlotControlUpdater control_update_relative_humidity;
        private FormPlotControlUpdater control_update_ventilation_state;
        private CoordinatorTimeOffset time_offset;

        private bool use_headroom;

        public FormDeviceMeasurementsPlotter(SensorLocation sensor_location, 
            Control update_control,
            PlotContainerSource plot_container_co2_ppm, PlotContainerSource plot_container_temperature, PlotContainerSource plot_container_relative_humidity, PlotContainerSource plot_container_ventilation_state, CoordinatorTimeOffset time_offset)
        {
            this.sensor_location = sensor_location.ToString();
            this.time_offset = time_offset;

            measurements_l_teltime = null;

            use_headroom = false;

            control_update_co2_ppm = new(sensor_location, update_control, plot_container_co2_ppm);
            control_update_temperature = new(sensor_location, update_control, plot_container_temperature);
            control_update_relative_humidity = new(sensor_location, update_control, plot_container_relative_humidity);
            control_update_ventilation_state = new(sensor_location, update_control, plot_container_ventilation_state);

            Refresh();
        }

        // keep appending data with this
        private bool Update(IEnumerable<SensorMeasurement> new_measurements, bool clr = false)
        {
            bool added_values = false;
            int new_max_reltime = measurements_l_teltime.HasValue ? measurements_l_teltime.Value : 0;

            if(clr)
            {
                new_max_reltime = 0;
            }

            foreach (SensorMeasurement measurement in new_measurements)
            {
                float rh = measurement.rh;
                if(measurement.attainable_rh > 0.0f)
                {
                    use_headroom = true;
                    rh = Math.Max(measurement.rh - measurement.attainable_rh, 0.0f);
                }

                new_max_reltime = Math.Max(new_max_reltime, measurement.relative_time);
                if ((measurements_l_teltime.HasValue && measurements_l_teltime.Value < measurement.relative_time) || (!measurements_l_teltime.HasValue))
                {
                    double date_time = time_offset.GetDate(measurement.relative_time).ToOADate();
                    control_update_co2_ppm.container.Get(sensor_location).Add(date_time, measurement.co2_ppm);
                    control_update_temperature.container.Get(sensor_location).Add(date_time, measurement.temp_c);
                    control_update_relative_humidity.container.Get(sensor_location).Add(date_time, measurement.attainable_rh);
                    control_update_ventilation_state.container.Get(sensor_location).Add(date_time, (int)measurement.state_at_this_time);

                    added_values = true;
                } 
            }

            measurements_l_teltime = new_max_reltime;
            return added_values;
        }

        public void Update(IEnumerable<SensorMeasurement> new_measurements_vs, IEnumerable<SensorMeasurement> new_measurements_s, IEnumerable<SensorMeasurement> new_measurements_l)
        {
            var all = new_measurements_l.Concat(new_measurements_s).Concat(new_measurements_vs).OrderBy(measurement => measurement.relative_time);

            //bool l = Update(new_measurements_l, true);
            //bool s = Update(new_measurements_s);
            //bool vs = Update(new_measurements_vs);


            //if (vs || s || l)
            if(Update(all))
            {
                Refresh();
            }
        }

        public void Update(CoordinatorDeviceEntry measurement)
        {
            float rh = measurement.rh;
            if (measurement.attainable_rh > 0.0f)
            {
                use_headroom = true;
                rh = Math.Max(measurement.rh - measurement.attainable_rh, 0.0f);
            }

            if ((measurements_l_teltime.HasValue && measurements_l_teltime.Value < measurement.relative_time) || (!measurements_l_teltime.HasValue))
            {
                double date_time = time_offset.GetDate(measurement.relative_time).ToOADate();
                control_update_co2_ppm.container.Get(sensor_location).Add(date_time, measurement.co2_ppm);
                control_update_temperature.container.Get(sensor_location).Add(date_time, measurement.temp_c);
                control_update_relative_humidity.container.Get(sensor_location).Add(date_time, measurement.attainable_rh);
                control_update_ventilation_state.container.Get(sensor_location).Add(date_time, Math.Max((int)measurement.current_ventilation_state_co2, (int)measurement.current_ventilation_state_rh));

                Refresh();
            }

            if (measurements_l_teltime.HasValue)
            {
                measurements_l_teltime = Math.Max(measurements_l_teltime.Value, measurement.relative_time);
            }
        }

        private void Refresh()
        {
            if (use_headroom)
            {
                control_update_relative_humidity.container.y_min = 0.0;
                control_update_relative_humidity.container.y_max = 50.0;
                //control_update_relative_humidity.container.plots.ElementAt(0).Value.ManageAxisLimits = true;
                control_update_relative_humidity.container.plots.ElementAt(0).Value.Axes.YAxis.Min = 0.0;
                control_update_relative_humidity.container.plots.ElementAt(0).Value.Axes.YAxis.Max = 50.0;
            }

            control_update_co2_ppm.CheckAdd("date/time", "CO2 PPM");
            control_update_temperature.CheckAdd("date/time", "Temperature °C");
            control_update_relative_humidity.CheckAdd("date/time", use_headroom ? "Relative Humidity % (headroom)" : "Relative Humidity %");
            control_update_ventilation_state.CheckAdd("date/time", "Ventilation State");
        }
    }
}
