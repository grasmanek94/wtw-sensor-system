using ScottPlot.DataSources;
using ScottPlot.WinForms;

namespace CoordinatorViewer
{
    internal class PlotContainerSource: IDisposable
    {
        public readonly FormsPlot forms_plot;
        public readonly Dictionary<string, ScatterPlotSource> plots;
        public double y_min;
        public double y_max;

        public PlotContainerSource(double y_min = 0.0, double y_max = 0.0) { 
            forms_plot = new FormsPlot();
            forms_plot.Dock = DockStyle.Fill;

            this.y_min = y_min;
            this.y_max = y_max;

            plots = new();
        }

        public ScatterPlotSource Add(string label, IScatterSource source)
        {
            ScatterPlotSource? plot;
            if (!plots.TryGetValue(label, out plot))
            {
                plot = new ScatterPlotSource(forms_plot, label, source);
                plots.Add(label, plot);
            }

            return plot;
        }

        public ScatterPlotSource? Get(string label)
        {
            ScatterPlotSource? plot;
            if (!plots.TryGetValue(label, out plot))
            {
                return null;
            }

            return plot;
        }

        public void Clear()
        {
            foreach(var plot in plots.Values)
            {
                plot.Dispose();
            }
            plots.Clear();
        }

        public void Fit()
        {
            forms_plot.Plot.AutoScale(true);

            forms_plot.Plot.YAxis.Min = y_min;
            forms_plot.Plot.YAxis.Max = y_max;
            forms_plot.Plot.XAxis.Max = 1;
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
