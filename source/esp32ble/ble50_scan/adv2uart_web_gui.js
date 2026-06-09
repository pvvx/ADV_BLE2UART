import {
  Adv2UartApi,
  CMD,
  bytesToHex,
  normalizeMac,
} from "./adv2uart_api.mjs";

class WebSerialTransport {
  constructor() {
    this.port = null;
    this.reader = null;
    this.onData = () => {};
    this.onError = () => {};
    this.readLoopRunning = false;
  }

  setDataHandler(fn) {
    this.onData = fn || (() => {});
  }

  setErrorHandler(fn) {
    this.onError = fn || (() => {});
  }

  async requestPort(filters = []) {
    if (!navigator.serial) {
      throw new Error("Web Serial API is not available in this browser");
    }
    try {
      this.port = await navigator.serial.requestPort({ filters });
    } catch (err) {
      if (err && (err.name === "NotFoundError" || err.name === "AbortError")) {
        throw new Error("No port selected by the user.");
      }
      throw err;
    }
    return this.port;
  }

  async open(config = {}) {
    if (!navigator.serial) {
      throw new Error("Web Serial API is not available in this browser");
    }
    const {
      port = null,
      baudRate = 2000000,
      dataBits = 8,
      stopBits = 1,
      parity = "none",
      bufferSize = 4096,
      pulseReset = false,
    } = config;

    if (port) {
      this.port = port;
    }
    if (!this.port) {
      await this.requestPort([]);
    }

    await this.port.open({ baudRate, dataBits, stopBits, parity, bufferSize });

    if (pulseReset && this.port.setSignals) {
      try {
        await this.port.setSignals({ dataTerminalReady: true, requestToSend: true });
        await new Promise((resolve) => setTimeout(resolve, 50));
        await this.port.setSignals({ dataTerminalReady: false, requestToSend: false });
      } catch (err) {
        this.onError(err);
      }
    }

    this._startReadLoop();
  }

  async write(frame) {
    if (!this.port || !this.port.writable) {
      throw new Error("Serial port is not writable");
    }
    const writer = this.port.writable.getWriter();
    try {
      await writer.write(frame);
    } finally {
      writer.releaseLock();
    }
  }

  async close() {
    this.readLoopRunning = false;
    const reader = this.reader;
    if (reader) {
      try {
        await reader.cancel();
      } catch (_err) {
        // ignore
      }
      try {
        reader.releaseLock();
      } catch (_err) {
        // ignore
      }
      this.reader = null;
    }

    if (this.port) {
      try {
        await this.port.close();
      } catch (_err) {
        // ignore
      }
    }
  }

  async _startReadLoop() {
    if (!this.port || !this.port.readable || this.readLoopRunning) {
      return;
    }
    this.readLoopRunning = true;

    while (this.readLoopRunning && this.port && this.port.readable) {
      let reader = null;
      try {
        reader = this.port.readable.getReader();
        this.reader = reader;
        while (this.readLoopRunning) {
          const { value, done } = await reader.read();
          if (done) {
            break;
          }
          if (value && value.length) {
            this.onData(value);
          }
        }
      } catch (err) {
        if (this.readLoopRunning) {
          this.onError(err);
        }
      } finally {
        if (reader) {
          try {
            reader.releaseLock();
          } catch (_err) {
            // ignore
          }
        }
        if (this.reader === reader) {
          this.reader = null;
        }
      }
    }
  }
}

function $(id) {
  return document.getElementById(id);
}

function splitMacList(raw) {
  const out = [];
  for (const part of String(raw || "").split(/[\n,;]/)) {
    const p = part.trim();
    if (!p) {
      continue;
    }
    out.push(normalizeMac(p));
  }
  return out;
}

function parsePinField(value) {
  const v = String(value || "").trim();
  if (!v) {
    throw new Error("Pin is required");
  }
  if (v.startsWith("0x") || v.startsWith("0X")) {
    return parseInt(v.slice(2), 16) & 0xff;
  }
  return parseInt(v, 10) & 0xff;
}

function nowText() {
  return new Date().toLocaleTimeString();
}

const transport = new WebSerialTransport();
const api = new Adv2UartApi(transport, {
  crcDebug: false,
});

const RF_POWER_OPTIONS = {
  "esp32-c3": [
    ["ESP_PWR_LVL_N24 (-24 dBm)", 0],
    ["ESP_PWR_LVL_N21 (-21 dBm)", 1],
    ["ESP_PWR_LVL_N18 (-18 dBm)", 2],
    ["ESP_PWR_LVL_N15 (-15 dBm)", 3],
    ["ESP_PWR_LVL_N12 (-12 dBm)", 4],
    ["ESP_PWR_LVL_N9 (-9 dBm)", 5],
    ["ESP_PWR_LVL_N6 (-6 dBm)", 6],
    ["ESP_PWR_LVL_N3 (-3 dBm)", 7],
    ["ESP_PWR_LVL_N0 (0 dBm)", 8],
    ["ESP_PWR_LVL_P3 (+3 dBm)", 9],
    ["ESP_PWR_LVL_P6 (+6 dBm)", 10],
    ["ESP_PWR_LVL_P9 (+9 dBm)", 11],
    ["ESP_PWR_LVL_P12 (+12 dBm)", 12],
    ["ESP_PWR_LVL_P15 (+15 dBm)", 13],
    ["ESP_PWR_LVL_P18 (+18 dBm)", 14],
    ["ESP_PWR_LVL_P20 (+20 dBm)", 15],
  ],
  "tb-03f-kit": [
    ["+10.46 dBm VBAT", 0x3f],
    ["+10.01 dBm VBAT", 0x3a],
    ["+7.79 dBm VBAT", 0x29],
    ["+5.13 dBm VBAT", 0x1d],
    ["+3.01 dBm VANT", 0xbf],
    ["+0.04 dBm VANT", 0xa9],
    ["-5.03 dBm VANT", 0x96],
    ["-15.88 dBm VANT", 0x86],
    ["-30 dBm", 0xff],
  ],
  "supermini-c6": [
    ["ESP_PWR_LVL_N24 (-24 dBm)", 0],
    ["ESP_PWR_LVL_N21 (-21 dBm)", 1],
    ["ESP_PWR_LVL_N18 (-18 dBm)", 2],
    ["ESP_PWR_LVL_N15 (-15 dBm)", 3],
    ["ESP_PWR_LVL_N12 (-12 dBm)", 4],
    ["ESP_PWR_LVL_N9 (-9 dBm)", 5],
    ["ESP_PWR_LVL_N6 (-6 dBm)", 6],
    ["ESP_PWR_LVL_N3 (-3 dBm)", 7],
    ["ESP_PWR_LVL_N0 (0 dBm)", 8],
    ["ESP_PWR_LVL_P3 (+3 dBm)", 9],
    ["ESP_PWR_LVL_P6 (+6 dBm)", 10],
    ["ESP_PWR_LVL_P9 (+9 dBm)", 11],
    ["ESP_PWR_LVL_P12 (+12 dBm)", 12],
    ["ESP_PWR_LVL_P15 (+15 dBm)", 13],
    ["ESP_PWR_LVL_P18 (+18 dBm)", 14],
    ["ESP_PWR_LVL_P20 (+20 dBm)", 15],
  ],
  "c6-gpio5": [
    ["ESP_PWR_LVL_N24 (-24 dBm)", 0],
    ["ESP_PWR_LVL_N21 (-21 dBm)", 1],
    ["ESP_PWR_LVL_N18 (-18 dBm)", 2],
    ["ESP_PWR_LVL_N15 (-15 dBm)", 3],
    ["ESP_PWR_LVL_N12 (-12 dBm)", 4],
    ["ESP_PWR_LVL_N9 (-9 dBm)", 5],
    ["ESP_PWR_LVL_N6 (-6 dBm)", 6],
    ["ESP_PWR_LVL_N3 (-3 dBm)", 7],
    ["ESP_PWR_LVL_N0 (0 dBm)", 8],
    ["ESP_PWR_LVL_P3 (+3 dBm)", 9],
    ["ESP_PWR_LVL_P6 (+6 dBm)", 10],
    ["ESP_PWR_LVL_P9 (+9 dBm)", 11],
    ["ESP_PWR_LVL_P12 (+12 dBm)", 12],
    ["ESP_PWR_LVL_P15 (+15 dBm)", 13],
    ["ESP_PWR_LVL_P18 (+18 dBm)", 14],
    ["ESP_PWR_LVL_P20 (+20 dBm)", 15],
  ],
  "c6-gpio8": [
    ["ESP_PWR_LVL_N24 (-24 dBm)", 0],
    ["ESP_PWR_LVL_N21 (-21 dBm)", 1],
    ["ESP_PWR_LVL_N18 (-18 dBm)", 2],
    ["ESP_PWR_LVL_N15 (-15 dBm)", 3],
    ["ESP_PWR_LVL_N12 (-12 dBm)", 4],
    ["ESP_PWR_LVL_N9 (-9 dBm)", 5],
    ["ESP_PWR_LVL_N6 (-6 dBm)", 6],
    ["ESP_PWR_LVL_N3 (-3 dBm)", 7],
    ["ESP_PWR_LVL_N0 (0 dBm)", 8],
    ["ESP_PWR_LVL_P3 (+3 dBm)", 9],
    ["ESP_PWR_LVL_P6 (+6 dBm)", 10],
    ["ESP_PWR_LVL_P9 (+9 dBm)", 11],
    ["ESP_PWR_LVL_P12 (+12 dBm)", 12],
    ["ESP_PWR_LVL_P15 (+15 dBm)", 13],
    ["ESP_PWR_LVL_P18 (+18 dBm)", 14],
    ["ESP_PWR_LVL_P20 (+20 dBm)", 15],
  ],
  "c6-noled": [
    ["ESP_PWR_LVL_N24 (-24 dBm)", 0],
    ["ESP_PWR_LVL_N21 (-21 dBm)", 1],
    ["ESP_PWR_LVL_N18 (-18 dBm)", 2],
    ["ESP_PWR_LVL_N15 (-15 dBm)", 3],
    ["ESP_PWR_LVL_N12 (-12 dBm)", 4],
    ["ESP_PWR_LVL_N9 (-9 dBm)", 5],
    ["ESP_PWR_LVL_N6 (-6 dBm)", 6],
    ["ESP_PWR_LVL_N3 (-3 dBm)", 7],
    ["ESP_PWR_LVL_N0 (0 dBm)", 8],
    ["ESP_PWR_LVL_P3 (+3 dBm)", 9],
    ["ESP_PWR_LVL_P6 (+6 dBm)", 10],
    ["ESP_PWR_LVL_P9 (+9 dBm)", 11],
    ["ESP_PWR_LVL_P12 (+12 dBm)", 12],
    ["ESP_PWR_LVL_P15 (+15 dBm)", 13],
    ["ESP_PWR_LVL_P18 (+18 dBm)", 14],
    ["ESP_PWR_LVL_P20 (+20 dBm)", 15],
  ],
};

const TB_LED_PINS = new Set([0x22, 0x23, 0x24, 0x14, 0x15]);
const TB_BOARD_LED_PIN = 0x22;
const ADDRESS_TYPE_LABELS = {
  0: "Public",
  1: "Random",
  2: "Public identity",
  3: "Random identity",
};

const ADV_EVENT_LABELS = {
  0x00: "ADV_IND",
  0x01: "ADV_DIRECT_IND",
  0x02: "ADV_SCAN_IND",
  0x03: "ADV_NONCONN_IND",
  0x04: "SCAN_RSP",
};

let advTotal = 0;
let adv1m = 0;
let advCoded = 0;
let advRows = 0;
const ADV_MAX_ROWS = 500;

function logLine(level, text) {
  const area = $("log");
  area.value += `[${nowText()}] [${level.toUpperCase()}] ${text}\n`;
  area.scrollTop = area.scrollHeight;
}

function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function appendAdvRow(evt) {
  const tbody = $("advBody");
  const tr = document.createElement("tr");

  const phyPrimary = evt.phys & 0x0f;
  const phySecondary = (evt.phys >> 4) & 0x0f;
  const coded = phyPrimary === 3 || phySecondary === 3;
  const phyLabel = evt.phys === 0 ? "Legacy/1M" : (phySecondary ? `P:${phyName(phyPrimary)} S:${phyName(phySecondary)}` : phyName(phyPrimary));
  const eventLabel = advEventLabel(evt.eventType);
  const addressLabel = advAddressLabel(evt.addressType);

  advTotal += 1;
  if (coded) {
    advCoded += 1;
  } else {
    adv1m += 1;
  }

  const cells = [
    { text: new Date(evt.timestamp).toLocaleTimeString() },
    { text: String(evt.rssi) },
    { text: `0x${evt.eventType.toString(16).toUpperCase().padStart(2, "0")}`, title: eventLabel },
    { text: `0x${evt.addressType.toString(16).toUpperCase().padStart(2, "0")}`, title: addressLabel },
    { text: evt.phys === 0 ? "0x00" : `0x${evt.phys.toString(16).toUpperCase().padStart(2, "0")}`, title: phyLabel },
    evt.mac,
    String(evt.payloadLen),
    evt.payloadHex,
  ];
  for (const c of cells) {
    const td = document.createElement("td");
    if (typeof c === "string") {
      td.textContent = c;
    } else {
      td.textContent = c.text;
      if (c.title) {
        td.title = c.title;
      }
    }
    tr.appendChild(td);
  }
  tbody.appendChild(tr);
  advRows += 1;

  while (advRows > ADV_MAX_ROWS) {
    tbody.removeChild(tbody.firstChild);
    advRows -= 1;
  }

  $("statTotal").textContent = String(advTotal);
  $("stat1m").textContent = String(adv1m);
  $("statCoded").textContent = String(advCoded);
}

function phyName(value) {
  switch (value & 0x0f) {
    case 0x01: return "1M";
    case 0x02: return "2M";
    case 0x03: return "Coded";
    default: return `0x${(value & 0x0f).toString(16).toUpperCase()}`;
  }
}

function advAddressLabel(value) {
  const base = value & 0x0f;
  const direct = (value >> 4) & 0x0f;
  const label = ADDRESS_TYPE_LABELS[base] || `0x${base.toString(16).toUpperCase()}`;
  return direct ? `${label} / direct 0x${direct.toString(16).toUpperCase()}` : label;
}

function advEventLabel(value) {
  return ADV_EVENT_LABELS[value] || `0x${value.toString(16).toUpperCase()}`;
}

function bindApi() {
  api.on("open", () => {
    logLine("info", "Serial connected");
    $("connectBtn").disabled = true;
    $("disconnectBtn").disabled = false;
  });

  api.on("close", () => {
    logLine("info", "Serial disconnected");
    $("connectBtn").disabled = false;
    $("disconnectBtn").disabled = true;
  });

  api.on("error", ({ error }) => {
    logLine("error", error?.message || String(error));
  });

  api.on("log", ({ level, message }) => {
    logLine(level, message);
  });

  api.on("tx", ({ commandName, payloadHex }) => {
    logLine("debug", `TX ${commandName} ${payloadHex}`);
  });

  api.on("rx_raw", ({ len, hex }) => {
    if ($("crcDebug").checked) {
      logLine("debug", `RX ${len}B ${hex}`);
    }
  });

  api.on("response", (evt) => {
    let line = `RESP ${evt.commandName} status=${evt.statusName} data=${bytesToHex(evt.data)}`;
    if (evt.command === CMD.INFO && evt.info) {
      if (evt.info.localMac) {
        $("rfLocalMac").value = evt.info.localMac;
      }
      $("infoOut").value = `${evt.info.statusName} mac=${evt.info.localMac || "n/a"}`;
      line = `INFO status=${evt.info.statusName} fw=0x${(evt.info.fwVersion & 0xff).toString(16).toUpperCase().padStart(2, "0")} mac=${evt.info.localMac || "n/a"}`;
    }
    if (evt.command === CMD.VERSION) {
      $("versionOut").value = `${evt.statusName} ver=0x${(evt.status & 0xff).toString(16).toUpperCase().padStart(2, "0")} data=${bytesToHex(evt.data)}`;
    }
    if (evt.command === CMD.VBAT && evt.vbat) {
      $("vbatOut").value = `${evt.vbat.statusName} mv=${evt.vbat.mv ?? "n/a"} temp=${evt.vbat.tempC ?? "n/a"}`;
      line = `VBAT status=${evt.vbat.statusName} mv=${evt.vbat.mv ?? "n/a"} temp=${evt.vbat.tempC ?? "n/a"}`;
    }
    if (evt.command === CMD.GPIOEVT && evt.gpioevt) {
      if (evt.gpioevt.mode === "event") {
        line = `GPIOEVT event GPIO${evt.gpioevt.pin} level=${evt.gpioevt.level} t=${evt.gpioevt.tsMs}ms`;
      } else if (evt.gpioevt.mode === "query") {
        line = `GPIOEVT query status=${evt.gpioevt.statusName} mask=0x${(evt.gpioevt.mask >>> 0).toString(16).toUpperCase().padStart(8, "0")}`;
      }
    }
    logLine("info", line);
  });

  api.on("adv", (evt) => {
    appendAdvRow(evt);
  });

  api.on("crc_error", (evt) => {
    logLine("warn", `CRC discard 0x${evt.discard.toString(16).toUpperCase().padStart(2, "0")}`);
  });
}

function bindUi() {
  const setActiveTab = (tabName) => {
    const tabBtns = document.querySelectorAll("#leftTabs .tabBtn");
    const tabPanels = document.querySelectorAll(".tabPanel");
    for (const btn of tabBtns) {
      btn.classList.toggle("active", btn.dataset.tab === tabName);
    }
    for (const panel of tabPanels) {
      panel.classList.toggle("active", panel.dataset.panel === tabName);
    }
  };

  const tabBtns = document.querySelectorAll("#leftTabs .tabBtn");
  for (const btn of tabBtns) {
    btn.addEventListener("click", () => {
      setActiveTab(btn.dataset.tab || "serial");
    });
  }
  setActiveTab("serial");

  const setActiveRightTab = (tabName) => {
    const tabBtns = document.querySelectorAll("#rightTabs .rightTabBtn");
    const tabPanels = document.querySelectorAll(".rightTabPanel");
    for (const btn of tabBtns) {
      btn.classList.toggle("active", btn.dataset.righttab === tabName);
    }
    for (const panel of tabPanels) {
      panel.classList.toggle("active", panel.dataset.rightpanel === tabName);
    }
  };

  const rightTabBtns = document.querySelectorAll("#rightTabs .rightTabBtn");
  for (const btn of rightTabBtns) {
    btn.addEventListener("click", () => {
      setActiveRightTab(btn.dataset.righttab || "adv");
    });
  }
  setActiveRightTab("adv");

  const deviceProfile = $("deviceProfile");
  const pulseReset = $("pulseReset");
  const tbLedSection = $("tbLedSection");
  const rgbSection = $("rgbSection");

  const fillRfPowerOptions = () => {
    const list = RF_POWER_OPTIONS[deviceProfile.value] || RF_POWER_OPTIONS["esp32-c3"];
    const select = $("rfPower");
    select.innerHTML = "";
    for (const [label, value] of list) {
      const opt = document.createElement("option");
      opt.value = String(value);
      opt.textContent = label;
      select.appendChild(opt);
    }
  };

  const applyProfileUi = () => {
    const isTb = deviceProfile.value === "tb-03f-kit";
    const hasRgbChip = deviceProfile.value.startsWith("esp32-c6") ||
                       deviceProfile.value === "esp32-h2" ||
                       deviceProfile.value === "esp32-s3" ||
                       runtimeRgbGpio !== null;
    if (tbLedSection) {
      tbLedSection.style.display = isTb ? "block" : "none";
    }
    if (rgbSection) {
      rgbSection.style.display = hasRgbChip ? "block" : "none";
    }

    $("txadvStartBtn").disabled = isTb;
    $("txadvStopBtn").disabled = isTb;
    $("txadvStatusBtn").disabled = isTb;
    $("connOpenBtn").disabled = isTb;
    $("connStatusBtn").disabled = isTb;
    $("connDiscBtn").disabled = isTb;
    $("connCancelBtn").disabled = isTb;
    $("gpioAnalogBtn").disabled = isTb;
    $("evtEnableBtn").disabled = isTb;
    $("evtDisableBtn").disabled = isTb;
    $("evtQueryBtn").disabled = isTb;
    $("evtClearBtn").disabled = isTb;

    fillRfPowerOptions();
  };

  deviceProfile.addEventListener("change", () => {
    if (deviceProfile.value === "tb-03f-kit") {
      pulseReset.checked = true;
      logLine("info", "Device profile: TB-03F-KIT");
    } else {
      pulseReset.checked = false;
      logLine("info", `Device profile: ${deviceProfile.value}`);
    }
    populateRgbGpioSelect();
    renderGpioStatusGrid();
    applyProfileUi();
  });

  populateRgbGpioSelect();
  applyProfileUi();

  // ---- GPIO pin ranges per chip ---------------------------------------
  const GPIO_MAX = {
    "esp32-c3": 21,
    "esp32-c6": 23,
    "esp32-h2": 27,
    "esp32-s3": 48,
    "tb-03f-kit": 0x24,
  };
  const gpioPinsForProfile = (profId) => {
    const max = GPIO_MAX[profId] || 21;
    const pins = [];
    for (let p = 0; p <= max; p++) pins.push(p);
    return pins;
  };
  const pinLabel = (pin) => {
    if (hasRgbLed() && pin === rgbLedPin()) return `GPIO${pin}(RGB)`;
    if (boardLedPin() === pin) return `GPIO${pin}(LED)`;
    if (pin === 9) return `GPIO${pin}(BOOT)`;
    return `GPIO${pin}`;
  };

  let gpioStatusData = {};

  const renderGpioStatusGrid = () => {
    const grid = $("gpioStatusGrid");
    grid.innerHTML = "";
    const pins = gpioPinsForProfile(deviceProfile.value);
    for (const pin of pins) {
      const d = gpioStatusData[pin] || {};
      const div = document.createElement("div");
      div.style.border = "1px solid var(--line)";
      div.style.borderRadius = "6px";
      div.style.padding = "4px 6px";
      div.style.fontSize = "11px";
      div.innerHTML = `<b>${pinLabel(pin)}</b><br>` +
        `level: <span style="font-weight:600;color:${d.level === 1 ? '#22863a' : '#888'}">${d.level !== undefined ? (d.level ? 'HIGH' : 'low') : '-'}</span>` +
        (d.analog !== undefined ? ` | ADC: ${d.analog}` : "") +
        (d.led !== undefined ? ` | LED:${d.led ? 'Y' : 'N'}` : "");
      grid.appendChild(div);
    }
  };

  renderGpioStatusGrid();

  $("connectBtn").addEventListener("click", async () => {
    try {
      const baudRate = Number($("baud").value || 2000000);
      const crcDebug = $("crcDebug").checked;
      api.options.crcDebug = crcDebug;
      await api.open({ baudRate, pulseReset: pulseReset.checked });
    } catch (err) {
      const msg = err.message || String(err);
      if (msg.includes("No port selected by the user")) {
        logLine("warn", msg);
      } else {
        logLine("error", msg);
      }
    }
  });

  $("disconnectBtn").addEventListener("click", async () => {
    try {
      try {
        await Promise.race([
          api.scanStop(),
          new Promise((_, reject) => {
            setTimeout(() => reject(new Error("stop scan timeout")), 1200);
          }),
        ]);
        logLine("info", "SCAN stop requested before disconnect");
      } catch (err) {
        logLine("warn", `SCAN stop before disconnect failed: ${err.message || String(err)}`);
      }
      await api.close();
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });

  $("scanStartBtn").addEventListener("click", async () => {
    try {
      const config = {
        phy1m: $("phy").value === "1m" || $("phy").value === "both",
        phyCoded: $("phy").value === "coded" || $("phy").value === "both",
        filterRandomAddresses: $("filterRandom").checked,
        filterPrivateAddresses: $("filterPrivate").checked,
        windowMs: Number($("scanWindowMs").value || 30),
      };
      await api.scanStart(config);
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });

  $("scanStopBtn").addEventListener("click", async () => {
    try {
      await api.scanStop();
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });

  $("clearAdvBtn").addEventListener("click", () => {
    $("advBody").innerHTML = "";
    advRows = 0;
    advTotal = 0;
    adv1m = 0;
    advCoded = 0;
    $("statTotal").textContent = "0";
    $("stat1m").textContent = "0";
    $("statCoded").textContent = "0";
  });

  $("applyMacBtn").addEventListener("click", async () => {
    try {
      const wl = splitMacList($("whitelist").value);
      const bl = splitMacList($("blacklist").value);
      await api.clearMacList();
      for (const mac of wl) {
        await api.addWhitelist(mac);
      }
      for (const mac of bl) {
        await api.addBlacklist(mac);
      }
      logLine("info", `MAC filters applied: whitelist=${wl.length} blacklist=${bl.length}`);
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });

  $("infoBtn").addEventListener("click", async () => {
    try { await api.info(); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("versionBtn").addEventListener("click", async () => {
    try { await api.version(); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("vbatBtn").addEventListener("click", async () => {
    try { await api.readVbat(); } catch (err) { logLine("error", err.message || String(err)); }
  });

  $("gpioReadBtn").addEventListener("click", async () => {
    try { await api.gpioRead(parsePinField($("gpioPin").value)); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("gpioWriteBtn").addEventListener("click", async () => {
    try { await api.gpioWrite(parsePinField($("gpioPin").value), Number($("gpioLevel").value || 0)); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("gpioToggleBtn").addEventListener("click", async () => {
    try { await api.gpioToggle(parsePinField($("gpioPin").value)); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("gpioConfigBtn").addEventListener("click", async () => {
    try {
      await api.gpioConfig(
        parsePinField($("gpioPin").value),
        $("gpioIn").checked,
        $("gpioOut").checked,
        Number($("gpioPull").value || 0),
      );
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });
  $("gpioAnalogBtn").addEventListener("click", async () => {
    try { await api.gpioAnalogRead(parsePinField($("gpioPin").value)); } catch (err) { logLine("error", err.message || String(err)); }
  });

  $("evtQueryBtn").addEventListener("click", async () => {
    try { await api.gpioEventQuery(); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("evtEnableBtn").addEventListener("click", async () => {
    try { await api.gpioEventEnable(parsePinField($("evtPin").value)); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("evtDisableBtn").addEventListener("click", async () => {
    try { await api.gpioEventDisable(parsePinField($("evtPin").value)); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("evtClearBtn").addEventListener("click", async () => {
    try { await api.gpioEventClear(); } catch (err) { logLine("error", err.message || String(err)); }
  });

  // ---- RGB LED controls ------------------------------------------------
  $("rgbPicker").addEventListener("input", () => {
    const c = $("rgbPicker").value;
    $("rgbR").value = parseInt(c.slice(1,3), 16);
    $("rgbG").value = parseInt(c.slice(3,5), 16);
    $("rgbB").value = parseInt(c.slice(5,7), 16);
  });
  $("rgbBrightness").addEventListener("input", () => {
    $("rgbBrightnessVal").textContent = $("rgbBrightness").value + "%";
  });
  $("rgbSetBtn").addEventListener("click", async () => {
    try {
      const bright = Math.max(1, Math.min(100, Number($("rgbBrightness").value || 100))) / 100;
      const r = Math.round(Number($("rgbR").value || 0) * bright);
      const g = Math.round(Number($("rgbG").value || 0) * bright);
      const b = Math.round(Number($("rgbB").value || 0) * bright);
      await api.gpioRgb(rgbLedPin(), r, g, b);
      logLine("info", `RGB set to (${r},${g},${b}) brightness ${$("rgbBrightness").value}%`);
    } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("rgbOffBtn").addEventListener("click", async () => {
    try {
      await api.gpioRgb(rgbLedPin(), 0, 0, 0);
      logLine("info", "RGB LED off");
    } catch (err) { logLine("error", err.message || String(err)); }
  });

  // ---- Runtime LED/RMT overrides (from URL query params) --------------
  const urlParams = new URLSearchParams(window.location.search);
  const urlLedGpio = urlParams.get("led_gpio");
  const urlRgbGpio = urlParams.get("rgb_gpio");
  const urlLedActiveLow = urlParams.get("led_active_low");

  const runtimeLedGpio = urlLedGpio !== null ? parseInt(urlLedGpio, 10) : null;
  const runtimeRgbGpio = urlRgbGpio !== null ? parseInt(urlRgbGpio, 10) : null;
  const runtimeLedActiveLow = urlLedActiveLow !== null ? (parseInt(urlLedActiveLow, 10) !== 0) : null;

  // ---- Board LED controls ----------------------------------------------
  const boardLedPin = () => {
    if (runtimeLedGpio !== null) return runtimeLedGpio;
    if (deviceProfile.value === "tb-03f-kit") return 0x22; // blue PC2
    if (deviceProfile.value.startsWith("esp32-c6-gpio")) return 0x0F; // GPIO15
    return 0x08; // ESP32-C3 GPIO8
  };
  const boardLedActiveLow = () => {
    if (runtimeLedActiveLow !== null) return runtimeLedActiveLow;
    return deviceProfile.value === "esp32-c3";
  };
  // Populate RGB GPIO selector when profile changes
  const populateRgbGpioSelect = () => {
    const sel = $("rgbGpioSelect");
    if (!sel) return;
    sel.innerHTML = "";
    const pins = gpioPinsForProfile(deviceProfile.value);
    for (const p of pins) {
      const opt = document.createElement("option");
      opt.value = String(p);
      opt.textContent = `GPIO${p}`;
      sel.appendChild(opt);
    }
    const cur = runtimeRgbGpio !== null ? runtimeRgbGpio : (pins.includes(15) ? 15 : 8);
    sel.value = String(cur);
  };

  const rgbLedPin = () => {
    if (runtimeRgbGpio !== null) return runtimeRgbGpio;
    const sel = $("rgbGpioSelect");
    if (sel && sel.value) return parseInt(sel.value, 10);
    return 0x0F; // fallback
  };
  const hasRgbLed = () => {
    if (runtimeRgbGpio !== null) return true;
    return deviceProfile.value.startsWith("esp32-c6-gpio");
  };
  const writeLed = async (pin, on) => {
    const level = boardLedActiveLow() ? (on ? 0 : 1) : (on ? 1 : 0);
    if (hasRgbLed() && pin === parseInt($("rgbGpioSelect")?.value || "15", 10)) {
      await api.gpioRgb(pin, on ? 32 : 0, on ? 32 : 0, on ? 32 : 0);
    } else {
      await api.gpioWrite(pin, level);
    }
  };
  $("ledOnBtn").addEventListener("click", async () => {
    try { await writeLed(boardLedPin(), true); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("ledOffBtn").addEventListener("click", async () => {
    try { await writeLed(boardLedPin(), false); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("ledToggleBtn").addEventListener("click", async () => {
    try {
      const pin = boardLedPin();
      if (hasRgbLed()) {
        const resp = await api.gpioRead(pin);
        const level = resp?.data?.[2];
        await writeLed(pin, level === 1 ? false : true);
      } else {
        await api.gpioToggle(pin);
      }
    } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("ledAllOffBtn").addEventListener("click", async () => {
    try {
      const pin = boardLedPin();
      if (deviceProfile.value.startsWith("esp32-c6-gpio")) {
        await api.gpioRgb(pin, 0, 0, 0);
      } else if (deviceProfile.value === "tb-03f-kit") {
        const tbPins = [0x22, 0x23, 0x24, 0x14, 0x15];
        for (const p of tbPins) { await api.gpioWrite(p, 0); }
      } else {
        await api.gpioWrite(pin, boardLedActiveLow() ? 1 : 0);
      }
      logLine("info", "All LEDs turned off");
    } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("ledQueryBtn").addEventListener("click", async () => {
    try {
      const pin = boardLedPin();
      const resp = await api.gpioRead(pin);
      const level = resp?.data?.[2];
      const on = boardLedActiveLow() ? (level === 0) : (level === 1);
      $("ledStatus").textContent = on ? "ON" : "OFF";
      logLine("info", `Board LED pin=${pin} level=${level} => ${on ? "ON" : "OFF"}`);
    } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("ledBlinkBtn").addEventListener("click", async () => {
    try {
      const pin = boardLedPin();
      const count = Math.max(1, Math.min(10, Number($("ledBlinkCount").value || 3)));
      const delayMs = Math.max(10, Math.min(500, Number($("ledBlinkDelay").value || 80)));
      for (let i = 0; i < count; i++) {
        await writeLed(pin, true);
        await new Promise(r => setTimeout(r, delayMs));
        await writeLed(pin, false);
        await new Promise(r => setTimeout(r, delayMs));
      }
    } catch (err) { logLine("error", err.message || String(err)); }
  });

  // ---- PWM controls ----------------------------------------------------
  $("pwmStartBtn").addEventListener("click", async () => {
    try {
      const pin = parsePinField($("pwmPin").value);
      const duty = Math.max(0, Math.min(100, Number($("pwmDuty").value || 50)));
      const periodUs = Math.max(1, Number($("pwmPeriod").value || 200));
      await api.gpioPwm(pin, duty, periodUs);
      logLine("info", `PWM start pin=${pin} duty=${duty}% period=${periodUs}µs`);
    } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("pwmOffBtn").addEventListener("click", async () => {
    try {
      const pin = parsePinField($("pwmPin").value);
      await api.gpioPwmOff(pin);
      logLine("info", `PWM off pin=${pin}`);
    } catch (err) { logLine("error", err.message || String(err)); }
  });

  $("gpioReadAllBtn").addEventListener("click", async () => {
    try {
      const isC6 = deviceProfile.value.startsWith("esp32-c6");
      const pins = isC6 ? [0,1,2,3,4,5,6,7,8,9,10,12,13,15,18,19,20,21,22,23,25,26,27] : ESP32_PINS;
      let lastMask = 0;
      for (const pin of pins) {
        try {
          const resp = await api.gpioRead(pin);
          if (resp?.data && resp.data.length >= 6) {
            gpioStatusData[pin] = { level: resp.data[2], led: resp.data[3] };
            lastMask = (resp.data[5] << 8) | resp.data[4];
          }
        } catch (_) { /* skip pins that error */ }
        await new Promise(r => setTimeout(r, 5));
      }
      renderGpioStatusGrid();
      if (lastMask) $("boardMask").textContent = "0x" + lastMask.toString(16).padStart(4, "0");
      logLine("info", `Read ${pins.length} GPIO pins`);
    } catch (err) { logLine("error", err.message || String(err)); }
  });

  // Device profile change: show/hide RGB and PWM sections

  $("txadvStartBtn").addEventListener("click", async () => {
    try {
      await api.txAdvStart({
        phy: Number($("txadvPhy").value || 0),
        intervalMs: Number($("txadvInterval").value || 100),
        payloadHex: $("txadvPayload").value,
      });
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });
  $("txadvStopBtn").addEventListener("click", async () => {
    try { await api.txAdvStop(); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("txadvStatusBtn").addEventListener("click", async () => {
    try { await api.txAdvStatus(); } catch (err) { logLine("error", err.message || String(err)); }
  });

  $("connStatusBtn").addEventListener("click", async () => {
    try { await api.connStatus(); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("connOpenBtn").addEventListener("click", async () => {
    try {
      await api.connOpen({
        coded: $("connPhy").value === "coded",
        addrType: Number($("connAddrType").value || 0),
        mac: $("connMac").value,
      });
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });
  $("connDiscBtn").addEventListener("click", async () => {
    try { await api.connDisconnect(); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("connCancelBtn").addEventListener("click", async () => {
    try { await api.connCancel(); } catch (err) { logLine("error", err.message || String(err)); }
  });

  // TB-03F-KIT direct LED controls (per LED: on/off/toggle/blink)
  const ensureLedOutput = async (pin) => {
    await api.gpioConfig(pin, false, true, 0);
  };

  const setLedState = async (pin, enabled) => {
    await ensureLedOutput(pin);
    await api.gpioWrite(pin, enabled ? 1 : 0);
  };

  const toggleLedState = async (pin) => {
    if (pin === TB_BOARD_LED_PIN) {
      const resp = await api.gpioRead(pin);
      const level = resp?.gpio?.level;
      if (level === 0 || level === 1) {
        await api.gpioWrite(pin, level ? 0 : 1);
        return;
      }
    }
    await api.gpioToggle(pin);
  };

  const blinkLed = async (pin) => {
    for (let i = 0; i < 3; i += 1) {
      await setLedState(pin, true);
      await delay(80);
      await setLedState(pin, false);
      await delay(80);
    }
  };

  if (tbLedSection) {
    tbLedSection.addEventListener("click", async (event) => {
      const btn = event.target.closest("button[data-led-pin][data-led-act]");
      if (!btn) {
        return;
      }
      const pin = Number(btn.dataset.ledPin || 0) & 0xff;
      const act = String(btn.dataset.ledAct || "");
      if (!TB_LED_PINS.has(pin) || deviceProfile.value !== "tb-03f-kit") {
        return;
      }
      try {
        if (act === "on") {
          await setLedState(pin, true);
        } else if (act === "off") {
          await setLedState(pin, false);
        } else if (act === "toggle") {
          await toggleLedState(pin);
        } else if (act === "blink") {
          await blinkLed(pin);
        }
      } catch (err) {
        logLine("error", err.message || String(err));
      }
    });
  }

  // RF tab
  $("rfStatusBtn").addEventListener("click", async () => {
    try {
      await api.info();
      await api.rfStatus();
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });
  $("rfPowerApplyBtn").addEventListener("click", async () => {
    try { await api.rfSetPower(Number($("rfPower").value || 0)); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("rfCapApplyBtn").addEventListener("click", async () => {
    try {
      const cap = Math.max(0, Math.min(63, Number($("rfCap").value || 0)));
      await api.rfSetCap(cap);
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });
  $("rfChApplyBtn").addEventListener("click", async () => {
    try {
      const ch0 = Number($("rfCh0").value || 37);
      const ch1 = Number($("rfCh1").value || 38);
      const ch2 = Number($("rfCh2").value || 39);
      await api.rfSetChannels(ch0, ch1, ch2);
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });
  $("rfCodedApplyBtn").addEventListener("click", async () => {
    try {
      const minMs = Math.max(10, Number($("rfCodedMinMs").value || 30));
      const units = Math.max(1, Math.min(200, Math.round(minMs / 10)));
      await api.rfSetCodedMinWindow(units);
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });

  // Console tab
  $("uartStatusBtn").addEventListener("click", async () => {
    try { await api.uartStatus(); } catch (err) { logLine("error", err.message || String(err)); }
  });
  $("uartPingBtn").addEventListener("click", async () => {
    try {
      const seq = Number($("uartPingSeq").value || 0) & 0xffff;
      await api.uartPing(seq);
      $("uartPingSeq").value = String((seq + 1) & 0xffff);
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });
  $("uartSetBaudBtn").addEventListener("click", async () => {
    try {
      await api.uartSetBaud(Number($("uartBaudIndex").value || 0));
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });
  $("txdataSendBtn").addEventListener("click", async () => {
    try {
      const handle = parseInt(String($("txdataHandle").value || "0012"), 16);
      await api.txData(handle, $("txdataPayload").value);
    } catch (err) {
      logLine("error", err.message || String(err));
    }
  });
}

bindApi();
bindUi();

logLine("info", "Web GUI ready. Click Connect and select the serial device.");
