﻿using CsvHelper;
using CsvHelper.Configuration;
using System.ComponentModel;
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

        public async Task<BindingList<T>> GetData<T>(Task<HttpResponseMessage> http_request)
        {
            var response = await http_request;
            if (response.StatusCode != System.Net.HttpStatusCode.OK)
            {
                return new BindingList<T>();
            }

            var data = await response.Content.ReadAsStringAsync();
            data = data.Replace("\t", "").Replace(" ", "");

            using (var reader = new StringReader(data))
            using (var csv = new CsvReader(reader, config))
            {
                try
                {
                    return new BindingList<T>(csv.GetRecords<T>().ToList());
                }
                catch
                {
                    return new BindingList<T>();
                }
            }
        }

        public async Task<long> GetTimeOffset()
        {
            var response = await http_client.GetAsync(new Uri(base_address, "/now"));
            if (response.StatusCode != System.Net.HttpStatusCode.OK)
            {
                return -1;
            }

            var data = await response.Content.ReadAsStringAsync();
            return long.Parse(data);
        }

        public async Task<BindingList<CoordinatorDeviceEntry>?> GetDevices()
        {
            return await GetData<CoordinatorDeviceEntry>(http_client.GetAsync(new Uri(base_address, "/get/devices")));
        }

        public async Task<BindingList<SensorMeasurement>> GetMeasurements(Task<HttpResponseMessage> response)
        {
            return await GetData<SensorMeasurement>(response);
        }

        public Task<HttpResponseMessage> GetMeasurements(string timespan, SensorLocation index, int? from_rel_time = null)
        {
            string target = "/get/" + timespan + "/?loc=" + ((int)index).ToString();
            if(from_rel_time.HasValue)
            {
                target += "&time=" + from_rel_time.Value.ToString();
            }

            return http_client.GetAsync(new Uri(base_address, target));
        }

        public async Task<BindingList<SensorMeasurement>> GetVeryShortMeasurements(SensorLocation index, int? from_rel_time = null)
        {
            return await GetMeasurements(GetMeasurements("very_short", index, from_rel_time));
        }

        public async Task<BindingList<SensorMeasurement>> GetShortMeasurements(SensorLocation index, int? from_rel_time = null)
        {
            return await GetMeasurements(GetMeasurements("short", index, from_rel_time));
        }

        public async Task<BindingList<SensorMeasurement>> GetLongMeasurements(SensorLocation index, int? from_rel_time = null)
        {
            return await GetMeasurements(GetMeasurements("long", index, from_rel_time));
        }
    }
}
