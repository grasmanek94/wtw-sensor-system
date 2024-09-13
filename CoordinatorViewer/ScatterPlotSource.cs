using ScottPlot.Plottables;
using ScottPlot.WinForms;
using ScottPlot.DataSources;
using ScottPlot;

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
            scatter_plot.LegendText = label;
            var x = forms_plot.Plot.Legend;
        }

        public void Dispose()
        {
            if (forms_plot.Plot.PlottableList.Contains(scatter_plot))
            {
                forms_plot.Plot.PlottableList.Remove(scatter_plot);
            }
        }

        ~ScatterPlotSource()
        {
            Dispose();
        }
    }
}
