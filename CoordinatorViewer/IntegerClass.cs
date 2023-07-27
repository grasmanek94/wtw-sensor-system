namespace CoordinatorViewer
{
    public partial class FormAllDevicesViewer
    {
        private class IntegerClass
        {
            public int Value { get; set; } = 0;
            public void max(int input)
            {
                Value = Math.Max(Value, input);
            }
        }
    }
}
