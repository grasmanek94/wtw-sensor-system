using CsvHelper;
using CsvHelper.Configuration;
using System.Globalization;
using System.Net.Http.Headers;

namespace CoordinatorViewer
{
    internal class CoordinatorData
    {
        private Uri base_address;
        private HttpClient http_client;
        private CsvConfiguration config;

        public CoordinatorData(string destination = "http://192.168.2.216/") { 
            if(destination.Last() != '/')
            {
                destination += '/';
            }

            base_address = new Uri(destination);
            http_client = new HttpClient();

            http_client.DefaultRequestHeaders.Authorization =
                 new AuthenticationHeaderValue(
                     "Basic", Convert.ToBase64String(
                         System.Text.ASCIIEncoding.ASCII.GetBytes(
                            $"{Program.username}:{Program.password}")));

            config = new CsvConfiguration(CultureInfo.InvariantCulture)
            {
                TrimOptions = TrimOptions.Trim
            };
        }

        public async Task<IEnumerable<T>?> GetData<T>(Task<HttpResponseMessage> http_request)
        {
            var response = await http_request;
            if (response.StatusCode != System.Net.HttpStatusCode.OK)
            {
                return null;
            }

            var data = await response.Content.ReadAsStringAsync();
            data = data.Replace("\t", "").Replace(" ", "");

            using (var reader = new StringReader(data))
            using (var csv = new CsvReader(reader, config))
            {
                return csv.GetRecords<T>().ToList();
            }
        }

        public async Task<IEnumerable<CoordinatorDeviceEntry>?> GetDevices()
        {
            return await GetData<CoordinatorDeviceEntry>(http_client.GetAsync(new Uri(base_address, "/get/devices")));
        }

        public async Task<IEnumerable<SensorMeasurement>?> GetMeasurements(Task<HttpResponseMessage> response)
        {
            return await GetData<SensorMeasurement>(response);
        }

        public Task<HttpResponseMessage> GetMeasurements(string timespan, int index)
        {
            return http_client.GetAsync(new Uri(base_address, "/get/" + timespan + "/?index=" + index.ToString()));
        }

        public Task<HttpResponseMessage> GetMeasurements(string timespan, string id)
        {
            return http_client.GetAsync(new Uri(base_address, "/get/" + timespan + "/?id=" + id));
        }

        public async Task<IEnumerable<SensorMeasurement>?> GetVeryShortMeasurements(string id)
        {
            return await GetMeasurements(GetMeasurements("very_short", id));
        }

        public async Task<IEnumerable<SensorMeasurement>?> GetVeryShortMeasurements(int index)
        {
            return await GetMeasurements(GetMeasurements("very_short", index));
        }

        public async Task<IEnumerable<SensorMeasurement>?> GetShortMeasurements(string id)
        {
            return await GetMeasurements(GetMeasurements("short", id));
        }

        public async Task<IEnumerable<SensorMeasurement>?> GetShortMeasurements(int index)
        {
            return await GetMeasurements(GetMeasurements("short", index));
        }

        public async Task<IEnumerable<SensorMeasurement>?> GetLongMeasurements(string id)
        {
            return await GetMeasurements(GetMeasurements("long", id));
        }

        public async Task<IEnumerable<SensorMeasurement>?> GetLongMeasurements(int index)
        {
            return await GetMeasurements(GetMeasurements("long", index));
        }
    }
}
