using ScottPlot;
using ScottPlot.Plottables;
using ScottPlot.TickGenerators;
using ScottPlot.WinForms;
using System;
using System.Diagnostics;

namespace CoordinatorViewer
{
    internal class PlotContainerSource: IDisposable
    {
        public readonly FormsPlot forms_plot;
        public readonly Dictionary<string, DataLogger> plots;
        public double y_min;
        public double y_max;
        private bool date_time_added;

        public PlotContainerSource(double y_min = 0.0, double y_max = 0.0) { 
            forms_plot = new FormsPlot();
            forms_plot.Dock = DockStyle.Fill;

            this.y_min = y_min;
            this.y_max = y_max;
            date_time_added = false;

            plots = new();
        }

        private static string CustomFormatter(DateTime dt)
        {
            return $"{dt:dd-MM}\n{dt:HH:mm}";
        }

        private void AddDateTime()
        {         
            forms_plot.Plot.Axes.DateTimeTicksBottom();
            ((DateTimeAutomatic)forms_plot.Plot.Axes.Bottom.TickGenerator).LabelFormatter = CustomFormatter;

            date_time_added = true;
        }

        public DataLogger Get(string label)
        {
            DataLogger? plot;
            if (!plots.TryGetValue(label, out plot))
            {
                plot = forms_plot.Plot.Add.DataLogger();
                plot.LegendText = label;
                plots.Add(label, plot);

                forms_plot.Plot.Axes.SetLimitsY(y_min, y_max);
               
                if(!date_time_added)
                {
                    AddDateTime();
                }
            }

            return plot;
        }

        public void Clear()
        {
            foreach(var plot in plots.Values)
            {
                // plot.Dispose();
            }
            plots.Clear();
        }

        public void Dispose()
        {
            Clear();
            forms_plot.Dispose();
        }

        ~PlotContainerSource()
        {
            Dispose();
        }
    }
}
