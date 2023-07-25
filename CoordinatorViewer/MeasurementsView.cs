﻿using ScottPlot;
using System.Collections;

namespace CoordinatorViewer
{
    internal class MeasurementsView : IReadOnlyList<Coordinates>
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

            public void Dispose() {}

            public bool MoveNext()
            {
                return (++index < mv.Count);
            }

            public void Reset()
            {
                index = 0;
            }
        }

        private List<SensorMeasurement> vs;
        private List<SensorMeasurement> s;
        private List<SensorMeasurement> l;
        private Func<SensorMeasurement, double> x;
        private Func<SensorMeasurement, double> y;

        public MeasurementsView(List<SensorMeasurement> vs, List<SensorMeasurement> s, List<SensorMeasurement> l, Func<SensorMeasurement, double> x, Func<SensorMeasurement, double> y) {
            this.vs = vs;
            this.s = s;
            this.l = l;
            this.x = x;
            this.y = y;
        }

        public Coordinates this[int index]
        {
            get {
                Coordinates data;
                int new_index = index;
                if(new_index < l.Count)
                {
                    data = new Coordinates(x(l[new_index]), y(l[new_index]));
                    return data;
                }

                new_index -= l.Count;

                if(new_index < s.Count)
                {
                    data = new Coordinates(x(s[new_index]), y(s[new_index]));
                    return data;
                }

                new_index -= s.Count;

                data = new Coordinates(x(vs[new_index]), y(vs[new_index]));
                return data;
            }
        }

        public int Count
        {
            get { return l.Count + s.Count + vs.Count; }
        }

        public IEnumerator<Coordinates> GetEnumerator()
        {
            return new IteratorC(this);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
