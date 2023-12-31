﻿using System.ComponentModel;

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
            public bool added2;

            public FormPlotControlUpdater(SensorLocation sensor_location, Control control, PlotContainerSource container)
            {
                this.sensor_location = sensor_location.ToString();
                this.control = control;
                this.container = container;
                added = false;
                added2 = false;
            }

            public void CheckAdd(MeasurementsView view, string x_label, string y_label)
            {
                if (!added && view.Count > 0)
                {
                    added = true;
                    RunOn(control, () =>
                    {
                        container.Add(sensor_location, view);
                    });
                }

                var plot = container.Get(sensor_location);
                if (plot?.scatter_plot?.Axes?.XAxis?.Label?.Text != null)
                {
                    plot.scatter_plot.Axes.XAxis.Label.Text = x_label;
                }
                if (plot?.scatter_plot?.Axes?.YAxis?.Label?.Text != null)
                {
                    plot.scatter_plot.Axes.YAxis.Label.Text = y_label;
                }
            }

            public void CheckAdd2(MeasurementsView view, string extra, string x_label, string y_label)
            {
                if (!added2 && view.Count > 0)
                {
                    added2 = true;
                    RunOn(control, () =>
                    {
                        container.Add(sensor_location + " " + extra, view);
                    });
                }
            }
        }

        public string sensor_location;

        public BindingList<SensorMeasurement> measurements_vs;
        public BindingList<SensorMeasurement> measurements_s;
        public BindingList<SensorMeasurement> measurements_l;

        public int? measurements_l_teltime;

        public MeasurementsView mv_co2_ppm;
        public MeasurementsView mv_temperature;
        public MeasurementsView mv_relative_humidity;
        public MeasurementsView mv_ventilation_state;

        private FormPlotControlUpdater control_update_co2_ppm;
        private FormPlotControlUpdater control_update_temperature;
        private FormPlotControlUpdater control_update_relative_humidity;
        private FormPlotControlUpdater control_update_ventilation_state;

        private readonly IntegerClass max_relative_time;
        private Func<SensorMeasurement, double> relative_time_getter;
        private bool use_headroom;

        public FormDeviceMeasurementsPlotter(SensorLocation sensor_location, 
            Control update_control,
            PlotContainerSource plot_container_co2_ppm, PlotContainerSource plot_container_temperature, PlotContainerSource plot_container_relative_humidity, PlotContainerSource plot_container_ventilation_state)
        {
            this.sensor_location = sensor_location.ToString();

            measurements_vs = new ();
            measurements_s = new ();
            measurements_l = new ();

            measurements_l_teltime = null;

            max_relative_time = new();
            relative_time_getter = new(x => (x.relative_time - max_relative_time.Value) / 60.0);
            use_headroom = false;

            mv_co2_ppm = new MeasurementsView(measurements_vs, measurements_s, measurements_l, relative_time_getter, y => y.co2_ppm);
            mv_temperature = new MeasurementsView(measurements_vs, measurements_s, measurements_l, relative_time_getter, y => y.temp_c);
            mv_relative_humidity = new MeasurementsView(measurements_vs, measurements_s, measurements_l, relative_time_getter, y => y.rh);
            mv_ventilation_state = new MeasurementsView(measurements_vs, measurements_s, measurements_l, relative_time_getter, y => ((double)y.state_at_this_time));

            control_update_co2_ppm = new(sensor_location, update_control, plot_container_co2_ppm);
            control_update_temperature = new(sensor_location, update_control, plot_container_temperature);
            control_update_relative_humidity = new(sensor_location, update_control, plot_container_relative_humidity);
            control_update_ventilation_state = new(sensor_location, update_control, plot_container_ventilation_state);

            Refresh();
        }

        private bool Update(BindingList<SensorMeasurement> new_measurements, BindingList<SensorMeasurement> current_measurements)
        {
            current_measurements.Clear();

            foreach (SensorMeasurement measurement in new_measurements)
            {
                if (measurement.attainable_rh > 0.0f)
                {
                    use_headroom = true;
                }
                max_relative_time.max(measurement.relative_time);
                current_measurements.Add(measurement);
            }

            if (use_headroom)
            {
                mv_relative_humidity.SetYGetter(y => Math.Max(y.rh - y.attainable_rh, 0.0f));
                control_update_relative_humidity.container.y_min = -1.0f;
                control_update_relative_humidity.container.y_max = 50.0f;
            }
            else
            {
                mv_relative_humidity.SetYGetter(y => y.rh);
            }

            return new_measurements.Count > 0;
        }

        // keep appending data with this
        private bool Update(BindingList<SensorMeasurement> new_measurements, BindingList<SensorMeasurement> current_measurements, ref int? max_reltime, bool clr = false)
        {
            bool added_values = false;
            int new_max_reltime = max_reltime.HasValue ? max_reltime.Value : 0;

            if(clr)
            {
                new_max_reltime = 0;
                current_measurements.Clear();
            }

            foreach (SensorMeasurement measurement in new_measurements)
            {
                if(measurement.attainable_rh > 0.0f)
                {
                    use_headroom = true;
                }

                new_max_reltime = Math.Max(new_max_reltime, measurement.relative_time);
                if (max_reltime.HasValue)
                {
                    if(max_reltime.Value < measurement.relative_time)
                    {
                        max_relative_time.max(measurement.relative_time);
                        current_measurements.Add(measurement);
                        added_values = true;
                    }
                } 
                else
                {
                    max_relative_time.max(measurement.relative_time);
                    current_measurements.Add(measurement);
                    added_values = true;
                }
            }

            if(use_headroom)
            {
                mv_relative_humidity.SetYGetter(y => Math.Max(y.rh - y.attainable_rh, 0.0f));
                control_update_relative_humidity.container.y_min = -1.0f;
                control_update_relative_humidity.container.y_max = 50.0f;
            }
            else {
                mv_relative_humidity.SetYGetter(y => y.rh);
            }

            max_reltime = new_max_reltime;
            return added_values;
        }

        public void Update(BindingList<SensorMeasurement> new_measurements_vs, BindingList<SensorMeasurement> new_measurements_s, BindingList<SensorMeasurement> new_measurements_l)
        {
            bool vs = Update(new_measurements_vs, measurements_vs);
            bool s = Update(new_measurements_s, measurements_s);
            bool l = Update(new_measurements_l, measurements_l, ref measurements_l_teltime);

            if (vs || s || l)
            {
                Refresh();
            }
        }

        private void Refresh()
        {
            control_update_co2_ppm.CheckAdd(mv_co2_ppm, "time(min)", "CO2 PPM");
            control_update_temperature.CheckAdd(mv_temperature, "time(min)", "Temperature °C");
            control_update_relative_humidity.CheckAdd(mv_relative_humidity, "time(min)", use_headroom ? "Relative Humidity % (headroom)" : "Relative Humidity %");
            control_update_ventilation_state.CheckAdd(mv_ventilation_state, "time(min)", "Ventilation State");
        }
    }
}
