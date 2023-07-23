using System.DirectoryServices;

namespace CoordinatorViewer
{
    internal static class Program
    {
        public static readonly string username = "";
        public static readonly string password = "";
        /// <summary>
        ///  The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            // To customize application configuration such as set high DPI settings or default font,
            // see https://aka.ms/applicationconfiguration.
            ApplicationConfiguration.Initialize();

            /*CoordinatorData data = new CoordinatorData();
            var task = data.GetDevices();
            task.Wait();
            var r = task.Result;
            foreach(CoordinatorDeviceEntry entry in r)
            {
                Console.WriteLine(entry.ToString());
            }*/


            //Application.Run(new FormSync());
            //Application.Run(new FormAsync());
            Application.Run(new FormAllDevicesViewer());
        }
    }
}
