namespace CoordinatorViewer
{
    public partial class FormSync : Form
    {
        private CancellationTokenSource source;
        private CancellationToken token;

        public FormSync()
        {
            InitializeComponent();

            btn_start.Click += Start;
            btn_cancel.Click += Stop;
            progress_bar.Minimum = 0;
            progress_bar.Maximum = 500;

            source = new CancellationTokenSource();
            token = source.Token;
            btn_cancel.Enabled = false;
        }

        private void Increase()
        {
            if (progress_bar.Value >= progress_bar.Maximum)
            {
                return;
            }

            Thread.Sleep(10);
            progress_bar.Value += 1;
        }

        private void Count(CancellationTokenSource c)
        {
            btn_cancel.Enabled = true;
            progress_bar.Value = 0;

            while (!c.IsCancellationRequested && progress_bar.Value < progress_bar.Maximum)
            {
                Increase();
            }

            btn_cancel.Enabled = false;
        }

        private void Start(object? sender, EventArgs e)
        {
            Count(source);
        }

        private void Stop(object? sender, EventArgs e)
        {
            source.Cancel();
        }
    }
}