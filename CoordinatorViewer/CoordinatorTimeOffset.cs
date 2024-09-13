
namespace CoordinatorViewer
{
    public class CoordinatorTimeOffset
    {
        public double time_offset;
        public DateTime offset_aquired;

        public CoordinatorTimeOffset(double time_offset)
        {
            this.time_offset = time_offset;
            this.offset_aquired = DateTime.Now;
        }

        public DateTime GetDate(double current_time_offset)
        {
            return offset_aquired.AddSeconds(current_time_offset - time_offset);
        }
    }
}
