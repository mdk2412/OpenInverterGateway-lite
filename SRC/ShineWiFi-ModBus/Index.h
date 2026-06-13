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

  <h2>Growatt MIN TL-XH</h2>

<table>
  <tbody>
    <tr>
      <th style="width: 50%">Priority Mode</th>
      <td id="priorityMode">Loading...</td>
    </tr>
    <tr>
      <th>Output Power</th>
      <td id="outputPower">Loading...</td>
    </tr>
    <tr>
      <th>PV2 Power</th>
      <td id="pv2Power">Loading...</td>
    </tr>
    <tr>
      <th>PV2 Voltage</th>
      <td id="pv2Voltage">Loading...</td>
    </tr>
    <tr>
      <th>Inverter Temperature</th>
      <td id="inverterTemperature">Loading...</td>
    </tr>
    <tr>
      <th>State of Charge</th>
      <td id="stateofCharge">Loading...</td>
    </tr>
    <tr>
      <th>Charging Power (Limit)</th>
      <td id="batteryCharge">Loading...</td>
    </tr>
    <tr>
      <th>Discharging Power (Limit)</th>
      <td id="batteryDischarge">Loading...</td>
    </tr>
    <tr>
      <th>Battery Temperature</th>
      <td id="batteryTemperature">Loading...</td>
    </tr>
  </tbody>
</table>

  <hr>

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

    )====="
#if ENABLE_MODBUS_COMMUNICATION == 1
R"=====(
    <a href="./postCommunicationModbus" role="button">Modbus Access</a>
)====="
#else
R"=====(
    <a href="./postCommunicationModbus" role="button" disabled>Modbus Access</a>
)====="
#endif
R"=====(
  </div>

<!-- STATUS BUTTONS -->
<div class="grid" style="margin-bottom: 1rem;">
  <a href="./status" role="button" class="secondary">JSON</a>
  <a href="./uiStatus" role="button" class="secondary">UI JSON</a>
  <a href="./metrics" role="button" class="secondary">Metrics</a>
)====="
  #ifdef ENABLE_WEB_DEBUG
R"=====(
    <a href="./debug" role="button" class="secondary">Log</a>
)====="
#else
R"=====(
    <a href="./debug" role="button" class="secondary" disabled>Log</a>
)====="
#endif
R"=====(

</div>

<!-- SYSTEM BUTTONS -->
<div class="grid" style="margin-bottom: 1rem;">
  <a href="./startAp" role="button" class="contrast"
     onclick="return confirm('Start Config AP?');">Start Config AP</a>

  <a href="./reboot" role="button" class="contrast"
     onclick="return confirm('Reboot?');">Reboot</a>
</div>

  <script>
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
  </script>

</main>
</body>
</html>
)=====";

#if ENABLE_MODBUS_COMMUNICATION == 1
const char SendPostSite_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Growatt MIN TL-XH</title>
  <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/@picocss/pico@2/css/pico.min.css">
</head>

<body>
<main class="container">

  <h2>Modbus Access</h2>

  <form id="modbusForm">

    <label>
      Register ID
      <input type="text" name="reg" value="0">
    </label>

    <label>
      Register Value
      <input type="text" name="val" readonly>
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

  <div class="grid" style="margin-bottom: 1rem;">
    <a href="." role="button" class="contrast">Back</a>
   </div>

  <script>
    const typeSelect = document.querySelector('select[name="type"]');
    const regSelect = document.querySelector('select[name="registerType"]');
    const writeButton = document.querySelector('button.danger');
    const valueInput = document.querySelector('input[name="val"]');

    function updateUI() {
      const is16Bit = typeSelect.value === '16b';
      const is32Bit = typeSelect.value === '32b';
      const isHolding = regSelect.value === 'H';
      const isInput = regSelect.value === 'I';

      writeButton.style.display = (is16Bit && isHolding) ? 'inline-block' : 'none';

      if (isInput || (isHolding && is32Bit)) {
        valueInput.readOnly = true;
      } else {
        valueInput.readOnly = false;
      }
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

        let extractedValue = trimmed;
        const match = trimmed.match(/(?:Reading|Writing) Value\s+(.+?)\s+(?:from|to)/i);
        if (match) extractedValue = match[1];

        if (op === 'R' || op === 'W') {
          valueInput.value = extractedValue;
        }

      } catch (e) {
        console.error("Error:", e.message);
      }
    }
  </script>

</main>
</body>
</html>
)=====";

#endif