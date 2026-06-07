#!/usr/bin/env node

(async () => {
  const {
    Adv2UartApi,
    CMD,
    normalizeMac,
    commandStatusName,
    bytesToHex,
  } = await import("./adv2uart_api.mjs");

  const LEVEL = {
    error: 0,
    warn: 1,
    info: 2,
    verbose: 3,
    debug: 4,
  };

  function logger(levelName, minLevel) {
    const level = LEVEL[levelName] ?? LEVEL.info;
    return {
      error: (...args) => { if (level >= LEVEL.error) console.error("[ERR]", ...args); },
      warn: (...args) => { if (level >= LEVEL.warn) console.log("[WRN]", ...args); },
      info: (...args) => { if (level >= LEVEL.info) console.log("[INF]", ...args); },
      verbose: (...args) => { if (level >= LEVEL.verbose) console.log("[VRB]", ...args); },
      debug: (...args) => { if (level >= LEVEL.debug) console.log("[DBG]", ...args); },
      minLevel,
    };
  }

  function sleep(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
  }

  function parseArgv(argv) {
    const out = {
      debug: false,
      verbose: false,
      info: false,
      sleep: 1,
      baudrate: 2000000,
      serialPort: process.platform === "win32" ? "COM11" : "/dev/ttyUSB0",
      timeout: 0.3,
      number: 0,
      duration: 0,
      idleTimeout: 0,
      statusInterval: 10,
      phy: "both",
      scanWindowMs: 30,
      filterRandom: false,
      filterPrivate: false,
      whitelist: [],
      blacklist: [],
      infoAfter: 0,
      battery: false,
      gpioEventPins: [],
      gpioEventOnly: false,
      crcDebug: false,
      help: false,
    };

    function takeValue(i, key) {
      if (i + 1 >= argv.length) {
        throw new Error(`Missing value for ${key}`);
      }
      return argv[i + 1];
    }

    for (let i = 0; i < argv.length; i += 1) {
      const a = argv[i];
      if (a === "-h" || a === "--help") {
        out.help = true;
      } else if (a === "-d" || a === "--debug") {
        out.debug = true;
      } else if (a === "-v" || a === "--verbose") {
        out.verbose = true;
      } else if (a === "-i" || a === "--info") {
        out.info = true;
      } else if (a === "-s" || a === "--sleep") {
        out.sleep = Number(takeValue(i, a)); i += 1;
      } else if (a === "-b" || a === "--baudrate") {
        out.baudrate = Number(takeValue(i, a)); i += 1;
      } else if (a === "-p" || a === "--port") {
        out.serialPort = takeValue(i, a); i += 1;
      } else if (a === "-t" || a === "--timeout") {
        out.timeout = Number(takeValue(i, a)); i += 1;
      } else if (a === "-n" || a === "--number") {
        out.number = Number(takeValue(i, a)); i += 1;
      } else if (a === "--duration") {
        out.duration = Number(takeValue(i, a)); i += 1;
      } else if (a === "--idle-timeout") {
        out.idleTimeout = Number(takeValue(i, a)); i += 1;
      } else if (a === "--status-interval") {
        out.statusInterval = Number(takeValue(i, a)); i += 1;
      } else if (a === "--phy") {
        out.phy = String(takeValue(i, a)).toLowerCase(); i += 1;
      } else if (a === "--scan-window-ms") {
        out.scanWindowMs = Number(takeValue(i, a)); i += 1;
      } else if (a === "--filter-random") {
        out.filterRandom = true;
      } else if (a === "--accept-random") {
        out.filterRandom = false;
      } else if (a === "--filter-private") {
        out.filterPrivate = true;
      } else if (a === "--accept-private") {
        out.filterPrivate = false;
      } else if (a === "--whitelist") {
        out.whitelist.push(takeValue(i, a)); i += 1;
      } else if (a === "--blacklist" || a === "--backlist") {
        out.blacklist.push(takeValue(i, a)); i += 1;
      } else if (a === "--info-after") {
        out.infoAfter = Number(takeValue(i, a)); i += 1;
      } else if (a === "--battery") {
        out.battery = true;
      } else if (a === "--gpio-event") {
        out.gpioEventPins.push(Number(takeValue(i, a))); i += 1;
      } else if (a === "--gpio-event-only") {
        out.gpioEventOnly = true;
      } else if (a === "--crc-debug") {
        out.crcDebug = true;
      } else {
        throw new Error(`Unknown option: ${a}`);
      }
    }

    if (!["1m", "coded", "both"].includes(out.phy)) {
      throw new Error("--phy must be one of: 1m, coded, both");
    }
    return out;
  }

  function parseMacFilters(items) {
    const out = [];
    for (const raw of items) {
      for (const piece of String(raw).split(/[;,]/)) {
        const p = piece.trim();
        if (!p) continue;
        out.push(normalizeMac(p));
      }
    }
    return out;
  }

  function printHelp() {
    console.log(`Usage: node adv2uart.js [options]

Options:
  -d, --debug                 debug logs
  -v, --verbose               verbose logs
  -i, --info                  info logs
  -s, --sleep <sec>           initial delay (default 1)
  -b, --baudrate <baud>       baudrate (default 2000000)
  -p, --port <path>           serial port
  -t, --timeout <sec>         read timeout hint (default 0.3)
  -n, --number <count>        stop after N advertisements
  --duration <sec>            stop after N seconds
  --idle-timeout <sec>        stop after N seconds without advertisements
  --status-interval <sec>     print idle status every N seconds
  --phy <1m|coded|both>       scan PHY selection
  --scan-window-ms <ms>       scan window in milliseconds
  --filter-random             filter random addresses
  --accept-random             accept random addresses
  --filter-private            filter private addresses
  --accept-private            accept private addresses
  --whitelist <list>          MAC/prefix list (comma separated, repeatable)
  --blacklist <list>          MAC/prefix list (comma separated, repeatable)
  --info-after <sec>          send INFO during scan after N seconds
  --battery                   query VBAT then exit
  --gpio-event <pin>          arm GPIO edge event for pin (repeatable)
  --gpio-event-only           event loop only, no scan start
  --crc-debug                 verbose CRC discard logs
  -h, --help                  show this help
`);
  }

  class NodeSerialTransport {
    constructor() {
      this.path = null;
      this.port = null;
      this.onData = () => {};
      this.onError = () => {};
      this.SerialPort = null;
    }

    setDataHandler(fn) {
      this.onData = fn || (() => {});
    }

    setErrorHandler(fn) {
      this.onError = fn || (() => {});
    }

    async _loadSerialPort() {
      if (this.SerialPort) {
        return this.SerialPort;
      }
      let mod;
      try {
        mod = await import("serialport");
      } catch (err) {
        throw new Error("Missing dependency 'serialport'. Install with: npm i serialport");
      }
      this.SerialPort = mod.SerialPort || mod.default?.SerialPort || mod.default;
      if (!this.SerialPort) {
        throw new Error("Cannot resolve SerialPort class from 'serialport' package");
      }
      return this.SerialPort;
    }

    async open(config) {
      const SerialPort = await this._loadSerialPort();
      const { path, baudRate = 2000000, pulseReset = true } = config;
      this.path = path;
      this.port = new SerialPort({ path, baudRate, autoOpen: false });

      await new Promise((resolve, reject) => {
        this.port.open((err) => err ? reject(err) : resolve());
      });

      this.port.on("data", (chunk) => {
        const data = chunk instanceof Uint8Array ? chunk : new Uint8Array(chunk);
        this.onData(data);
      });
      this.port.on("error", (err) => this.onError(err));

      if (pulseReset) {
        await new Promise((resolve) => this.port.set({ dtr: true, rts: true }, () => resolve()));
        await sleep(50);
        await new Promise((resolve) => this.port.set({ dtr: false, rts: false }, () => resolve()));
      }
    }

    async write(frame) {
      if (!this.port) {
        throw new Error("Port not open");
      }
      await new Promise((resolve, reject) => {
        this.port.write(Buffer.from(frame), (err) => err ? reject(err) : resolve());
      });
      await new Promise((resolve, reject) => {
        this.port.drain((err) => err ? reject(err) : resolve());
      });
    }

    async close() {
      if (!this.port) {
        return;
      }
      const p = this.port;
      this.port = null;
      await new Promise((resolve) => {
        p.close(() => resolve());
      });
    }
  }

  let opts;
  try {
    opts = parseArgv(process.argv.slice(2));
  } catch (err) {
    console.error(err.message);
    printHelp();
    process.exit(2);
  }

  if (opts.help) {
    printHelp();
    process.exit(0);
  }

  const levelName = opts.debug ? "debug" : opts.verbose ? "verbose" : opts.info ? "info" : "info";
  const log = logger(levelName, LEVEL[levelName]);

  const whiteList = parseMacFilters(opts.whitelist);
  const blackList = parseMacFilters(opts.blacklist);

  const transport = new NodeSerialTransport();
  const api = new Adv2UartApi(transport, { crcDebug: opts.crcDebug });

  let scanEnabled = false;
  let advCount = 0;
  let lastAdvAt = Date.now();
  let startedAt = 0;
  let infoSentAt = 0;
  let infoAcked = false;

  api.on("log", ({ level, message }) => {
    if (level === "warn") log.warn(message);
    else if (level === "debug") log.debug(message);
    else log.info(message);
  });

  api.on("tx", ({ payloadHex, commandName }) => {
    log.debug(`TX ${commandName} ${payloadHex}`);
  });

  api.on("response", (evt) => {
    if (evt.command === CMD.SCAN) {
      scanEnabled = evt.data.length >= 1 && evt.data[0] !== 0;
      log.warn(`SCAN status=${evt.statusName} running=${scanEnabled ? "yes" : "no"}`);
      return;
    }
    if (evt.command === CMD.INFO) {
      infoAcked = true;
      const version = evt.data.length >= 1 ? `0x${evt.data[0].toString(16).toUpperCase().padStart(2, "0")}` : "?";
      const mac = evt.data.length >= 7 ? bytesToHex(Uint8Array.from(evt.data.slice(1, 7)).reverse()) : "?";
      log.warn(`INFO status=${evt.statusName} fw=${version} mac=${mac}`);
      return;
    }
    if (evt.command === CMD.VBAT) {
      if (evt.vbat) {
        const t = evt.vbat.tempC === null ? "n/a" : `${evt.vbat.tempC} C`;
        log.warn(`VBAT status=${evt.vbat.statusName} mv=${evt.vbat.mv ?? "n/a"} temp=${t}`);
      } else {
        log.warn(`VBAT status=${evt.statusName}`);
      }
      return;
    }
    if (evt.command === CMD.GPIOEVT && evt.gpioevt) {
      if (evt.gpioevt.mode === "event") {
        log.warn(`GPIOEVT event GPIO${evt.gpioevt.pin} level=${evt.gpioevt.level} t=${evt.gpioevt.tsMs}ms`);
      } else if (evt.gpioevt.mode === "query") {
        log.warn(`GPIOEVT query status=${evt.gpioevt.statusName} mask=0x${(evt.gpioevt.mask >>> 0).toString(16).toUpperCase().padStart(8, "0")}`);
      } else {
        const m = typeof evt.gpioevt.mask === "number"
          ? ` mask=0x${(evt.gpioevt.mask >>> 0).toString(16).toUpperCase().padStart(8, "0")}`
          : "";
        log.warn(`GPIOEVT ack status=${evt.gpioevt.statusName}${m}`);
      }
      return;
    }

    log.info(`RESP cmd=${evt.commandName} status=${evt.statusName} data=${bytesToHex(evt.data)}`);
  });

  api.on("adv", (evt) => {
    advCount += 1;
    lastAdvAt = Date.now();
    const now = new Date(evt.timestamp).toISOString().slice(11, 19);
    const addrType = `0x${evt.addressType.toString(16).toUpperCase().padStart(2, "0")}`;
    const eventType = `0x${evt.eventType.toString(16).toUpperCase().padStart(2, "0")}`;
    const phyLabel = evt.phys === 0 ? "legacy" : `0x${evt.phys.toString(16).toUpperCase().padStart(2, "0")}`;
    log.info(`${now} RSSI=${String(evt.rssi).padStart(4, " ")} evt=${eventType} addr=${addrType} phy=${phyLabel} mac=${evt.mac} len=${String(evt.payloadLen).padStart(3, " ")} payload=${evt.payloadHex}`);
  });

  log.warn(`Connecting to ${opts.serialPort} @ ${opts.baudrate}...`);

  let scanStarted = false;
  try {
    await api.open({
      path: opts.serialPort,
      baudRate: opts.baudrate,
      timeout: opts.timeout,
      pulseReset: true,
    });

    await sleep(Math.max(0, opts.sleep * 1000));

    for (const pin of opts.gpioEventPins) {
      log.warn(`Arming GPIOEVT on GPIO${pin}`);
      await api.gpioEventEnable(pin);
      await sleep(50);
    }

    if (opts.gpioEventOnly) {
      if (opts.gpioEventPins.length === 0) {
        throw new Error("--gpio-event-only requires at least one --gpio-event <pin>");
      }
      log.warn("GPIO event only mode. Press Ctrl+C to stop.");
      const endAt = opts.duration > 0 ? Date.now() + opts.duration * 1000 : 0;
      while (!endAt || Date.now() < endAt) {
        await sleep(100);
      }
      process.exit(0);
    }

    if (opts.battery) {
      const resp = await api.readVbat();
      if (resp.vbat) {
        const t = resp.vbat.tempC === null ? "unavailable" : `${resp.vbat.tempC} C`;
        log.warn(`VBAT = ${resp.vbat.mv ?? "n/a"} mV; temperature = ${t}`);
        process.exit(resp.vbat.status === 0 ? 0 : 1);
      }
      process.exit(resp.index === 0 ? 0 : 1);
    }

    await api.info();
    await api.clearMacList();
    for (const item of whiteList) {
      await api.addWhitelist(item);
    }
    for (const item of blackList) {
      await api.addBlacklist(item);
    }

    await api.scanStart({
      phy1m: opts.phy === "1m" || opts.phy === "both",
      phyCoded: opts.phy === "coded" || opts.phy === "both",
      filterRandomAddresses: opts.filterRandom,
      filterPrivateAddresses: opts.filterPrivate,
      windowMs: opts.scanWindowMs,
    });
    scanStarted = true;

    startedAt = Date.now();
    lastAdvAt = startedAt;
    let lastStatusAt = startedAt;
    let done = false;

    while (!done) {
      const now = Date.now();

      if (opts.duration > 0 && now - startedAt >= opts.duration * 1000) {
        log.warn(`Duration reached: ${opts.duration.toFixed(1)} seconds`);
        done = true;
      }
      if (!done && opts.idleTimeout > 0 && now - lastAdvAt >= opts.idleTimeout * 1000) {
        log.warn(`Idle timeout reached: ${opts.idleTimeout.toFixed(1)} seconds`);
        done = true;
      }
      if (!done && opts.statusInterval > 0 && now - lastAdvAt >= opts.statusInterval * 1000 && now - lastStatusAt >= opts.statusInterval * 1000) {
        log.info(`idle: no advertisements for ${((now - lastAdvAt) / 1000).toFixed(1)} seconds`);
        lastStatusAt = now;
      }
      if (!done && opts.infoAfter > 0 && infoSentAt === 0 && now - startedAt >= opts.infoAfter * 1000) {
        log.warn(`Sending INFO during active scan at ${((now - startedAt) / 1000).toFixed(1)}s`);
        infoSentAt = now;
        infoAcked = false;
        api.info().catch((err) => log.error(`INFO send failed: ${err.message}`));
      }
      if (!done && infoSentAt !== 0 && !infoAcked && now - infoSentAt >= 2000) {
        log.warn("INFO during scan not acknowledged within 2.0 seconds");
        infoSentAt = 0;
      }
      if (!done && infoSentAt !== 0 && infoAcked) {
        log.warn("INFO during scan acknowledged");
        infoSentAt = 0;
      }
      if (!done && opts.number > 0 && advCount >= opts.number) {
        done = true;
      }

      await sleep(20);
    }

    if (scanStarted) {
      await api.scanStop().catch(() => {});
      scanStarted = false;
    }
    await api.close();
    process.exit(0);
  } catch (err) {
    log.error(err.message || String(err));
    try {
      if (scanStarted) {
        await api.scanStop().catch(() => {});
      }
      await api.close().catch(() => {});
    } catch (_e) {
      // ignore close errors
    }
    process.exit(1);
  }
})();
