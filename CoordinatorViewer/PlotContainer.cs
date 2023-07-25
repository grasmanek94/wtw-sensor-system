using ScottPlot.WinForms;

namespace CoordinatorViewer
{
    internal class PlotContainer
    {
        public FormsPlot forms_plot { get; set; }

        public readonly Dictionary<string, ScatterPlot> plots;

        public PlotContainer() { 
            forms_plot = new FormsPlot();
            forms_plot.Dock = DockStyle.Fill;
            forms_plot.Plot.AutoScale(true);

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
                plot.Clear();
            }
            plots.Clear();
        }

        ~PlotContainer()
        {
            Clear();
        }
    }
}
