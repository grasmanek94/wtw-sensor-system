/////////// API.js ///////////
const BASE_URL = 'coordinator.php?query=';
const BASE_ITHO_URL = 'nrg-itho.php?query=';

const LOCATIONS = [0, 1, 2, 3, 4, 5];
const LOCATION_NAMES = {
  0: 'Living Room',
  1: 'Bathroom',
  2: 'Bedroom Front',
  3: 'Bedroom Left',
  4: 'Bedroom Right',
  5: 'Fresh Air Inlet'
};

const PROPERTY_LABELS = {
  temp_c: 'Temperature (°C)',
  co2_ppm: 'CO2 (PPM)',
  rh: 'Relative Humidity (%)',
  attainable_rh: 'Attainable RH (%)',
  rh_headroom: 'Relative Humidity (Headroom %)',
  state_at_this_time: 'Ventilation State'
};

const COLORS = ['#e6194b', '#3cb44b', '#ffe119', '#4363d8', '#f58231', '#911eb4'];

let highestSeq = {};
let chartData = {};
let timeOffset = 0;
let traces = {};
let currentProperty = 'temp_c';

export async function fetchInitialData() {
  const now = parseInt(await axios.get(`${BASE_URL}/now`).then(res => res.data));
  const allData = {};
  const deviceMap = {};

  for (let loc of LOCATIONS) {
    const [long, short, veryShort] = await Promise.all([
      fetchCSV(`/get/long?loc=${loc}`),
      fetchCSV(`/get/short?loc=${loc}`),
      fetchCSV(`/get/very_short?loc=${loc}`)
    ]);
    allData[loc] = [...long, ...short, ...veryShort];
    highestSeq[loc] = Math.max(...allData[loc].map(d => parseInt(d.sequence_number)));
  }

  const devices = await fetchDevices();
  devices.forEach(d => deviceMap[d.sensor_location_id] = d);
  return { allData, deviceMap, now };
}

export async function fethIthoStatus() {
  return await axios.get(`${BASE_ITHO_URL}/api.html?get=ithostatus`).then(res => res.data);
}

export async function pollDevices() {
  const devices = await fetchDevices();
  const newData = {};
  for (let device of devices) {
    const loc = device.sensor_location_id;
    const seq = parseInt(device.sequence_number);
    if (seq > (highestSeq[loc] || 0)) {
      highestSeq[loc] = seq;
      newData[loc] = device;
    }
  }
  const ithoStatus = await fethIthoStatus();
  return { devices, newData, ithoStatus };
}

async function fetchDevices() {
  const res = await axios.get(`${BASE_URL}/get/devices`);
  return parseCSV(res.data);
}

async function fetchCSV(endpoint) {
  const res = await axios.get(`${BASE_URL}${endpoint}`);
  return parseCSV(res.data);
}

function parseCSV(csv) {
  const lines = csv.trim().split('\n');
  const headers = lines[0].split(',').map(h => h.trim());
  return lines.slice(1).map(line => {
    const values = line.split(',').map(v => v.trim());
    return Object.fromEntries(headers.map((h, i) => [h, values[i]]));
  });
}

/////////// UI.js ///////////
export function setupTabs() {
  const buttons = document.querySelectorAll('.tab-button');
  const contents = document.querySelectorAll('.tab-content');
  buttons.forEach(button => {
    button.addEventListener('click', () => {
      const tab = button.getAttribute('data-tab');
      contents.forEach(content => content.classList.remove('active'));
      document.getElementById(tab).classList.add('active');
      if (tab === 'graph') {
        Plotly.Plots.resize(document.getElementById('plot'));
      }
    });
  });
}

export function updateDeviceTable(devices) {
  const tbody = document.querySelector('#deviceTable tbody');
  tbody.innerHTML = '';
  devices.forEach(device => {
    const row = document.createElement('tr');
    row.innerHTML = `
      <td>${LOCATION_NAMES[device.sensor_location_id] || device.sensor_location_id}</td>
      <td>${device.device_id}</td>
      <td>${device.sensor_status === '0' || device.sensor_status === '4608' ? 'OK' : 'Error'}</td>
      <td>${device.temp_c}</td>
      <td>${device.co2_ppm}</td>
      <td>${device.rh}</td>
      <td><a href="http://${device.device_id}" target="_blank">Link</a></td>
    `;
    tbody.appendChild(row);
  });
}

export function updateIthoStatus(ithoStatus) {
  const tbody = document.querySelector('#ithoStatus tbody');
  tbody.innerHTML = '';
  const row = document.createElement('tr');
  row.innerHTML = `
    <td>${ithoStatus['requested-fanspeed_perc']}</td>
    <td>${ithoStatus['supply-fan-actual_rpm']}</td>
    <td>${ithoStatus['exhaust-fan-actual_rpm']}</td>
    <td>${ithoStatus['status']}</td>
    <td>${ithoStatus['global-fault-code']}</td>
    <td>${ithoStatus['bypass-position']}</td>
    <td>${ithoStatus['valve-position']}</td>
    <td>${ithoStatus['actual-mode']}</td>
    <td>${ithoStatus['supply-temp_c']}</td>
    <td>${ithoStatus['exhaust-temp_c']}</td>
  `;
  tbody.appendChild(row);
}

export function setIthoSpeed(level) {
  axios.get(`${BASE_ITHO_URL}/api.html?vremotecmd=${level}&vremoteindex=0`)
    .then(res => console.log(`Speed set to ${level}:`, res.data))
    .catch(err => console.error(`Failed to set speed to ${level}:`, err));
}

/////////// ChartManager.js (Plotly) ///////////
const layout = {
  title: '',
  xaxis: {
    type: 'date',
    fixedrange: false
  },
  yaxis: {
    title: property,
    automargin: true,
    fixedrange: true,
    autorange: true
  },
  legend: {
    orientation: 'h',
    x: 0.5,
    xanchor: 'center'
  },
  margin: { tp: 0, l: 0, r: 0, b: 0 },
  dragmode: 'pan'
};

function extractXY(data, property) {
  return data
    .map(d => {
      const x = new Date(timeOffset + parseInt(d.relative_time) * 1000);
      let y;
      if (property === 'rh_headroom') {
        const rh = parseFloat(d.rh);
        const attainable = parseFloat(d.attainable_rh);
        y = (!isNaN(rh) && !isNaN(attainable)) ? rh - attainable : NaN;
      } else {
        y = parseFloat(d[property]);
      }
      return (!isNaN(y) && isFinite(y)) ? { x, y } : null;
    })
    .filter(p => p !== null);
}

export function setupChart(allData, deviceMap, deviceNow, property) {
  chartData = allData;
  currentProperty = property;
  const systemNow = Date.now();
  timeOffset = systemNow - deviceNow * 1000;

  const plotDiv = document.getElementById('plot');
  traces = {};

  const data = LOCATIONS.map((loc, i) => {
    const deviceData = chartData[loc];
    if (!deviceData || deviceData.length === 0) return null;

    const filtered = extractXY(deviceData, property);
    if (filtered.length === 0) return null;

    const x = filtered.map(p => p.x);
    const y = filtered.map(p => p.y);
    traces[loc] = { x, y };

    return {
      x,
      y,
      mode: 'lines',
      name: LOCATION_NAMES[loc],
      line: { color: COLORS[i % COLORS.length] }
    };
  }).filter(trace => trace !== null);

  const config = {
    responsive: true,
    scrollZoom: true,
    displayModeBar: true,
    doubleClick: 'reset',
    showTips: true
  };

  Plotly.newPlot(plotDiv, data, layout, config);
}

export function updateChartProperty(property) {
  currentProperty = property;
  const plotDiv = document.getElementById('plot');

  const data = LOCATIONS.map((loc, i) => {
    const deviceData = chartData[loc];
    if (!deviceData || deviceData.length === 0) return null;

    const filtered = extractXY(deviceData, property);
    if (filtered.length === 0) return null;

    const x = filtered.map(p => p.x);
    const y = filtered.map(p => p.y);
    traces[loc] = { x, y };

    return {
      x,
      y,
      mode: 'lines',
      name: LOCATION_NAMES[loc],
      line: { color: COLORS[i % COLORS.length] }
    };
  }).filter(trace => trace !== null);

  if (data.length > 0) {
    Plotly.react(plotDiv, data, layout);
  } else {
    console.warn('No valid data to update chart for property:', property);
  }
}

export function appendLiveData({ newData }) {
  const update = { x: [], y: [] };
  const indices = [];

  Object.entries(newData).forEach(([loc, d]) => {
    const xVal = new Date(timeOffset + parseInt(d.relative_time) * 1000);
    const yVal = parseFloat(d[currentProperty]);
    chartData[loc].push(d);
    traces[loc].x.push(xVal);
    traces[loc].y.push(yVal);
    update.x.push([xVal]);
    update.y.push([yVal]);
    indices.push(LOCATIONS.indexOf(parseInt(loc)));
  });

  Plotly.extendTraces('plot', update, indices);
}
