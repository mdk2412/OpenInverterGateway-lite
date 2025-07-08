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

    .dataContainer {
      display: flex;
      flex-direction: column;
      gap: 0.7em;
      width: 100%;
    }

    h3 {
      font-size: 1.1em;
      margin: 0;
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

    .yellow {
      border-color: #cabd5f;
      background: #b9b22b;
    }

    .red {
      border-color: #ca5f5f;
      background: #b92b2b;
    }

@media (max-width: 600px) {
  h2 {
    font-size: 1.5rem;
  }

  .linkButton {
    font-size: 1em;
    min-width: auto;           /* Verhindert, dass Buttons blockweise umbrechen */
    flex: 1 1 auto;            /* Sorgt für gleichmäßige Verteilung */
    text-align: center;
  }

  .buttonRow-green,
  .buttonRow-yellow,
  .buttonRow-red {
    flex-wrap: nowrap;         /* Verhindert Zeilenumbruch */
    overflow-x: auto;          /* Ermöglicht horizontales Scrollen bei Bedarf */
  }
}

.sectionDivider {
  width: 100%;
  border: none;
  border-top: 2px solid #ccc;
  margin: 2em 0 1.5em 0;
}

  </style>
</head>
<body>
  <h2>Growatt MIN TL-XH</h2>

  <div class="dataContainer">
    <h3>Priority Mode: <span id="priorityMode">Loading...</span></h3>
    <h3>Output Power: <span id="outputPower">Loading...</span></h3>
    <h3>PV2 Power: <span id="pv2Power">Loading...</span></h3>
    <h3>PV2 Voltage: <span id="pv2Voltage">Loading...</span></h3>
    <h3>Inverter Temperature: <span id="inverterTemperature">Loading...</span></h3>
    <h3>State of Charge: <span id="stateofCharge">Loading...</span></h3>
    <h3>Charging Power: <span id="batteryCharge">Loading...</span></h3>
    <h3>Discharging Power: <span id="batteryDischarge">Loading...</span></h3>
    <h3>Battery Temperature: <span id="batteryTemperature">Loading...</span></h3>
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
    <a href="#" onclick="if(confirm('Set priority to load first?')) fetch('/loadfirst'); return false;" class="linkButton red">Load First</a>
    <a href="#" onclick="if(confirm('Set priority to battery first?')) fetch('/batteryfirst'); return false;" class="linkButton red">Battery First</a>
    <a href="#" onclick="if(confirm('Set priority to grid first?')) fetch('/gridfirst'); return false;" class="linkButton red">Grid First</a>
    <a href="./postCommunicationModbus" class="linkButton red">RW Modbus</a>
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
    }

    .linkButton {
      border-radius: 4px;
      border: 1px solid #91ca5f;
      background: #6eb92b;
      color: #fff;
      padding: 0.6em 1em;
      text-decoration: none;
      font-size: 0.9em;
      text-align: center;
      display: inline-block;
    }

    .red {
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
    }
  </style>
</head>
<body>
  <h2>POST Communication Modbus</h2>

  <form action="/postCommunicationModbus_p" method="POST">
    <input type="text" name="reg" placeholder="Register ID">
    <input type="text" name="val" placeholder="Input Value (16-bit only!)">
    <select name="type">
      <option value="16b" selected>16-bit</option>
      <option value="32b">32-bit</option>
    </select>
    <select name="operation">
      <option value="R">Read</option>
      <option value="W" selected>Write</option>
    </select>
    <select name="registerType">
      <option value="I">Input Register</option>
      <option value="H" selected>Holding Register</option>
    </select>

    <button type="submit" class="linkButton red">Submit</button>
  </form>

  <hr class="sectionDivider">

  <a href="." class="linkButton">Back</a>
</body>
</html>
)=====";
