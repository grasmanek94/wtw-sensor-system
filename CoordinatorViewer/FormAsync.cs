namespace CoordinatorViewer
{
    public partial class FormAsync : Form
    {
        private CancellationTokenSource source;
        private CancellationToken token;

        public FormAsync()
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

        private async Task<bool> Increase()
        {
            if (progress_bar.Value >= progress_bar.Maximum)
            {
                return false;
            }

            return await Task.Run(async () =>
            {
                await Task.Delay(10);
                progress_bar.Invoke(new Action(() =>
                {
                    progress_bar.Value += 1;
                    
                }));
                return true;
            });
        }

        private async void Count()
        {
            btn_cancel.Invoke(new Action(() => {
                btn_cancel.Enabled = true;
                progress_bar.Value = 0;
            }));

            while (!source.IsCancellationRequested && progress_bar.Value < progress_bar.Maximum)
            {
                if(!await Task.Run(Increase))
                {
                    break;
                }
            }

            source = new CancellationTokenSource();
            token = source.Token;

            btn_cancel.Invoke(new Action(() =>
            {
                btn_cancel.Enabled = false;
            }));
        }

        private async void Start(object? sender, EventArgs e)
        {
            await Task.Run(() => { Count(); });
        }

        private void Stop(object? sender, EventArgs e)
        {
            source.Cancel();
        }
    }
}
