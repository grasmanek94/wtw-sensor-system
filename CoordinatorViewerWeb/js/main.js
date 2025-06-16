import {
  fetchInitialData,
  pollDevices,
  setupChart,
  updateChartProperty,
  appendLiveData,
  setupTabs,
  updateDeviceTable,
  updateIthoStatus,
  setIthoSpeed
} from './api.js';

let currentProperty = 'temp_c';

window.setIthoSpeed = setIthoSpeed;

document.addEventListener('DOMContentLoaded', async () => {
  setupTabs();

  const { allData, deviceMap, now } = await fetchInitialData();
  setupChart(allData, deviceMap, now, currentProperty);

  const propertySelect = document.getElementById('property');
  if (propertySelect) {
    propertySelect.addEventListener('change', (e) => {
      currentProperty = e.target.value;
      updateChartProperty(currentProperty);
    });
  }

  setInterval(async () => {
    const newData = await pollDevices();
    if (newData) {
      appendLiveData(newData);
      updateDeviceTable(newData.devices);
      updateIthoStatus(newData.ithoStatus);
    }
  }, 5000);

  window.addEventListener('resize', () => {
    Plotly.Plots.resize(document.getElementById('plot'));
  });
});
