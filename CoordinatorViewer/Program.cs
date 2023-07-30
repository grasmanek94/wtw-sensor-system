using Newtonsoft.Json.Linq;

namespace CoordinatorViewer
{
    internal static class Program
    {
        public static string username = "";
        public static string password = "";

        static void UpdateAuth()
        {
            string current_level = "./";
            const string next_level = "../";
            const string config_file = "/data/config.json";

            while (true)
            {
                var directories = Directory.GetDirectories(current_level);
                
                foreach (var d in directories)
                {
                    string config_file_result = d + config_file;
                    if (File.Exists(config_file_result))
                    {
                        JObject config = JObject.Parse(File.ReadAllText(config_file_result));
                        if(config.ContainsKey("auth_user") && config.ContainsKey("auth_pw"))
                        {
                            username = config["auth_user"]?.ToString() ?? "";
                            password = config["auth_pw"]?.ToString() ?? "";
                            return;
                        }
                    }
                }

                string absolute = Path.GetFullPath(current_level);
                if(absolute.EndsWith(":\\"))
                {
                    break;
                }
                current_level += next_level;
            }
        }

        /// <summary>
        ///  The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            // To customize application configuration such as set high DPI settings or default font,
            // see https://aka.ms/applicationconfiguration.
            ApplicationConfiguration.Initialize();

            UpdateAuth();

            Application.Run(new FormAllDevicesViewer());
        }
    }
}
