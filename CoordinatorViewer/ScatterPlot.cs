using ScottPlot.Plottables;
using ScottPlot.WinForms;
using ScottPlot;

namespace CoordinatorViewer
{
    internal class ScatterPlot
    {
        private FormsPlot forms_plot { get; set; }
        public Scatter scatter_plot { get; set; }

        public readonly List<Coordinates> coordinates;

        public ScatterPlot(FormsPlot forms_plot, string label)
        {
            this.forms_plot = forms_plot;
            coordinates = new();

            scatter_plot = forms_plot.Plot.Add.Scatter(coordinates);
            scatter_plot.Label = label;
        }

        public void Add(IEnumerable<Coordinates> data)
        {
            coordinates.AddRange(data);
        }

        public void Add(Coordinates data)
        {
            coordinates.Add(data);
        }

        public void Add(double x, double y)
        {
            coordinates.Add(new Coordinates(x, y));
        }

        public void Clear()
        {
            coordinates.Clear();
        }

        ~ScatterPlot()
        {
            forms_plot.Plot.Plottables.Remove(scatter_plot);
        }
    }
}
