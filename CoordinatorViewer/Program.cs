using Newtonsoft.Json.Linq;
using System.Reflection;

namespace CoordinatorViewer
{
    internal static class Program
    {
        public static string username = "";
        public static string password = "";

        static bool UpdateAuth(string? base_path)
        {
            if(base_path == null)
            {
                return false;
            }

            string current_level = Path.Combine(base_path, "./");
            const string next_level = "../";
            const string config_file = "data/config.json";

            while (true)
            {
                var directories = Directory.GetDirectories(current_level);
                
                foreach (var d in directories)
                {
                    string config_file_result = Path.Combine(d, config_file);
                    if (File.Exists(config_file_result))
                    {
                        JObject config = JObject.Parse(File.ReadAllText(config_file_result));
                        if(config.ContainsKey("auth_user") && config.ContainsKey("auth_pw"))
                        {
                            username = config["auth_user"]?.ToString() ?? "";
                            password = config["auth_pw"]?.ToString() ?? "";
                            return true;
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

            return false;
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

            if(!UpdateAuth(Path.GetDirectoryName(Assembly.GetEntryAssembly()?.Location)))
            {
                UpdateAuth(Environment.CurrentDirectory);
            }

            Application.Run(new FormAllDevicesViewer());
        }
    }
}
