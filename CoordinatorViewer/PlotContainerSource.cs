using ScottPlot.Plottables;
using ScottPlot.WinForms;

namespace CoordinatorViewer
{
    internal class PlotContainerSource: IDisposable
    {
        public readonly FormsPlot forms_plot;
        public readonly Dictionary<string, DataLogger> plots;
        public double y_min;
        public double y_max;

        public PlotContainerSource(double y_min = 0.0, double y_max = 0.0) { 
            forms_plot = new FormsPlot();
            forms_plot.Dock = DockStyle.Fill;

            this.y_min = y_min;
            this.y_max = y_max;

            plots = new();
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
