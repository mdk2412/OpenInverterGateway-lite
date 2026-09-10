#pragma once

const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Growatt MIN TL-XH</title>
  <link rel="stylesheet" href="/pico.lime.min.css">
</head>

<body>
  <main class="container">
    <nav>
      <ul>
        <li><button class="outline tab active" data-tab="main">Dashboard</button></li>
        <li><button class="outline tab" data-tab="settings">Settings</button></li>
        <li><button class="outline tab" data-tab="system">System</button></li>
        <li><button class="outline tab" data-tab="modbus">Modbus</button></li>
        <li><button class="outline tab" data-tab="log">Log</button></li>
        <li><button class="outline tab" data-tab="update">Update</button></li>
      </ul>
    </nav>

    <!-- MAIN DASHBOARD -->
    <section id="main" class="tab-content">
      <table>
        <tbody>
          <tr>
            <th>On/Off Mode</th>
            <td id="onoffMode">Loading...</td>
          </tr>
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
            <th>Charging Power<br>(Limit)</th>
            <td id="batteryCharge">Loading...</td>
          </tr>
          <tr>
            <th>Discharging Power<br>(Limit)</th>
            <td id="batteryDischarge">Loading...</td>
          </tr>
          <tr>
            <th>Battery Temperature</th>
            <td id="batteryTemperature">Loading...</td>
          </tr>
        </tbody>
      </table>

      <div class="grid" id="priorityButtons">
        <button type="button" id="btn-loadfirst" onclick="fetch('/loadfirst')">Load First</button>
        <button type="button" id="btn-batteryfirst" class="secondary" onclick="fetch('/batteryfirst')">Battery
          First</button>
        <button type="button" id="btn-gridfirst" class="contrast"
          onclick="if(confirm('Set priority to grid first?'))fetch('/gridfirst')">Grid First</button>
      </div>
    </section>

    <!-- MODBUS -->
    <section id="modbus" class="tab-content" hidden>
      <form id="modbusForm">
        <label>Register ID<input type="number" name="reg"></label>
        <label>Register Value<input type="number" name="val" id="modbusVal"></label>
        Register Width
        <label><input type="radio" name="width" value="16b" checked> 16-bit</label>
        <label><input type="radio" name="width" value="32b"> 32-bit</label>
        Register Type
        <label><input type="radio" name="type" value="I"> Input</label>
        <label><input type="radio" name="type" value="H" checked> Holding</label>
        <div class="grid">
          <button type="button" onclick="submitOperation('R')">Read</button>
          <button type="button" id="modbusWriteButton" class="secondary" onclick="submitOperation('W')">Write</button>
        </div>
      </form>
    </section>

    <!-- LOG -->
    <section id="log" class="tab-content" hidden>
      <iframe id="debugFrame" style="width:100%;height:75vh;border:none"></iframe>
    </section>

    <!-- SYSTEM -->
    <section id="system" class="tab-content" hidden>
      <div class="grid">
        <button type="button" onclick="location.href='./uiStatus'">UI JSON</button>
      </div>
      <div class="grid">
        <button type="button" class="secondary" onclick="if(confirm('Start Config AP?'))location.href='./startAp'">Start
          Config AP</button>
        <button type="button" class="secondary" onclick="if(confirm('Reboot?'))location.href='./reboot'">Reboot</button>
      </div>
    </section>

    <!-- SETTINGS -->
    <section id="settings" class="tab-content" hidden>
      <form id="settingsForm">
        <hr>
        <label><input name="bat_standby" type="checkbox" role="switch"> Battery Standby</label>
        <label>Sleep Threshold (W)<input type="number" name="bat_slp_thr" id="bat_slp_thr" min="0" step="1"
            placeholder="0"></label>
        <label>Wake Threshold (W)<input type="number" name="bat_wke_thr" id="bat_wke_thr" min="0" step="1"
            placeholder="0"></label>
        <hr>
        <label><input name="accharge" type="checkbox" role="switch"> AC Charging</label>
        <label>Inverter Maximum Power (W)<input type="number" name="ac_max_pow" id="ac_max_pow" min="2500" max="12500"
            step="50" placeholder="2500"></label>
        <label>Offset (W)<input type="number" name="ac_off_set" id="ac_off_set" min="-100" max="100" step="1"
            placeholder="0"></label>
        <hr>
        <label><input name="prioctrl" type="checkbox" role="switch"> Priority Control</label>
        <label>Power to Grid threshold (W)<input type="number" name="ptogrid_thr" id="ptogrid_thr" min="0" step="1"
            placeholder="0"></label>
        <label>Power to User threshold (W)<input type="number" name="ptouser_thr" id="ptouser_thr" min="0" step="1"
            placeholder="0"></label>
        <hr>
        <label><input name="surch" type="checkbox" role="switch"> Surplus Charging</label>
        <label>Power Limit (W)<input type="number" name="power_limit" id="power_limit" min="0" step="1"
            placeholder="0"></label>
        <div class="grid">
          <button type="button" onclick="saveSettings()">Save Settings</button>
        </div>
      </form>
    </section>

    <!-- UPDATE -->
    <section id="update" class="tab-content" hidden>
      <form method="POST" action="/update" enctype="multipart/form-data">
        <label>Choose Firmware File (.bin)<input type="file" name="firmware" accept=".bin" required></label>
        <div class="grid">
          <button type="submit">Start Update</button>
        </div>
      </form>
    </section>

    <!-- JAVASCRIPT -->
    <script>
      document.addEventListener("DOMContentLoaded", () => {
        window.setActivePriority = function (activeBtn) {
          const container = document.getElementById("priorityButtons");
          if (!container) return;
          container.querySelectorAll("button").forEach(btn => {
            if (btn === activeBtn) btn.classList.add("outline");
            else btn.classList.remove("outline");
          });
        };

        function highlightPriorityButton(priorityArray) {
          if (!priorityArray || !Array.isArray(priorityArray)) return;
          const prioText = priorityArray.join(" ").toLowerCase();
          const btnLoad = document.getElementById("btn-loadfirst");
          const btnBat = document.getElementById("btn-batteryfirst");
          const btnGrid = document.getElementById("btn-gridfirst");

          if (btnLoad && btnBat && btnGrid) {
            btnLoad.classList.toggle("outline", !prioText.includes("load"));
            btnBat.classList.toggle("outline", !prioText.includes("battery"));
            btnGrid.classList.toggle("outline", !prioText.includes("grid"));
          }
        }

        // Tab Switching
        document.addEventListener("click", e => {
          const tabBtn = e.target.closest(".tab");
          if (tabBtn) {
            document.querySelector(".tab.active")?.classList.remove("active");
            tabBtn.classList.add("active");
            document.querySelectorAll(".tab-content").forEach(sec => sec.hidden = (sec.id !== tabBtn.dataset.tab));

            if (tabBtn.dataset.tab === "settings") loadSettings();

            // NEU: Log-Iframe erst beim ersten Klick auf den Log-Tab laden
            if (tabBtn.dataset.tab === "log") {
              const iframe = document.getElementById("debugFrame");
              if (iframe && !iframe.src) iframe.src = "./debug";
            }
          }
        });

        // Main Refresh
        async function refreshDashboard() {
          const mainSec = document.getElementById("main");
          if (mainSec.hidden) return;
          try {
            const res = await fetch("/uiStatus");
            if (!res.ok) return;
            const data = await res.json();
            highlightPriorityButton(data.Priority);

            const updateCell = (id, valArray, rateArray = null) => {
              const el = document.getElementById(id);
              if (el && Array.isArray(valArray)) {
                el.innerHTML = rateArray
                  ? `${valArray.join(" ")}<br>(${rateArray.join(" ")})`
                  : valArray.join(" ");
              }
            };

            const map = {
              onoffMode: data.OnOff,
              priorityMode: data.Priority,
              outputPower: data.OutputPower,
              pv2Power: data.PV2Power,
              pv2Voltage: data.PV2Voltage,
              inverterTemperature: data.InverterTemperature,
              stateofCharge: data.BDCStateOfCharge,
              batteryTemperature: data.BDCTemperatureA
            };

            for (const key in map) updateCell(key, map[key]);
            updateCell("batteryCharge", data.BDCChargePower, data.BDCChargePowerRate);
            updateCell("batteryDischarge", data.BDCDischargePower, data.BDCDischargePowerRate);
          } catch (err) {
            console.error("Error fetching Data:", err);
          }
        }

        // Modbus Handling
        const modbusVal = document.getElementById("modbusVal");
        const modbusWriteBtn = document.getElementById("modbusWriteButton");

        function getRadioVal(name) {
          return document.querySelector(`input[name="${name}"]:checked`)?.value;
        }

        function updateModbusState() {
          const isDisable = (getRadioVal("width") === "32b" || getRadioVal("type") === "I");
          modbusWriteBtn.disabled = isDisable;
          modbusVal.disabled = isDisable;
        }

        document.querySelectorAll('input[name="width"], input[name="type"]').forEach(input => {
          input.addEventListener("change", updateModbusState);
        });

        function parseBool(v) {
          if (typeof v === "boolean") return v;
          if (typeof v === "number") return v !== 0;
          if (typeof v === "string") return ["true", "on", "1"].includes(v.trim().toLowerCase());
          return false;
        }

        // Settings Handling
        async function loadSettings() {
          try {
            const res = await fetch("/getSettings");
            if (!res.ok) return;
            const s = await res.json();

            document.querySelector('input[name="bat_standby"]').checked = parseBool(s.bat_standby);
            document.querySelector('input[name="accharge"]').checked = parseBool(s.accharge);
            document.querySelector('input[name="prioctrl"]').checked = parseBool(s.prioctrl);
            document.querySelector('input[name="surch"]').checked = parseBool(s.surch);

            document.getElementById("bat_slp_thr").value = s.bat_slp_thr ?? "";
            document.getElementById("bat_wke_thr").value = s.bat_wke_thr ?? "";
            document.getElementById("ac_max_pow").value = s.ac_max_pow ?? "";
            document.getElementById("ac_off_set").value = s.ac_off_set ?? "";
            document.getElementById("ptogrid_thr").value = s.ptogrid_thr ?? "";
            document.getElementById("ptouser_thr").value = s.ptouser_thr ?? "";
            document.getElementById("power_limit").value = s.power_limit ?? "";
          } catch (err) {
            console.error("Error loading settings:", err);
          }
        }

        window.submitOperation = async function (type) {
          const form = new FormData(document.getElementById("modbusForm"));
          const params = new URLSearchParams();
          params.append("operation", type);
          params.append("reg", form.get("reg"));
          params.append("val", form.get("val"));
          params.append("width", form.get("width"));
          params.append("type", form.get("type"));

          try {
            const res = await fetch("/postCommunicationModbus_p", { method: "POST", body: params });
            const txt = (await res.text()).trim();
            const isErr = txt.toLowerCase().includes("failed");
            let display = txt;
            const match = txt.match(/(\d+)/);
            if (match) display = match[1];
            modbusVal.value = isErr ? txt : display;
          } catch (e) {
            modbusVal.value = "JS Error: " + e.message;
          }
        };

        window.saveSettings = async function () {
          const form = document.getElementById("settingsForm");
          const btn = document.querySelector('#settings button[type="button"]');
          const params = new URLSearchParams();

          params.append("bat_standby", form.bat_standby.checked ? "on" : "off");
          params.append("accharge", form.accharge.checked ? "on" : "off");
          params.append("prioctrl", form.prioctrl.checked ? "on" : "off");
          params.append("surch", form.surch.checked ? "on" : "off");

          params.append("bat_slp_thr", form.bat_slp_thr.value);
          params.append("bat_wke_thr", form.bat_wke_thr.value);
          params.append("ac_max_pow", form.ac_max_pow.value);
          params.append("ac_off_set", form.ac_off_set.value);
          params.append("ptogrid_thr", form.ptogrid_thr.value);
          params.append("ptouser_thr", form.ptouser_thr.value);
          params.append("power_limit", form.power_limit.value);

          const origTxt = btn.textContent;
          const origCls = btn.className;
          btn.classList.add("outline");

          try {
            const res = await fetch("/saveSettings", { method: "POST", body: params });
            btn.textContent = (await res.text()).trim();
          } catch {
            btn.textContent = "Error";
          }
          setTimeout(() => {
            btn.textContent = origTxt;
            btn.className = origCls;
          }, 1000);
        };

        // Init
        updateModbusState();
        refreshDashboard();
        setInterval(() => {
          if (!document.hidden) {
            refreshDashboard();
          }
        }, 1000);
      });
    </script>
  </main>
</body>

</html>
)=====";