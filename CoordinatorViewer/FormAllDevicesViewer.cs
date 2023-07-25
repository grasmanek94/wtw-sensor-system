using System;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Numerics;
using OpenTK.Graphics.ES10;
using ScottPlot;
using ScottPlot.Axis;
using ScottPlot.DataSources;
using ScottPlot.WinForms;

namespace CoordinatorViewer
{
    public partial class FormAllDevicesViewer : Form
    {
        private System.Timers.Timer timer;
        private CoordinatorData coordinator_data;
        private readonly BindingList<CoordinatorDeviceEntry> device_entries_list;
        private readonly Dictionary<int, int> device_entries_list_indexes;
        private readonly PlotContainer pc_vent_state;
        private readonly PlotContainer pc_temp;
        private readonly PlotContainer pc_co2_ppm;
        private readonly PlotContainer pc_rh;
        private readonly List<PlotContainer> pc_list;
        
        private class IntegerClass
        {
            public int Value { get; set; } = 0;
            public void max(int input)
            {
                Value = Math.Max(Value, input);
            }
        }

        private readonly IntegerClass max_relative_time;

        public FormAllDevicesViewer()
        {
            InitializeComponent();

            coordinator_data = new();
            device_entries_list = new();
            device_entries_list_indexes = new();
            data_grid.DataSource = device_entries_list;
            data_grid.AutoGenerateColumns = true;
            data_grid.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
            data_grid.AllowUserToAddRows = false;
            data_grid.EditMode = DataGridViewEditMode.EditProgrammatically;

            pc_vent_state = new(-1, 4);
            pc_temp = new(10.0, 35.0);
            pc_co2_ppm = new(0.0, 1600.0);
            pc_rh = new(10.0, 95.0);

            pc_list = new()
            {
                pc_vent_state,
                pc_temp,
                pc_co2_ppm,
                pc_rh
            };

            max_relative_time = new();

            timer = new();
            timer.Elapsed += Start;
            timer.Interval = 1000;
            timer.Start();

            data_grid.SelectionChanged += DeviceSelectionChanged;

            AddPlotToTable();
        }

        private void AddPlotToTable()
        {
            foreach (var pc in pc_list)
            {
                plots_panel.Controls.Add(pc.forms_plot);
            }

            plots_panel.Refresh();
        }

        private async Task<bool> UpdateGraphs()
        {
            RunOn(plots_panel, () =>
            {
                foreach (var pc in pc_list)
                {
                    pc.Clear();
                }
            });

            foreach (var entry in device_entries_list)
            {
                if (!entry.is_associated)
                {
                    continue;
                }

                var measurements_vs = await coordinator_data.GetVeryShortMeasurements(entry.device_id);
                var measurements_s = await coordinator_data.GetShortMeasurements(entry.device_id);
                var measurements_l = await coordinator_data.GetLongMeasurements(entry.device_id);
                max_relative_time.max(measurements_vs.Last().relative_time);
                // int mrt = measurements_vs.Last().relative_time;

                Func<SensorMeasurement, double> relative_time_getter = x => (x.relative_time - max_relative_time.Value) / 60.0;

                var mv_co2 = new MeasurementsView(measurements_vs, measurements_s, measurements_l, relative_time_getter, y => y.co2_ppm);
                var mv_temp = new MeasurementsView(measurements_vs, measurements_s, measurements_l, relative_time_getter, y => y.temp_c);
                var mv_rh = new MeasurementsView(measurements_vs, measurements_s, measurements_l, relative_time_getter, y => y.rh);
                var mv_vs = new MeasurementsView(measurements_vs, measurements_s, measurements_l, relative_time_getter, y => ((double)y.state_at_this_time));

                RunOn(plots_panel, () =>
                {
                    if (mv_co2.Count > 0)
                    {
                        pc_co2_ppm.Add(entry.device_id).Add(mv_co2);
                    }

                    if (mv_temp.Count > 0)
                    {
                        pc_temp.Add(entry.device_id).Add(mv_temp);
                    }

                    if (mv_rh.Count > 0)
                    {
                        pc_rh.Add(entry.device_id).Add(mv_rh);
                    }

                    if (mv_vs.Count > 0)
                    {
                        pc_vent_state.Add(entry.device_id).Add(mv_vs);
                    }
                });
            }

            RunOn(plots_panel, () =>
            {
                foreach (var pc in pc_list)
                {
                    pc.Fit();
                }

                plots_panel.Refresh();  
            });

            return true;
        }

        private async void DeviceSelectionChanged(object? sender, EventArgs e)
        {
            if (data_grid.SelectedCells.Count != 1)
            {
                return;
            }

            var selected_cell = data_grid.SelectedCells[0];

            
            int index = selected_cell.RowIndex;
            string column_name = selected_cell.OwningColumn.Name;

            Debug.WriteLine(index + ":" + column_name);
        }

        private void RunOn(Control which, Action action)
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

        private bool CopyIfChanged(CoordinatorDeviceEntry dest, CoordinatorDeviceEntry src)
        {
            bool changed = false;
            if (src.device_index != dest.device_index)
            {
                dest.device_index = src.device_index;
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

        private async void Start(object? sender, System.Timers.ElapsedEventArgs e)
        {
            timer.Stop();
            try
            {
                var devices = await coordinator_data.GetDevices();
                if (devices != null)
                {
                    RunOn(data_grid, new Action(() =>
                    {
                        bool changed = false;
                        foreach (var device in devices)
                        {
                            if (device.device_id.Length <= 0 || !device.is_associated)
                            {
                                continue;
                            }

                            int index = -1;
                            if (device_entries_list_indexes.TryGetValue(device.GetHashCode(), out index))
                            {
                                changed |= CopyIfChanged(device_entries_list[index], device);
                            }
                            else
                            {
                                device_entries_list_indexes.Add(device.GetHashCode(), device_entries_list.Count);
                                device_entries_list.Add(device);
                            }

                        }

                        if (changed)
                        {
                            data_grid.Refresh();
                        }
                    }));

                    if(await UpdateGraphs())
                    {
                        // Good job!
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Debug.WriteLine(ex.StackTrace);
            }
            finally
            {
                timer.Start();
            }
        }
    }
}
