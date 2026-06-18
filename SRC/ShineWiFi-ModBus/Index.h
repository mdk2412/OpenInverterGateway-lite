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
          <a href="#" role="button" class="secondary outline tab" data-tab="settings">
            Settings
          </a>
        </li>
      </ul>
    </nav>

    <!-- Dashboard -->
    <section id="main" class="tab-content">

      <table>
        <tbody>
          <tr>
            <th>Priority Mode</th>
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
      <fieldset>
        <div class="grid">
          <button type="button" onclick="if (confirm('Set priority to load first?')) fetch('/loadfirst')">Load
            First</button>
          <button type="button" class="secondary"
            onclick="if (confirm('Set priority to battery first?')) fetch('/batteryfirst')">Battery First</button>
          <button type="button" class="contrast"
            onclick="if (confirm('Set priority to grid first?')) fetch('/gridfirst')">Grid First</button>
        </div>
      </fieldset>

    </section>

    <!-- MODBUS -->

    <section id="modbus" class="tab-content" hidden>
      <form id="modbusForm">
        <fieldset>
          <label>
            Register ID
            <input type="text" name="reg">
          </label>

          <label>
            Register Value
            <input type="text" name="val" id="modbusVal" readonly>
          </label>

          <fieldset>
            Register Width
            <label><input type="radio" name="width" value="16b" checked> 16-bit</label>
            <label><input type="radio" name="width" value="32b"> 32-bit</label>
          </fieldset>

          <fieldset>
            Register Type
            <label><input type="radio" name="type" value="I"> Input</label>
            <label><input type="radio" name="type" value="H" checked> Holding</label>
          </fieldset>

          <fieldset>
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
      <iframe src="./debug" style="width: 100%; height: 75vh; border: none;"></iframe>
    </section>

    <!-- System -->

    <section id="system" class="tab-content" hidden>
      <fieldset>
        <div class="grid">
          <button type="button" onclick="location.href='./status'">JSON</button>
          <button type="button" onclick="location.href='./uiStatus'">UI JSON</button>
          <button type="button" onclick="location.href='./metrics'">Metrics</button>
        </div>
      </fieldset>

      <fieldset>
        <div class="grid">
          <button type="button" class="contrast"
            onclick="if (confirm('Start Config AP?')) location.href='./startAp'">Start Config AP</button>
          <button type="button" class="contrast"
            onclick="if (confirm('Reboot?')) location.href='./reboot'">Reboot</button>
        </div>
      </fieldset>
    </section>

    <!-- Settings -->

    <section id="settings" class="tab-content" hidden>
      <form id="settingsForm">
        <fieldset>
          <hr>
          <label><input name="bat_standby" type="checkbox" role="switch"> Battery Standby</label>

          <label>Sleep Threshold (W)
            <input type="number" name="bat_slp_thr" id="bat_slp_thr" min="0" step="1" placeholder="50">
          </label>

          <label>Wake Threshold (W)
            <input type="number" name="bat_wke_thr" id="bat_wke_thr" min="0" step="1" placeholder="75">
          </label>
          <hr>
          <label><input name="accharge" type="checkbox" role="switch"> AC Charging</label>

          <label>Inverter Maximum Power (W)
            <input type="number" name="ac_max_pow" id="ac_max_pow" min="0" step="1" placeholder="2500">
          </label>

          <label>Offset (%)
            <input type="number" name="ac_off_set" id="ac_off_set" min="0" max="100" step="1" placeholder="1">
          </label>

          <fieldset>
            <div class="grid">
              <button type="button" class="contrast" onclick="saveSettings()">Save Settings</button>
            </div>
          </fieldset>

        </fieldset>
      </form>

    </section>

    <!-- JAVASCRIPT -->

    <script>
      document.addEventListener("DOMContentLoaded", () => {

        // TAB SWITCHING
        document.querySelectorAll(".tab").forEach(tab => {
          tab.addEventListener("click", (e) => {
            e.preventDefault();

            document.querySelector(".tab.active")?.classList.remove("active");
            tab.classList.add("active");

            const target = tab.dataset.tab;

            document.querySelectorAll(".tab-content").forEach(sec => {
              sec.hidden = sec.id !== target;
            });
          });
        });

        // MAIN PAGE AUTO-UPDATE
        async function loadData() {
          const dashboard = document.getElementById("main");
          if (dashboard.hidden) return;

          try {
            const response = await fetch("/uiStatus");
            if (!response.ok) return;

            const data = await response.json();

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

        function getSelected(name) {
          return document.querySelector(`input[name="${name}"]:checked`)?.value;
        }

        function updateDashboard() {
          const width = getSelected("width");
          const type = getSelected("type");

          const disableWrite = (width === "32b" || type === "I");

          writeButton.disabled = disableWrite;
          valueInput.disabled = disableWrite;
        }

        updateDashboard();

        document.querySelectorAll('input[name="width"], input[name="type"]').forEach(r =>
          r.addEventListener("change", updateDashboard)
        );

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
              valueInput.value = trimmed.toLowerCase().includes("succeeded")
                ? extractedValue
                : trimmed;
              return;
            }

            if (op === "W") {
              valueInput.value = trimmed.toLowerCase().includes("succeeded")
                ? extractedValue
                : trimmed;
            }

          } catch (e) {
            console.error("Error:", e.message);
          }
        }

        // SETTINGS LOGIC
        async function loadSettings() {
          try {
            const response = await fetch("/getSettings");
            if (!response.ok) return;

            const data = await response.json();

            document.querySelector('input[name="bat_standby"]').checked = !!data.bat_standby;
            document.querySelector('input[name="accharge"]').checked = !!data.accharge;

            if (data.bat_slp_thr) document.getElementById("bat_slp_thr").value = data.bat_slp_thr;
            if (data.bat_wke_thr) document.getElementById("bat_wke_thr").value = data.bat_wke_thr;
            if (data.ac_max_pow) document.getElementById("ac_max_pow").value = data.ac_max_pow;
            if (data.ac_off_set) document.getElementById("ac_off_set").value = data.ac_off_set;

          } catch (e) {
            console.error("Error loading settings:", e);
          }
        }

        async function saveSettings() {
          const form = document.getElementById("settingsForm");
          const btn = document.querySelector('button[onclick="saveSettings()"]');

          const payload = new URLSearchParams();

          if (form.bat_standby.checked) payload.append("bat_standby", "on");
          if (form.accharge.checked) payload.append("accharge", "on");

          if (form.bat_slp_thr.value) payload.append("bat_slp_thr", form.bat_slp_thr.value);
          if (form.bat_wke_thr.value) payload.append("bat_wke_thr", form.bat_wke_thr.value);
          if (form.ac_max_pow.value) payload.append("ac_max_pow", form.ac_max_pow.value);
          if (form.ac_off_set.value) payload.append("ac_off_set", form.ac_off_set.value);

          const oldText = btn.textContent;
          const oldClass = btn.className;

          try {
            const response = await fetch("/saveSettings", {
              method: "POST",
              body: payload
            });

            const result = await response.text();

            btn.textContent = result;
            btn.className = "contrast outline";

            setTimeout(() => {
              btn.textContent = oldText;
              btn.className = oldClass;
            }, 2000);

          } catch (e) {
            btn.textContent = "Error: " + e.message;
            btn.className = "contrast outline";

            setTimeout(() => {
              btn.textContent = oldText;
              btn.className = oldClass;
            }, 3000);
          }
        }

        loadSettings();

      });
    </script>

  </main>
</body>

</html>
)=====";