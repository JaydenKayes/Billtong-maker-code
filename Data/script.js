// --- ESP32 Web Server Address ---
// Replace this with your ESP32's actual IP address shown in Serial Monitor
const ESP32_IP = "http://192.168.1.100";  

// --- Sensor Toggle ---
// Used to temporarily stop live updates if needed
let sensorsEnabled = true;

// --- Chart.js Setup ---
// Get the canvas element and prepare the graph for live data display
const ctx = document.getElementById("dataChart").getContext("2d");
const dataChart = new Chart(ctx, {
  type: "line", // Line chart for temperature and humidity
  data: {
    labels: [], // X-axis time labels
    datasets: [
      {
        label: "Temperature (°C)",
        borderColor: "red",
        data: [], // Temperature data points
        fill: false
      },
      {
        label: "Humidity (%)",
        borderColor: "blue",
        data: [], // Humidity data points
        fill: false
      }
    ]
  },
  options: {
    responsive: true,
    scales: {
      x: {
        title: { display: true, text: "Time (s)" } // Label for X-axis
      },
      y: {
        beginAtZero: false // Allows values above/below 0 naturally
      }
    }
  }
});

// --- Fetch Live Sensor Data from ESP32 ---
async function refreshData() {
  // Stop fetching if sensors are disabled
  if (!sensorsEnabled) return;

  try {
    // Request data from ESP32 web server
    const response = await fetch(`${ESP32_IP}/data`);
    const json = await response.json(); // Parse JSON reply

    // Update displayed sensor readings
    document.getElementById("temp").innerText = json.temp;
    document.getElementById("hum").innerText = json.hum;
    document.getElementById("fan").innerText = json.fan ? "ON" : "OFF";
    document.getElementById("lamp").innerText = json.lamp ? "ON" : "OFF";

    // --- Add new data to chart ---
    const time = new Date().toLocaleTimeString(); // Get current time for label
    dataChart.data.labels.push(time);
    dataChart.data.datasets[0].data.push(json.temp);
    dataChart.data.datasets[1].data.push(json.hum);

    // Keep only the latest 20 data points for readability
    if (dataChart.data.labels.length > 20) {
      dataChart.data.labels.shift();
      dataChart.data.datasets[0].data.shift();
      dataChart.data.datasets[1].data.shift();
    }

    dataChart.update(); // Refresh chart display
  } catch (err) {
    console.error("Error fetching data:", err); // Log any network errors
  }
}

// --- Control Functions for Fan and Lamp ---
function toggleFan(state) {
  // Send a command to turn fan ON or OFF
  fetch(`${ESP32_IP}/${state ? "fanOn" : "fanOff"}`);
}

function toggleLamp(state) {
  // Send a command to turn lamp ON or OFF
  fetch(`${ESP32_IP}/${state ? "lampOn" : "lampOff"}`);
}

// --- Enable or Disable Sensor Updates ---
function toggleSensors() {
  sensorsEnabled = !sensorsEnabled;
  document.getElementById("sensors").innerText = sensorsEnabled ? "ON" : "OFF";
}

// --- Auto Refresh ---
// Call refreshData() every 2 seconds for live updates
setInterval(refreshData, 2000);
