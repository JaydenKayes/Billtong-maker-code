// Replace with your ESP32's IP
const ESP32_IP = "http://192.168.1.100";  

let sensorsEnabled = true;

// Chart.js setup
const ctx = document.getElementById("dataChart").getContext("2d");
const dataChart = new Chart(ctx, {
  type: "line",
  data: {
    labels: [],
    datasets: [
      {
        label: "Temperature (°C)",
        borderColor: "red",
        data: [],
        fill: false
      },
      {
        label: "Humidity (%)",
        borderColor: "blue",
        data: [],
        fill: false
      }
    ]
  },
  options: {
    responsive: true,
    scales: {
      x: {
        title: { display: true, text: "Time (s)" }
      },
      y: {
        beginAtZero: false
      }
    }
  }
});

// Fetch live sensor data
async function refreshData() {
  if (!sensorsEnabled) return;

  try {
    const response = await fetch(`${ESP32_IP}/data`);
    const json = await response.json();

    document.getElementById("temp").innerText = json.temp;
    document.getElementById("hum").innerText = json.hum;
    document.getElementById("fan").innerText = json.fan ? "ON" : "OFF";
    document.getElementById("lamp").innerText = json.lamp ? "ON" : "OFF";

    // Add data to graph
    const time = new Date().toLocaleTimeString();
    dataChart.data.labels.push(time);
    dataChart.data.datasets[0].data.push(json.temp);
    dataChart.data.datasets[1].data.push(json.hum);

    // Keep chart limited to 20 points
    if (dataChart.data.labels.length > 20) {
      dataChart.data.labels.shift();
      dataChart.data.datasets[0].data.shift();
      dataChart.data.datasets[1].data.shift();
    }

    dataChart.update();
  } catch (err) {
    console.error("Error fetching data:", err);
  }
}

// --- Control functions ---
function toggleFan(state) {
  fetch(`${ESP32_IP}/${state ? "fanOn" : "fanOff"}`);
}

function toggleLamp(state) {
  fetch(`${ESP32_IP}/${state ? "lampOn" : "lampOff"}`);
}

function toggleSensors() {
  sensorsEnabled = !sensorsEnabled;
  document.getElementById("sensors").innerText = sensorsEnabled ? "ON" : "OFF";
}

// Update every 2 seconds
setInterval(refreshData, 2000);