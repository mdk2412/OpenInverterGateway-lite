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
            min-width: 310px;
            max-width: 800px;
            height: 400px;
            margin: 0 auto;
            font-family: Arial;
        }
        h2 {
            font-size: 2.5rem;
            text-align: center;
        }
        div {
            font-size: 1rem;
            padding: 10px 0px 10px 0px;
        }
        .linkButtonBar {
            text-align: center;
            padding: 20px 0px 20px 0px;
        }
        .linkButton {
            -webkit-border-radius: 4px;
            -moz-border-radius: 4px;
            border-radius: 4px;
            border: solid 1px #91ca5f;
            text-shadow: 0 -1px 0 rgba(0, 0, 0, 0.4);
            -webkit-box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.4), 0 1px 1px rgba(0, 0, 0, 0.2);
            -moz-box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.4), 0 1px 1px rgba(0, 0, 0, 0.2);
            box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.4), 0 1px 1px rgba(0, 0, 0, 0.2);
            background: #6eb92b;
            color: #FFF;
            padding: 8px 12px;
            text-decoration: none;
            display: inline-block;
            margin: 2px;
            font-size: .8rem;
            text-align: center;
        }
        .yellow {
            border: solid 1px rgb(202, 189, 95);
            text-shadow: 0 -1px 0 rgba(0, 0, 0, 0.4);
            -webkit-box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.4), 0 1px 1px rgba(0, 0, 0, 0.2);
            -moz-box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.4), 0 1px 1px rgba(0, 0, 0, 0.2);
            box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.4), 0 1px 1px rgba(0, 0, 0, 0.2);
            background:rgb(185, 178, 43);
        }
        .red {
            border: solid 1px rgb(202, 95, 95);
            text-shadow: 0 -1px 0 rgba(0, 0, 0, 0.4);
            -webkit-box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.4), 0 1px 1px rgba(0, 0, 0, 0.2);
            -moz-box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.4), 0 1px 1px rgba(0, 0, 0, 0.2);
            box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.4), 0 1px 1px rgba(0, 0, 0, 0.2);
            background:rgb(185, 43, 43);
        }
    </style>
</head>
<body>
    <h2>Growatt MIN TL-XH</h2>
<h3>Output Power: <span id="outputPower">Lade...</span></h3>
<h3>Priority Mode: <span id="priorityMode">Lade...</span></h3>
<h3>PV Total Power: <span id="pvTotalPower">Lade...</span></h3>
<h3>State of Charge: <span id="stateofCharge">Lade...</span></h3>

<script>
  async function ladeDaten() {
    try {
      const response = await fetch("/status");
      const daten = await response.json();

      document.getElementById("outputPower").textContent = daten.OutputPower + " W";
      const priorityMap = {
      0: "Load First",
      1: "Battery First",
      2: "Grid First"
      };

      document.getElementById("priorityMode").textContent = priorityMap[daten.Priority] || "Unbekannt";

      document.getElementById("pvTotalPower").textContent = daten.PVTotalPower + " V";
      document.getElementById("stateofCharge").textContent = daten.BDCStateOfCharge + " %";
    } catch (err) {
      console.error("Fehler beim Abrufen der Daten:", err);
      document.getElementById("outputPower").textContent = "Fehler";
      document.getElementById("priorityMode").textContent = "Fehler";
      document.getElementById("pvTotalPower").textContent = "Fehler";
      document.getElementById("stateofCharge").textContent = "Fehler";
    }
  }

  ladeDaten();
  setInterval(ladeDaten, 5000); // alle 5 Sekunden aktualisieren
</script>

    <div class="linkButtonBar">
        <a href="./status" class="linkButton">JSON</a>
        <a href="./uiStatus" class="linkButton">UI JSON</a>
        <a href="./metrics" class="linkButton">Metrics</a>
        <a href="./debug" class="linkButton">Log</a><br>
        <a onClick="return confirm('Start Config AP?');" href="./startAp" class="linkButton yellow">Start Config AP</a>
        <a onClick="return confirm('Reboot?');" href="./reboot" class="linkButton yellow">Reboot</a><br>
        <a onClick="return confirm('Set priority to load first?');" href="./loadfirst" class="linkButton red">Load First</a>
        <a onClick="return confirm('Set priority to battery first?');" href="./batteryfirst" class="linkButton red">Battery First</a>
        <a onClick="return confirm('Set priority to grid first?');" href="./gridfirst" class="linkButton red">Grid First</a>
        <a href="./postCommunicationModbus" class="linkButton red">RW Modbus</a>
    </div>
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
</head>
<body>
  <h2>POST Communication Modbus</h2>
  <form action="/postCommunicationModbus_p" method="POST">
    <input type="text" name="reg" placeholder="Register ID"><br>
    <input type="text" name="val" placeholder="Input Value (16-bit only!)"><br>
    <select name="type">
      <option value="16b" selected>16-bit</option>
      <option value="32b">32-bit</option>
    </select><br>
    <select name="operation">
      <option value="R">Read</option>
      <option value="W" selected>Write</option>
    </select><br>
    <select name="registerType">
      <option value="I">Input Register</option>
      <option value="H" selected>Holding Register</option>
    </select><br>
    <input type="submit" value="Submit">
  </form><br>
  <a href=".">back</a>
</body>
</html>
)=====";
