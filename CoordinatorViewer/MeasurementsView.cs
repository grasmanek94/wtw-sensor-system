using ScottPlot;
using ScottPlot.DataSources;
using System.Collections;
using System.ComponentModel;

namespace CoordinatorViewer
{
    internal class MeasurementsView : IReadOnlyList<Coordinates>, IScatterSource
    {
        public class IteratorC : IEnumerator<Coordinates>
        {
            private int index;
            private MeasurementsView mv;

            public Coordinates Current
            {
                get
                {
                    return mv[index];
                }
            }

            object IEnumerator.Current
            {
                get
                {
                    return Current;
                }
            }

            public IteratorC(MeasurementsView mv)
            {
                this.index = 0;
                this.mv = mv;
            }

            public void Dispose() { }

            public bool MoveNext()
            {
                return (++index < mv.Count);
            }

            public void Reset()
            {
                index = 0;
            }
        }

        private BindingList<SensorMeasurement> vs;
        private BindingList<SensorMeasurement> s;
        private BindingList<SensorMeasurement> l;

        private Func<SensorMeasurement, double> x;
        private Func<SensorMeasurement, double> y;

        private CoordinateRange x_range;
        private CoordinateRange y_range;

        public MeasurementsView(BindingList<SensorMeasurement> vs, BindingList<SensorMeasurement> s, BindingList<SensorMeasurement> l, Func<SensorMeasurement, double> x, Func<SensorMeasurement, double> y)
        {
            this.vs = vs;
            this.s = s;
            this.l = l;
            this.x = x;
            this.y = y;

            x_range = new CoordinateRange(0.0, 0.0);
            y_range = new CoordinateRange(0.0, 0.0);

            this.vs.ListChanged += ListChangedVeryShort;
            this.s.ListChanged += ListChangedShort;
            this.l.ListChanged += ListChangedLong;

            CalculateRange();
        }

        public void SetYGetter(Func<SensorMeasurement, double> y)
        {
            this.y = y;
        }

        private void CalculateRange()
        {
            foreach (var entry in this)
            {
                x_range.Min = Math.Min(x_range.Min, entry.X);
                x_range.Max = Math.Max(x_range.Max, entry.X);
                y_range.Min = Math.Min(y_range.Min, entry.Y);
                y_range.Max = Math.Max(y_range.Max, entry.Y);
            }
        }

        private void CheckRange(ListChangedEventArgs e, BindingList<SensorMeasurement> list)
        {
            switch (e.ListChangedType)
            {
                case ListChangedType.ItemAdded:
                    x_range.Min = Math.Min(x_range.Min, x(list[e.NewIndex]));
                    x_range.Max = Math.Max(x_range.Max, x(list[e.NewIndex]));
                    y_range.Min = Math.Min(y_range.Min, y(list[e.NewIndex]));
                    y_range.Max = Math.Max(y_range.Max, y(list[e.NewIndex]));
                    break;

                case ListChangedType.ItemDeleted:
                    CalculateRange();
                    break;

                case ListChangedType.ItemMoved:
                    break;

                case ListChangedType.ItemChanged:
                    CalculateRange();
                    break;
            }
        }

        private void ListChangedVeryShort(object? sender, ListChangedEventArgs e)
        {
            CheckRange(e, vs);
        }

        private void ListChangedShort(object? sender, ListChangedEventArgs e)
        {
            CheckRange(e, s);
        }

        private void ListChangedLong(object? sender, ListChangedEventArgs e)
        {
            CheckRange(e, l);
        }

        public Coordinates this[int index]
        {
            get
            {
                Coordinates data;

                if (l != null)
                {
                    if (index < l.Count)
                    {
                        data = new Coordinates(x(l[index]), y(l[index]));
                        return data;
                    }
                    index -= l.Count;
                }

                if (s != null)
                {
                    if (index < s.Count)
                    {
                        data = new Coordinates(x(s[index]), y(s[index]));
                        return data;
                    }
                    index -= s.Count;
                }

                data = new Coordinates(x(vs[index]), y(vs[index]));
                return data;
            }
        }

        public int Count
        {
            get { return (l?.Count ?? 0) + (s?.Count ?? 0) + (vs?.Count ?? 0); }
        }

        public IEnumerator<Coordinates> GetEnumerator()
        {
            return new IteratorC(this);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public IReadOnlyList<Coordinates> GetScatterPoints()
        {
            return this;
        }

        public CoordinateRange GetLimitsX()
        {
            return x_range;
        }

        public CoordinateRange GetLimitsY()
        {
            return y_range;
        }

        public AxisLimits GetLimits()
        {
            return new AxisLimits(x_range, y_range);
        }
    }
}
