#pragma once
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Growatt MIN TL-XH</title>
  <link rel="stylesheet" href="/pico.min.css">
</head>
<body>
<main class="container">
<nav style="position: sticky; top: 0; z-index: 10;">
  <ul>
    <li>
      <a href="#" role="button" class="secondary tab active" data-tab="main">
        Dashboard
      </a>
    </li>

    <li>
      <a href="#" role="button" class="secondary tab" data-tab="modbus">
        Modbus Access
      </a>
    </li>

    <li>
      <a href="#" role="button" class="secondary tab" data-tab="log">
        Web Debug
      </a>
    </li>
    <li>
      <a href="#" role="button" class="secondary tab" data-tab="settings">
        Settings
      </a>
    </li>
  </ul>
</nav>

<!-- ========================================================= -->
<!-- ====================== Dashboard ======================== -->
<!-- ========================================================= -->

<section id="main" class="tab-content">

  <table>
    <tbody>
      <tr><th style="width: 50%">Priority Mode</th><td id="priorityMode">Loading...</td></tr>
      <tr><th>Output Power</th><td id="outputPower">Loading...</td></tr>
      <tr><th>PV2 Power</th><td id="pv2Power">Loading...</td></tr>
      <tr><th>PV2 Voltage</th><td id="pv2Voltage">Loading...</td></tr>
      <tr><th>Inverter Temperature</th><td id="inverterTemperature">Loading...</td></tr>
      <tr><th>State of Charge</th><td id="stateofCharge">Loading...</td></tr>
      <tr><th>Charging Power (Limit)</th><td id="batteryCharge">Loading...</td></tr>
      <tr><th>Discharging Power (Limit)</th><td id="batteryDischarge">Loading...</td></tr>
      <tr><th>Battery Temperature</th><td id="batteryTemperature">Loading...</td></tr>
    </tbody>
  </table>

  <!-- PRIORITY BUTTONS -->
  <div class="grid" style="margin-bottom: 1rem;">
    <a href="#" role="button"
       onclick="if(confirm('Set priority to load first?')) fetch('/loadfirst'); return false;">
       Load First
    </a>

    <a href="#" role="button"
       onclick="if(confirm('Set priority to battery first?')) fetch('/batteryfirst'); return false;">
       Battery First
    </a>

    <a href="#" role="button"
       onclick="if(confirm('Set priority to grid first?')) fetch('/gridfirst'); return false;">
       Grid First
    </a>

  </div>

  <!-- STATUS BUTTONS -->
  <div class="grid" style="margin-bottom: 1rem;">
    <a href="./status" role="button" class="secondary">JSON</a>
    <a href="./uiStatus" role="button" class="secondary">UI JSON</a>
    <a href="./metrics" role="button" class="secondary">Metrics</a>

  </div>

  <!-- SYSTEM BUTTONS -->
  <div class="grid" style="margin-bottom: 1rem;">
    <a href="./startAp" role="button" class="contrast"
       onclick="return confirm('Start Config AP?');">Start Config AP</a>

    <a href="./reboot" role="button" class="contrast"
       onclick="return confirm('Reboot?');">Reboot</a>
  </div>

</section>


<!-- ========================================================= -->
<!-- ==================== MODBUS ACCESS ======================= -->
<!-- ========================================================= -->

<section id="modbus" class="tab-content" hidden>

  <form id="modbusForm">

    <label>
      Register ID
      <input type="text" name="reg">
    </label>

    <label>
      Register Value
      <input type="text" name="val" id="modbusVal" readonly>
    </label>

    <label>
      Type
      <select name="type">
        <option value="16b" selected>16-bit</option>
        <option value="32b">32-bit</option>
      </select>
    </label>

    <label>
      Register Type
      <select name="registerType">
        <option value="I">Input Register</option>
        <option value="H" selected>Holding Register</option>
      </select>
    </label>

    <div class="grid" style="margin-bottom: 1rem;">
      <button type="button" onclick="submitOperation('R')">Read</button>
      <button type="button" class="secondary" onclick="submitOperation('W')">Write</button>
    </div>

  </form>
</section>

<!-- ========================================================= -->
<!-- ==================== Web Debug ========================== -->
<!-- ========================================================= -->

<section id="log" class="tab-content" hidden>

  <iframe src="./debug" style="width: 100%; height: 70vh; border: none;"></iframe>

</section>

<!-- ========================================================= -->
<!-- ==================== Settings =========================== -->
<!-- ========================================================= -->

<section id="settings" class="tab-content" hidden>

  <form id="settingsForm">

    <h3>Network</h3>

    <label>
      Hostname
      <input type="text" name="hostname" id="hostname">
    </label>

    <label>
      Static IP
      <input type="text" name="static_ip" id="static_ip">
    </label>

    <label>
      Netmask
      <input type="text" name="static_netmask" id="static_netmask">
    </label>

    <label>
      Gateway
      <input type="text" name="static_gateway" id="static_gateway">
    </label>

    <label>
      DNS
      <input type="text" name="static_dns" id="static_dns">
    </label>

    <h3>MQTT</h3>

    <label>
      Server
      <input type="text" name="mqtt_server" id="mqtt_server">
    </label>

    <label>
      Port
      <input type="number" name="mqtt_port" id="mqtt_port">
    </label>

    <label>
      Topic
      <input type="text" name="mqtt_topic" id="mqtt_topic">
    </label>

    <label>
      User
      <input type="text" name="mqtt_user" id="mqtt_user">
    </label>

    <label>
      Password
      <input type="password" name="mqtt_pwd" id="mqtt_pwd">
    </label>

    <button type="button" onclick="saveSettings()">Save</button>

  </form>

</section>

<!-- ========================================================= -->
<!-- ====================== JAVASCRIPT ======================== -->
<!-- ========================================================= -->

<script>
  // TAB SWITCHING
  document.querySelectorAll(".tab").forEach(tab => {
    tab.addEventListener("click", (e) => {
      e.preventDefault();

      document.querySelectorAll(".tab").forEach(t => t.classList.remove("active"));
      tab.classList.add("active");

      const target = tab.dataset.tab;
      document.querySelectorAll(".tab-content").forEach(sec => sec.hidden = true);
      document.getElementById(target).hidden = false;
    });
  });

  // MAIN PAGE AUTO-UPDATE
  async function loadData() {
    try {
      const response = await fetch("/uiStatus");
      const data = await response.json();
      document.getElementById("priorityMode").textContent = data.Priority[0] + " " + data.Priority[1];
      document.getElementById("outputPower").textContent = data.OutputPower[0] + " " + data.OutputPower[1];
      document.getElementById("pv2Power").textContent = data.PV2Power[0] + " " + data.PV2Power[1];
      document.getElementById("pv2Voltage").textContent = data.PV2Voltage[0] + " " + data.PV2Voltage[1];
      document.getElementById("inverterTemperature").textContent = data.InverterTemperature[0] + " " + data.InverterTemperature[1];
      document.getElementById("stateofCharge").textContent = data.BDCStateOfCharge[0] + " " + data.BDCStateOfCharge[1];
      document.getElementById("batteryCharge").textContent = data.BDCChargePower[0] + " " + data.BDCChargePower[1] + " (" + data.BDCChargePowerRate[0] + " " + data.BDCChargePowerRate[1] + ")";
      document.getElementById("batteryDischarge").textContent = data.BDCDischargePower[0] + " " + data.BDCDischargePower[1] + " (" + data.BDCDischargePowerRate[0] + " " + data.BDCDischargePowerRate[1] + ")";
      document.getElementById("batteryTemperature").textContent = data.BDCTemperatureA[0] + " " + data.BDCTemperatureA[1];
    } catch (e) {
      console.error("Error fetching data:", e);
    }
  }

  loadData();
  setInterval(loadData, 1000);


  // MODBUS ACCESS LOGIC
  const typeSelect = document.querySelector('select[name="type"]');
  const regSelect = document.querySelector('select[name="registerType"]');
  const writeButton = document.querySelector('button.secondary');
  const valueInput = document.querySelector('input[name="val"]');

  function updateUI() {
    const is16Bit = typeSelect.value === '16b';
    const is32Bit = typeSelect.value === '32b';
    const isHolding = regSelect.value === 'H';
    const isInput = regSelect.value === 'I';

    writeButton.style.display = (is16Bit && isHolding) ? 'inline-block' : 'none';
    valueInput.readOnly = isInput || (isHolding && is32Bit);
  }

  updateUI();
  typeSelect.addEventListener('change', updateUI);
  regSelect.addEventListener('change', updateUI);

async function submitOperation(op) {
  const form = document.getElementById("modbusForm");
  const formData = new FormData(form);
  formData.append("operation", op);

  try {
    const response = await fetch("/postCommunicationModbus_p", {
      method: "POST",
      body: formData
    });

    const text = await response.text();
    const trimmed = text.trim();

    // Wert extrahieren
    let extractedValue = trimmed;
    const match = trimmed.match(/(?:Reading|Writing) Value\s+(.+?)\s+(?:from|to)/i);
    if (match) extractedValue = match[1];

    const lower = trimmed.toLowerCase();

    // READ → immer extrahierten Wert anzeigen
    if (op === 'R') {
      modbusVal.value = extractedValue;
      return;
    }

    // WRITE → Fehler oder Wert anzeigen
    if (op === 'W') {
      if (lower.includes("failed")) {
        modbusVal.value = trimmed;     // komplette Fehlermeldung
      } else {
        modbusVal.value = extractedValue;
      }
    }

  } catch (e) {
    console.error("Error:", e.message);
  }
}

async function saveSettings() {
  const form = document.getElementById("settingsForm");
  const formData = new FormData(form);

  const response = await fetch("/saveSettings", {
    method: "POST",
    body: formData
  });

  alert(await response.text());
}

</script>
</main>
</body>

</html>
)=====";