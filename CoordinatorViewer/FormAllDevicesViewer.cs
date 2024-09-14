using ScottPlot.Plottables;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace CoordinatorViewer
{
    public partial class FormAllDevicesViewer : Form
    {
        private System.Timers.Timer timer;
        private CoordinatorData coordinator_data;
        private CoordinatorTimeOffset? time_offset;
        private readonly BindingList<CoordinatorDeviceEntry> device_entries_list;
        private readonly Dictionary<int, int> device_entries_list_indexes;
        private readonly Dictionary<CoordinatorDeviceEntry, FormDeviceMeasurementsPlotter> device_entry_measurements;
        private readonly PlotContainerSource pc_vent_state;
        private readonly PlotContainerSource pc_temp;
        private readonly PlotContainerSource pc_co2_ppm;
        private readonly PlotContainerSource pc_rh;
        private readonly List<PlotContainerSource> pc_list;

        private enum ViewMode
        {
            FULL,
            JUMP,
            SLIDE
        }

        private ViewMode current_view_mode;

        public FormAllDevicesViewer()
        {
            InitializeComponent();

            coordinator_data = new();

            device_entries_list = new();
            device_entries_list_indexes = new();
            device_entry_measurements = new();

            data_grid.DataSource = device_entries_list;
            data_grid.AutoGenerateColumns = true;
            data_grid.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
            data_grid.AllowUserToAddRows = false;
            data_grid.EditMode = DataGridViewEditMode.EditProgrammatically;

            pc_vent_state = new(0.5, 3.5);
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

            timer = new();
            timer.Elapsed += Start;
            timer.Interval = 1000;
            timer.Start();

            data_grid.SelectionChanged += DeviceSelectionChanged;

            AddPlotToTable();

            current_view_mode = ViewMode.FULL;

            data_grid.MouseClick += MouseClickAction;
        }

        private void MouseClickAction(object? sender, MouseEventArgs e)
        {
            if(e.Button == MouseButtons.Right)
            {
                Debug.WriteLine("SwitchViewMode");
                SwitchViewMode();
            }
        }

        private void SwitchViewMode()
        {
            switch (current_view_mode)
            {
                case ViewMode.FULL:
                    SetAllLoggerViewMode(ViewMode.JUMP);
                    break;

                case ViewMode.JUMP:
                    SetAllLoggerViewMode(ViewMode.SLIDE);
                    break;

                case ViewMode.SLIDE:
                    SetAllLoggerViewMode(ViewMode.FULL);
                    break;
            }
        }

        private void SetAllLoggerViewMode(ViewMode view_mode)
        {
            current_view_mode = view_mode;

            foreach (var pc in pc_list)
            {
                pc.forms_plot.Plot.PlottableList.ForEach(p =>
                {
                    switch (view_mode)
                    {
                        case ViewMode.FULL:
                            ((DataLogger)p).ViewFull();
                            break;

                        case ViewMode.JUMP:
                            ((DataLogger)p).ViewJump();
                            break;

                        case ViewMode.SLIDE:
                            ((DataLogger)p).ViewSlide();
                            break;
                    }
                });
            }
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
            foreach (var entry in device_entries_list)
            {
                if (!entry.is_associated)
                {
                    continue;
                }

                FormDeviceMeasurementsPlotter? measurements;
                if (!device_entry_measurements.TryGetValue(entry, out measurements))
                {
                    measurements = new FormDeviceMeasurementsPlotter(
                        entry.sensor_location_id,
                        plots_panel,
                        pc_co2_ppm, pc_temp, pc_rh, pc_vent_state, time_offset);
                    device_entry_measurements.Add(entry, measurements);

                    var measurements_vs = await coordinator_data.GetVeryShortMeasurements(entry.sensor_location_id);
                    var measurements_s = await coordinator_data.GetShortMeasurements(entry.sensor_location_id);
                    var measurements_l = await coordinator_data.GetLongMeasurements(entry.sensor_location_id);

                    measurements.Update(measurements_vs, measurements_s, measurements_l);
                }

                measurements.Update(entry);
            }

            RunOn(plots_panel, () =>
            {
                foreach (var pc in pc_list)
                {
                    // pc.Fit();
                    pc.forms_plot.Update();
                }

                plots_panel.Refresh();
            });

            return true;
        }

        private void OpenUrl(string url)
        {
            try
            {
                Process.Start(url);
            }
            catch
            {
                // hack because of this: https://github.com/dotnet/corefx/issues/10361
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                {
                    url = url.Replace("&", "^&");
                    Process.Start(new ProcessStartInfo(url) { UseShellExecute = true });
                }
                else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    Process.Start("xdg-open", url);
                }
                else if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
                {
                    Process.Start("open", url);
                }
                else
                {
                    throw;
                }
            }
        }

        private void DeviceSelectionChanged(object? sender, EventArgs e)
        {
            if (data_grid.SelectedCells.Count != 1)
            {
                return;
            }

            var selected_cell = data_grid.SelectedCells[0];


            int index = selected_cell.RowIndex;
            string column_name = selected_cell.OwningColumn.Name;

            Debug.WriteLine(index + ":" + column_name);

            if (column_name == "device_id" && (ModifierKeys & Keys.Control) == Keys.Control)
            {
                OpenUrl("http://" + selected_cell.Value.ToString() + "/");
            }
        }

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

        private async void Start(object? sender, System.Timers.ElapsedEventArgs e)
        {
            timer.Stop();
            try
            {
                if (time_offset == null)
                {
                    var time_offset_task = await coordinator_data.GetTimeOffset();
                    time_offset = new CoordinatorTimeOffset(time_offset_task);
                }

                var devices = await coordinator_data.GetDevices();
                if (devices != null)
                {
                    RunOn(data_grid, new Action(() =>
                    {
                        bool changed = false;
                        foreach (var device in devices)
                        {
                            if (device == null || device.device_id == null || device.device_id.Length <= 0 || !device.is_associated)
                            {
                                continue;
                            }

                            int index = -1;
                            if (device_entries_list_indexes.TryGetValue(device.GetHashCode(), out index))
                            {
                                changed |= Utils.CopyIfChanged(device_entries_list[index], device);
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

                    if (await UpdateGraphs())
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
