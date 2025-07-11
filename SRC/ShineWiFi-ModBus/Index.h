#pragma once
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html lang="en">

<head>
  <meta charset='utf-8'>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Growatt MIN TL-XH</title>
  <style>
    body {
      display: flex;
      flex-direction: column;
      align-items: center;
      max-width: 90vw;
      margin: 0 auto;
      font-family: Arial, sans-serif;
      font-size: 1rem;
      padding: 1em;
    }

    h2 {
      font-size: 2rem;
      text-align: center;
      margin-bottom: 1em;
    }

    h3 {
      font-size: 1.1em;
      margin: 0;
    }

    .dataContainer {
      display: grid;
      grid-template-columns: max-content minmax(80px, 1fr);
      column-gap: 0.5em;
      row-gap: 0.7em;
      width: 100%;
    }

    .dataContainer .label {
      text-align: left;
    }

    .dataContainer .value {
      text-align: right;
    }

    .buttonRow-green,
    .buttonRow-yellow,
    .buttonRow-red {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      gap: 0.5em;
      margin-bottom: 1.2em;
    }

    .linkButton {
      border-radius: 4px;
      border: 1px solid #91ca5f;
      background: #6eb92b;
      color: #fff;
      padding: 0.6em 1em;
      text-decoration: none;
      font-size: 0.9em;
      min-width: 120px;
      text-align: center;
    }

    .linkButton.yellow {
      border-color: #cabd5f;
      background: #b9b22b;
    }

    .linkButton.red {
      border-color: #ca5f5f;
      background: #b92b2b;
    }

    .sectionDivider {
      width: 100%;
      border: none;
      border-top: 2px solid #ccc;
      margin: 2em 0 1.5em 0;
    }

    @media (max-width: 600px) {
      h2 {
        font-size: 1.5rem;
      }

      .linkButton {
        font-size: 1em;
        min-width: auto;
        flex: 1 1 auto;
        text-align: center;
      }

      .buttonRow-green,
      .buttonRow-yellow,
      .buttonRow-red {
        flex-wrap: nowrap;
        overflow-x: auto;
      }
    }

    @media (min-width: 800px) {
      .dataContainer {
        grid-template-columns: max-content minmax(80px, 200px);
        column-gap: 0.3em;
      }
    }
  </style>

</head>

<body>
  <h2>Growatt MIN TL-XH</h2>

  <div class="dataContainer">
    <span class="label">Priority Mode:</span> <span id="priorityMode" class="value">Loading...</span>
    <span class="label">Output Power:</span> <span id="outputPower" class="value">Loading...</span>
    <span class="label">PV2 Power:</span> <span id="pv2Power" class="value">Loading...</span>
    <span class="label">PV2 Voltage:</span> <span id="pv2Voltage" class="value">Loading...</span>
    <span class="label">Inverter Temperature:</span> <span id="inverterTemperature" class="value">Loading...</span>
    <span class="label">State of Charge:</span> <span id="stateofCharge" class="value">Loading...</span>
    <span class="label">Charging Power:</span> <span id="batteryCharge" class="value">Loading...</span>
    <span class="label">Discharging Power:</span> <span id="batteryDischarge" class="value">Loading...</span>
    <span class="label">Battery Temperature:</span> <span id="batteryTemperature" class="value">Loading...</span>
  </div>

  <hr class="sectionDivider">

  <div class="buttonRow-green">
    <a href="./status" class="linkButton">JSON</a>
    <a href="./uiStatus" class="linkButton">UI JSON</a>
    <a href="./metrics" class="linkButton">Metrics</a>
    <a href="./debug" class="linkButton">Log</a>
  </div>

  <div class="buttonRow-yellow">
    <a onclick="return confirm('Start Config AP?');" href="./startAp" class="linkButton yellow">Start Config AP</a>
    <a onclick="return confirm('Reboot?');" href="./reboot" class="linkButton yellow">Reboot</a>
  </div>

  <div class="buttonRow-red">
    <a href="#" onclick="if(confirm('Set priority to load first?')) fetch('/loadfirst'); return false;"
      class="linkButton red">Load First</a>
    <a href="#" onclick="if(confirm('Set priority to battery first?')) fetch('/batteryfirst'); return false;"
      class="linkButton red">Battery First</a>
    <a href="#" onclick="if(confirm('Set priority to grid first?')) fetch('/gridfirst'); return false;"
      class="linkButton red">Grid First</a>
    <a href="./postCommunicationModbus" class="linkButton red">Modbus Access</a>
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
        console.error("Fehler beim Abrufen der Daten:", e);
      }
    }

    loadData();
    setInterval(loadData, 1000);
  </script>
</body>

</html>
)=====";
const char SendPostSite_page[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html lang="en">

<head>
  <meta charset='utf-8'>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Growatt MIN TL-XH</title>
  <style>
    body {
      display: flex;
      flex-direction: column;
      align-items: center;
      max-width: 90vw;
      margin: 0 auto;
      font-family: Arial, sans-serif;
      font-size: 1rem;
      padding: 1em;
    }

    h2 {
      font-size: 2rem;
      text-align: center;
      margin-bottom: 1em;
    }

    form {
      display: flex;
      flex-direction: column;
      gap: 0.7em;
      width: 100%;
      max-width: 400px;
    }

    input[type="text"],
    select {
      padding: 0.5em;
      font-size: 1em;
      border: 1px solid #ccc;
      border-radius: 4px;
      width: 100%;
      box-sizing: border-box;
    }

    .linkButton {
      border-radius: 4px;
      border: 1px solid #91ca5f;
      background: #6eb92b;
      color: #fff;
      padding: 0.6em 1em;
      text-decoration: none;
      font-size: 0.9em;
      min-width: 120px;
      text-align: center;
      display: inline-block;
    }

    .linkButton.yellow {
      border-color: #cabd5f !important;
      background: #b9b22b !important;
    }

    .linkButton.red {
      border-color: #ca5f5f !important;
      background: #b92b2b !important;
    }

    .operationButtons {
      display: flex;
      justify-content: space-between;
      gap: 1em;
      width: 100%;
    }

    .operationButtons button {
      flex: 1 1 45%;
      text-align: center;
    }

    .sectionDivider {
      width: 100%;
      border: none;
      border-top: 2px solid #ccc;
      margin: 2em 0 1.5em 0;
    }

    @media (max-width: 600px) {
      h2 {
        font-size: 1.5rem;
      }

      .linkButton {
        font-size: 1em;
        min-width: auto;
        flex: 1 1 auto;
        text-align: center;
      }

      .operationButtons {
        flex-wrap: nowrap;
        overflow-x: auto;
      }
    }

    .fieldLabel {
      font-weight: normal;
      margin-bottom: 0.2em;
    }
    .valueUpdated {
  animation: highlightField 1s ease;
}

@keyframes highlightField {
  0% { background-color: #6eb92b; }
  50% { background-color: #c5ebaf; }
  100% { background-color: #6eb92b; }
}
.valueError {
  animation: errorFlash 1s ease;
}

@keyframes errorFlash {
  0% { background-color: #b92b2b; }
  50% { background-color: #f5b5b9; }
  100% { background-color: #b92b2b; }

  </style>
</head>

<body>
  <h2>Modbus Access</h2>

  <form id="modbusForm">
    <div>
      <div class="fieldLabel">Register ID</div>
      <input type="text" name="reg" placeholder="" value="0">
    </div>

    <div>
      <div class="fieldLabel">Register Value</div>
      <input type="text" name="val" placeholder="" readonly>
    </div>

    <select name="type">
      <option value="16b" selected>16-bit</option>
      <option value="32b">32-bit</option>
    </select>

    <select name="registerType">
      <option value="I">Input Register</option>
      <option value="H" selected>Holding Register</option>
    </select>

    <div class="operationButtons">
      <button type="button" class="linkButton yellow" onclick="submitOperation('R')">Read</button>
      <button type="button" class="linkButton red" onclick="submitOperation('W')">Write</button>
    </div>

  </form>

  <hr class="sectionDivider">

  <a href="." class="linkButton">Back</a>

  <script>
    const typeSelect = document.querySelector('select[name="type"]');
    const regSelect = document.querySelector('select[name="registerType"]');
    const writeButton = document.querySelector('.linkButton.red');
    const valueInput = document.querySelector('input[name="val"]');

    function updateUI() {
      const is16Bit = typeSelect.value === '16b';
      const is32Bit = typeSelect.value === '32b';
      const isHolding = regSelect.value === 'H';
      const isInput = regSelect.value === 'I';

      // Write-Button nur bei 16-bit UND Holding Register sichtbar
      if (is16Bit && isHolding) {
        writeButton.style.display = 'inline-block';
      } else {
        writeButton.style.display = 'none';
      }

      // Eingabefeld sperren (statt ausblenden), wenn:
      // - Input Register gewählt ist ODER
      // - Holding Register + 32-bit gewählt sind
      if (isInput || (isHolding && is32Bit)) {
        valueInput.readOnly = true;
        valueInput.placeholder = "";
        valueInput.style.opacity = 0.5;
        valueInput.style.cursor = "not-allowed";
      } else {
        valueInput.readOnly = false;
        valueInput.placeholder = "";
        valueInput.style.opacity = 1;
        valueInput.style.cursor = "text";
      }
    }
    // Beim Laden direkt prüfen
    updateUI();

    // Bei jeder Änderung überwachen
    typeSelect.addEventListener('change', updateUI);
    regSelect.addEventListener('change', updateUI);

    // Vorhandene POST-Funktion
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

// Extrahiere den Wert
let extractedValue = trimmed;
const match = trimmed.match(/(?:Reading|Writing) Value\s+(.+?)\s+(?:from|to)/i);
if (match) {
  extractedValue = match[1];
}

if (op === 'R' || op === 'W') {
  valueInput.value = extractedValue;

  // Grünes Highlight bei erfolgreicher Operation
  valueInput.classList.add("valueUpdated");
  setTimeout(() => {
    valueInput.classList.remove("valueUpdated");
  }, 1000);
}

// Rotes Highlight bei Fehler
if (trimmed.toLowerCase().includes("failed")) {
  valueInput.classList.add("valueError");
  setTimeout(() => {
    valueInput.classList.remove("valueError");
  }, 1000);
}

      } catch (e) {
        console.error("Error:", e.message);
      }
    }

  </script>

</body>

</html>
)=====";