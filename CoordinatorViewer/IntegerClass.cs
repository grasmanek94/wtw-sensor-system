namespace CoordinatorViewer
{
    class IntegerClass
    {
        public int Value { get; set; } = 0;
        public void max(int input)
        {
            Value = Math.Max(Value, input);
        }
    }
}
