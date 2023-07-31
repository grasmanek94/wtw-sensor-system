using ScottPlot.Plottables;
using ScottPlot.WinForms;
using ScottPlot.DataSources;

namespace CoordinatorViewer
{
    internal class ScatterPlotSource : IDisposable
    {
        private FormsPlot forms_plot { get; set; }
        public Scatter scatter_plot { get; set; }

        public ScatterPlotSource(FormsPlot forms_plot, string label, IScatterSource source)
        {
            this.forms_plot = forms_plot;

            scatter_plot = forms_plot.Plot.Add.Scatter(source);
            scatter_plot.Label = label;
            var x = forms_plot.Plot.Legends[0];
        }

        public void Dispose()
        {
            if (forms_plot.Plot.Plottables.Contains(scatter_plot))
            {
                forms_plot.Plot.Plottables.Remove(scatter_plot);
            }
        }

        ~ScatterPlotSource()
        {
            Dispose();
        }
    }
}
