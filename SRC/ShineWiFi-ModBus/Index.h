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
    <nav>
      <ul>
        <li>
          <a href="#" role="button" class="secondary outline tab active" data-tab="main">
            Dashboard
          </a>
        </li>

        <li>
          <a href="#" role="button" class="secondary outline tab" data-tab="modbus">
            Modbus
          </a>
        </li>

        <li>
          <a href="#" role="button" class="secondary outline tab" data-tab="log">
            Log
          </a>
        </li>

        <li>
          <a href="#" role="button" class="secondary outline tab" data-tab="system">
            System
          </a>
        </li>

        <li>
          <a href="#" role="button" class="secondary outline tab" data-tab="extras">
            Extras
          </a>
        </li>
      </ul>
    </nav>

    <!-- Dashboard -->
    <section id="main" class="tab-content">

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

      <!-- PRIORITY BUTTONS -->
      <fieldset class="no-border">
        <div class="grid">
          <button type="button" onclick="if (confirm('Set priority to load first?')) fetch('/loadfirst');">
            Load First
          </button>

          <button type="button" class="secondary"
            onclick="if (confirm('Set priority to battery first?')) fetch('/batteryfirst');">
            Battery First
          </button>

          <button type="button" class="contrast"
            onclick="if (confirm('Set priority to grid first?')) fetch('/gridfirst');">
            Grid First
          </button>

        </div>
      </fieldset>

    </section>

    <!-- MODBUS -->

    <section id="modbus" class="tab-content" hidden>
      <form id="modbusForm">
        <fieldset class="no-border">
          <label>
            Register ID
            <input type="text" name="reg">
          </label>

          <label>
            Register Value
            <input type="text" name="val" id="modbusVal" readonly>
          </label>

          <fieldset class="no-border">
            Register Width

            <label>
              <input type="radio" name="width" value="16b" checked>
              16-bit
            </label>

            <label>
              <input type="radio" name="width" value="32b">
              32-bit
            </label>
          </fieldset>

          <fieldset class="no-border">
            Register Type

            <label>
              <input type="radio" name="type" value="I">
              Input
            </label>

            <label>
              <input type="radio" name="type" value="H" checked>
              Holding
            </label>
          </fieldset>

          <fieldset class="no-border">
            <div class="grid">
              <button type="button" onclick="submitOperation('R')">Read</button>
              <button type="button" class="contrast" onclick="submitOperation('W')">Write</button>
            </div>
          </fieldset>

        </fieldset>
      </form>
    </section>

    <!-- Log -->

    <section id="log" class="tab-content" hidden>

      <iframe src="./debug" style="width: 100%; height: 70vh; border: none;"></iframe>

    </section>

    <!-- System -->

    <section id="system" class="tab-content" hidden>

      <!-- STATUS BUTTONS -->
      <fieldset class="no-border">
        <div class="grid">
          <button type="button" onclick="location.href='./status'">JSON</button>
          <button type="button" onclick="location.href='./uiStatus'">UI JSON</button>
          <button type="button" onclick="location.href='./metrics'">Metrics</button>
        </div>
      </fieldset>

      <!-- SYSTEM BUTTONS -->
      <fieldset class="no-border">
        <div class="grid">
          <button type="button" class="contrast" onclick="if (confirm('Start Config AP?')) location.href='./startAp'">
            Start Config AP
          </button>

          <button type="button" class="contrast" onclick="if (confirm('Reboot?')) location.href='./reboot'">
            Reboot
          </button>
        </div>
      </fieldset>

    </section>

    <!-- Extras -->

    <section id="extras" class="tab-content" hidden>

      <h3>Battery Standby Settings</h3>
      <form id="extrasForm">
        <fieldset class="no-border">
          <label>
            Battery Sleep Threshold (W)
            <input type="number" name="bat_slp_thr" id="bat_slp_thr" min="0" step="1" placeholder="50">
          </label>

          <label>
            Battery Wake Threshold (W)
            <input type="number" name="bat_wke_thr" id="bat_wke_thr" min="0" step="1" placeholder="75">
          </label>

          <h3>AC Charge Control Settings</h3>

          <label>
            Inverter Maximum Power (W)
            <input type="number" name="ac_max_pow" id="ac_max_pow" min="0" step="1" placeholder="2500">
          </label>

          <label>
            Offset (%)
            <input type="number" name="ac_off_set" id="ac_off_set" min="0" max="100" step="1" placeholder="1">
          </label>

          <button type="button" class="contrast" onclick="saveExtras()">Save Extras</button>
        </fieldset>
      </form>

      <div id="extrasStatus"></div>

    </section>

    <!-- JAVASCRIPT -->

    <script>
      // TAB SWITCHING
      document.querySelectorAll(".tab").forEach(tab => {
        tab.addEventListener("click", (e) => {
          e.preventDefault();

          document.querySelector(".tab.active")?.classList.remove("active");

          // neuen Tab aktiv setzen
          tab.classList.add("active");

          const target = tab.dataset.tab;

          // Inhalte umschalten
          document.querySelectorAll(".tab-content").forEach(sec => {
            sec.hidden = sec.id !== target;
          });
        });
      });

      // MAIN PAGE AUTO-UPDATE
      async function loadData() {
        // Dashboard nur aktualisieren, wenn es sichtbar ist
        const dashboard = document.getElementById("main");
        if (dashboard.hidden) return;

        try {
          const response = await fetch("/uiStatus");
          if (!response.ok) {
            console.error("HTTP error:", response.status);
            return;
          }

          const data = await response.json();

          // Werte setzen
          document.getElementById("priorityMode").textContent =
            data.Priority[0] + " " + data.Priority[1];

          document.getElementById("outputPower").textContent =
            data.OutputPower[0] + " " + data.OutputPower[1];

          document.getElementById("pv2Power").textContent =
            data.PV2Power[0] + " " + data.PV2Power[1];

          document.getElementById("pv2Voltage").textContent =
            data.PV2Voltage[0] + " " + data.PV2Voltage[1];

          document.getElementById("inverterTemperature").textContent =
            data.InverterTemperature[0] + " " + data.InverterTemperature[1];

          document.getElementById("stateofCharge").textContent =
            data.BDCStateOfCharge[0] + " " + data.BDCStateOfCharge[1];

          document.getElementById("batteryCharge").textContent =
            data.BDCChargePower[0] + " " + data.BDCChargePower[1] +
            " (" + data.BDCChargePowerRate[0] + " " + data.BDCChargePowerRate[1] + ")";

          document.getElementById("batteryDischarge").textContent =
            data.BDCDischargePower[0] + " " + data.BDCDischargePower[1] +
            " (" + data.BDCDischargePowerRate[0] + " " + data.BDCDischargePowerRate[1] + ")";

          document.getElementById("batteryTemperature").textContent =
            data.BDCTemperatureA[0] + " " + data.BDCTemperatureA[1];

        } catch (e) {
          console.error("Error fetching data:", e);
        }
      }

      loadData();
      setInterval(loadData, 1000);

      // MODBUS ACCESS LOGIC
      const valueInput = document.getElementById("modbusVal");
      const writeButton = document.querySelector("#modbusForm button.contrast");

      // Radio-Helper
      function getSelected(name) {
        return document.querySelector(`input[name="${name}"]:checked`)?.value;
      }

      function updateUI() {
        const width = getSelected("width");   // 16b / 32b
        const type = getSelected("type");    // I / H

        const disableWrite = (width === "32b" || type === "I");

        // PicoCSS: disabled = automatisch ausgegraut
        writeButton.disabled = disableWrite;

        // Value-Feld readonly bei Input oder 32-bit
        valueInput.readOnly = disableWrite;
      }

      // Initiales UI-Update
      updateUI();

      // Event Listener für Radio Buttons
      document.querySelectorAll('input[name="width"], input[name="type"]').forEach(r =>
        r.addEventListener("change", updateUI)
      );

      // SUBMIT OPERATION

      async function submitOperation(op) {
        const form = document.getElementById("modbusForm");
        const data = new FormData(form);

        const payload = new URLSearchParams();
        payload.append("operation", op);
        payload.append("reg", data.get("reg"));
        payload.append("val", data.get("val"));
        payload.append("width", data.get("width"));
        payload.append("type", data.get("type"));

        try {
          const response = await fetch("/postCommunicationModbus_p", {
            method: "POST",
            body: payload
          });

          const trimmed = (await response.text()).trim();

          let extractedValue = trimmed;
          const match = trimmed.match(/Value\s+(\d+)/i);
          if (match) extractedValue = match[1];

          if (op === "R") {
            if (trimmed.toLowerCase().includes("succeeded")) {
              valueInput.value = extractedValue;
            } else {
              valueInput.value = trimmed;
            }
            return;
          }

          if (op === "W") {
            if (trimmed.toLowerCase().includes("succeeded")) {
              valueInput.value = extractedValue;
            } else {
              valueInput.value = trimmed;
            }
          }

        } catch (e) {
          console.error("Error:", e.message);
        }
      }

      // EXTRAS SETTINGS LOGIC
      async function loadExtras() {
        try {
          const response = await fetch("/getExtras");
          if (!response.ok) {
            console.error("HTTP error:", response.status);
            return;
          }

          const data = await response.json();

          if (data.bat_slp_thr) document.getElementById("bat_slp_thr").value = data.bat_slp_thr;
          if (data.bat_wke_thr) document.getElementById("bat_wke_thr").value = data.bat_wke_thr;
          if (data.ac_max_pow) document.getElementById("ac_max_pow").value = data.ac_max_pow;
          if (data.ac_off_set) document.getElementById("ac_off_set").value = data.ac_off_set;

        } catch (e) {
          console.error("Error loading extras:", e);
        }
      }

      async function saveExtras() {
        const bat_slp_thr = document.getElementById("bat_slp_thr").value;
        const bat_wke_thr = document.getElementById("bat_wke_thr").value;
        const ac_max_pow = document.getElementById("ac_max_pow").value;
        const ac_off_set = document.getElementById("ac_off_set").value;

        const payload = new URLSearchParams();
        if (bat_slp_thr) payload.append("bat_slp_thr", bat_slp_thr);
        if (bat_wke_thr) payload.append("bat_wke_thr", bat_wke_thr);
        if (ac_max_pow) payload.append("ac_max_pow", ac_max_pow);
        if (ac_off_set) payload.append("ac_off_set", ac_off_set);

        try {
          const response = await fetch("/saveExtras", {
            method: "POST",
            body: payload
          });

          const result = await response.text();
          const statusDiv = document.getElementById("extrasStatus");
          statusDiv.innerHTML = `<p style="color: green;"><strong>${result}</strong></p>`;

          // Clear status after 3 seconds
          setTimeout(() => {
            statusDiv.innerHTML = "";
          }, 3000);

        } catch (e) {
          const statusDiv = document.getElementById("extrasStatus");
          statusDiv.innerHTML = `<p style="color: red;"><strong>Error: ${e.message}</strong></p>`;
          console.error("Error saving extras:", e);
        }
      }

      // Load extras when page loads
      loadExtras();

    </script>

  </main>
</body>

</html>
)=====";