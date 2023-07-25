using ScottPlot.WinForms;

namespace CoordinatorViewer
{
    internal class PlotContainer: IDisposable
    {
        public readonly FormsPlot forms_plot;
        public readonly Dictionary<string, ScatterPlot> plots;
        private readonly double y_min;
        private readonly double y_max;

        public PlotContainer(double y_min = 0.0, double y_max = 0.0) { 
            forms_plot = new FormsPlot();
            forms_plot.Dock = DockStyle.Fill;

            this.y_min = y_min;
            this.y_max = y_max;

            plots = new();
        }

        public ScatterPlot Add(string label)
        {
            ScatterPlot plot;
            if (!plots.TryGetValue(label, out plot))
            {
                plot = new ScatterPlot(forms_plot, label);
                plots.Add(label, plot);
            }

            return plot;
        }

        public ScatterPlot Get(string label)
        {
            ScatterPlot plot;
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
        }

        public void Dispose()
        {
            Clear();
            forms_plot.Dispose();
        }

        ~PlotContainer()
        {
            Dispose();
        }
    }
}
