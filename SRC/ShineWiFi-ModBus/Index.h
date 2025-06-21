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
        h3 {
            font-family: 'Times New Roman', sans-serif;
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
<h3 style="position: relative; left: 20px;">Output Power: <span id="outputPower">Loading...</span></h3>
<h3 style="position: relative; left: 20px;">Priority Mode: <span id="priorityMode">Loading...</span></h3>
<h3 style="position: relative; left: 20px;">PV2 Power: <span id="pv2Power">Loading...</span></h3>
<h3 style="position: relative; left: 20px;">PV2 Voltage: <span id="pv2Voltage">Loading...</span></h3>
<h3 style="position: relative; left: 20px;">State of Charge: <span id="stateofCharge">Loading...</span></h3>
<h3 style="position: relative; left: 20px;">Battery Charging Power: <span id="batteryCharge">Loading...</span></h3>
<h3 style="position: relative; left: 20px;">Battery Discharging Power: <span id="batteryDischarge">Loading...</span></h3>
<h3 style="position: relative; left: 20px;">Inverter Temperature: <span id="inverterTemperature">Loading...</span></h3>

<script>
  async function loadData() {
    try {
      const response = await fetch("/uiStatus");
      const data = await response.json();
      document.getElementById("outputPower").textContent = data.OutputPower[0] + " " + data.OutputPower[1];
      document.getElementById("priorityMode").textContent = data.Priority[0] + " " + data.Priority[1];
      document.getElementById("pv2Power").textContent = data.PV2Power[0] + " " + data.PV2Power[1];
      document.getElementById("pv2Voltage").textContent = data.PV2Voltage[0] + " " + data.PV2Voltage[1];
      document.getElementById("stateofCharge").textContent = data.BDCStateOfCharge[0] + " " + data.BDCStateOfCharge[1];
      document.getElementById("batteryCharge").textContent = data.BDCChargePower[0] + " " + data.BDCChargePower[1];
      document.getElementById("batteryDischarge").textContent = data.BDCDischargePower[0] + " " + data.BDCDischargePower[1];
      document.getElementById("inverterTemperature").textContent = data.InverterTemperature[0] + " " + data.InverterTemperature[1];
      } 
    finally {
    }
  }

  loadData();
  setInterval(loadData, 1000); 
</script>

    <div class="linkButtonBar">
        <a href="./status" class="linkButton">JSON</a>
        <a href="./uiStatus" class="linkButton">UI JSON</a>
        <a href="./metrics" class="linkButton">Metrics</a>
        <a href="./debug" class="linkButton">Log</a><br>
        <a onClick="return confirm('Start Config AP?');" href="./startAp" class="linkButton yellow">Start Config AP</a>
        <a onClick="return confirm('Reboot?');" href="./reboot" class="linkButton yellow">Reboot</a><br>
        <a href="#" onclick="if(confirm('Set priority to load first?')) fetch('/loadfirst'); return false;" class="linkButton red">Load First</a>
        <a href="#" onclick="if(confirm('Set priority to battery first?')) fetch('/batteryfirst'); return false;" class="linkButton red">Battery First</a>
        <a href="#" onclick="if(confirm('Set priority to grid first?')) fetch('/gridfirst'); return false;" class="linkButton red">Grid First</a>
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
