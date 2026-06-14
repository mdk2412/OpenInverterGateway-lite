#pragma once
const char MAIN_page[] PROGMEM = R"=====(
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
    <nav style="position: sticky; top: 0; z-index: 10;">
      <ul>
        <li>
          <a href="#" role="button" class="secondary tab active" data-tab="main">
            Dashboard
          </a>
        </li>

        <li>
          <a href="#" role="button" class="secondary tab" data-tab="modbus">
            Modbus
          </a>
        </li>

        <li>
          <a href="#" role="button" class="secondary tab" data-tab="log">
            Log
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
          <a role="button" onclick="if(confirm('Set priority to load first?')) fetch('/loadfirst'); return false;">
            Load First
          </a>

          <a role="button"
            onclick="if(confirm('Set priority to battery first?')) fetch('/batteryfirst'); return false;">
            Battery First
          </a>

          <a role="button" onclick="if(confirm('Set priority to grid first?')) fetch('/gridfirst'); return false;">
            Grid First
          </a>
        </div>
      </fieldset>

      <!-- STATUS BUTTONS -->
      <fieldset class="no-border">
        <div class="grid">
          <a href="./status" role="button" class="secondary">JSON</a>
          <a href="./uiStatus" role="button" class="secondary">UI JSON</a>
          <a href="./metrics" role="button" class="secondary">Metrics</a>
        </div>
      </fieldset>

      <!-- SYSTEM BUTTONS -->
      <fieldset class="no-border">
        <div class="grid">
          <a href="./startAp" role="button" class="contrast" onclick="return confirm('Start Config AP?');">Start Config
            AP</a>

          <a href="./reboot" role="button" class="contrast" onclick="return confirm('Reboot?');">Reboot</a>
        </div>
      </fieldset>

    </section>


    <!-- ========================================================= -->
    <!-- ==================== MODBUS ACCESS ======================= -->
    <!-- ========================================================= -->
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
            <h4>Register Width</h4>

            <label>
              <input type="radio" name="type" value="16b" checked>
              16‑bit
            </label>

            <label>
              <input type="radio" name="type" value="32b">
              32‑bit
            </label>
          </fieldset>

          <fieldset class="no-border">
            <h4>Register Type</h4>

            <label>
              <input type="radio" name="registerType" value="I">
              Input
            </label>

            <label>
              <input type="radio" name="registerType" value="H" checked>
              Holding
            </label>
          </fieldset>

          <fieldset class="no-border">
            <div class="grid">
              <button type="button" onclick="submitOperation('R')">Read</button>
              <button type="button" class="secondary" onclick="submitOperation('W')">Write</button>
            </div>
          </fieldset>

        </fieldset>
      </form>
    </section>

    <!-- ========================================================= -->
    <!-- ==================== Log ================================ -->
    <!-- ========================================================= -->

    <section id="log" class="tab-content" hidden>

      <iframe src="./debug" style="width: 100%; height: 70vh; border: none;"></iframe>

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


      // ============================================================
      // MODBUS ACCESS LOGIC (KORRIGIERT)
      // ============================================================

      const valueInput = document.getElementById("modbusVal");
      const writeButton = document.querySelector("button.secondary");

      // Radio-Helper
      function getSelected(name) {
        return document.querySelector(`input[name="${name}"]:checked`);
      }

      function updateUI() {
        const typeSel = getSelected("type");
        const regSel = getSelected("registerType");

        const is16Bit = typeSel?.value === "16b";
        const is32Bit = typeSel?.value === "32b";
        const isHolding = regSel?.value === "H";
        const isInput = regSel?.value === "I";

        writeButton.style.display = (is16Bit && isHolding) ? "inline-block" : "none";
        valueInput.readOnly = isInput || (isHolding && is32Bit);
      }

      // Initiales UI-Update
      updateUI();

      // Event Listener für Radio Buttons
      document.querySelectorAll('input[name="type"]').forEach(r =>
        r.addEventListener("change", updateUI)
      );

      document.querySelectorAll('input[name="registerType"]').forEach(r =>
        r.addEventListener("change", updateUI)
      );


      // ============================================================
      // SUBMIT OPERATION
      // ============================================================

      async function submitOperation(op) {
        const form = document.getElementById("modbusForm");
        const formData = new FormData(form);
        formData.append("operation", op);

        try {
          const response = await fetch("/postCommunicationModbus_p", {
            method: "POST",
            body: formData
          });

          const trimmed = (await response.text()).trim();

          // Wert extrahieren: Zahl nach "Value"
          let extractedValue = trimmed;
          const match = trimmed.match(/Value\s+(\d+)/i);
          if (match) extractedValue = match[1];

          if (op === 'R') {
            valueInput.value = extractedValue;
            return;
          }

          if (op === 'W') {
            const lower = trimmed.toLowerCase();
            if (lower.includes("failed")) {
              valueInput.value = trimmed;   // komplette Fehlermeldung
            } else {
              valueInput.value = extractedValue;
            }
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