using System.ComponentModel;

namespace CoordinatorViewer
{
    public partial class FormAllDevicesViewer
    {
        private class FormDeviceMeasurementsPlotter
        {
            private class FormPlotControlUpdater
            {
                public string device_id;
                public Control control;
                public PlotContainerSource container;
                public bool added;

                public FormPlotControlUpdater(string device_id, Control control, PlotContainerSource container)
                {
                    this.device_id = device_id;
                    this.control = control;
                    this.container = container;
                    added = false;
                }

                public void CheckAdd(MeasurementsView view, string x_label, string y_label)
                {
                    if (!added && view.Count > 0)
                    {
                        added = true;
                        RunOn(control, () =>
                        {
                            container.Add(device_id, view);
                        });
                    }

                    var plot = container.Get(device_id);
                    if (plot?.scatter_plot?.Axes?.XAxis?.Label?.Text != null)
                    {
                        plot.scatter_plot.Axes.XAxis.Label.Text = x_label;
                    }
                    if (plot?.scatter_plot?.Axes?.YAxis?.Label?.Text != null)
                    {
                        plot.scatter_plot.Axes.YAxis.Label.Text = y_label;
                    }
                }
            }

            public string device_id;

            public BindingList<SensorMeasurement> measurements_vs;
            public BindingList<SensorMeasurement> measurements_s;
            public BindingList<SensorMeasurement> measurements_l;

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

            public FormDeviceMeasurementsPlotter(string device_id,
                BindingList<SensorMeasurement> vs, BindingList<SensorMeasurement> s, BindingList<SensorMeasurement> l, 
                Control update_control,
                PlotContainerSource plot_container_co2_ppm, PlotContainerSource plot_container_temperature, PlotContainerSource plot_container_relative_humidity, PlotContainerSource plot_container_ventilation_state)
            {
                this.device_id = device_id;

                measurements_vs = vs;
                measurements_s = s;
                measurements_l = l;

                max_relative_time = new();
                max_relative_time.max(measurements_vs.Last().relative_time);
                relative_time_getter = new(x => (x.relative_time - max_relative_time.Value) / 60.0);
                //relative_time_getter = new(x => (x.relative_time / 60.0));

                mv_co2_ppm = new MeasurementsView(measurements_vs, measurements_s, measurements_l, relative_time_getter, y => y.co2_ppm);
                mv_temperature = new MeasurementsView(measurements_vs, measurements_s, measurements_l, relative_time_getter, y => y.temp_c);
                mv_relative_humidity = new MeasurementsView(measurements_vs, measurements_s, measurements_l, relative_time_getter, y => y.rh);
                mv_ventilation_state = new MeasurementsView(measurements_vs, measurements_s, measurements_l, relative_time_getter, y => ((double)y.state_at_this_time));

                control_update_co2_ppm = new(device_id, update_control, plot_container_co2_ppm);
                control_update_temperature = new(device_id, update_control, plot_container_temperature);
                control_update_relative_humidity = new(device_id, update_control, plot_container_relative_humidity);
                control_update_ventilation_state = new(device_id, update_control, plot_container_ventilation_state);

                Refresh();
            }

            public void Add(SensorMeasurement measurement)
            {
                if (measurements_vs.Last().sequence_number == measurement.sequence_number)
                {
                    return;
                }

                max_relative_time.max(measurement.relative_time);
                measurements_vs.Add(measurement);

                Refresh();
            }

            private void Refresh()
            {
                control_update_co2_ppm.CheckAdd(mv_co2_ppm, "time(min)", "CO2 PPM");
                control_update_temperature.CheckAdd(mv_temperature, "time(min)", "Temperature °C");
                control_update_relative_humidity.CheckAdd(mv_relative_humidity, "time(min)", "Relative Humidity %");
                control_update_ventilation_state.CheckAdd(mv_ventilation_state, "time(min)", "Ventilation State");
            }
        }
    }
}
