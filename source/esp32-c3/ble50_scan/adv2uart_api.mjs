export const CMD = {
  INFO: 0x00,
  SCAN: 0x01,
  WMAC: 0x02,
  BMAC: 0x03,
  CLRM: 0x04,
  PRNT: 0x05,
  GPIO: 0x06,
  UART: 0x08,
  RFSDK: 0x09,
  VERSION: 0x0a,
  TXADV: 0x0b,
  CONN: 0x0c,
  TXDATA: 0x0d,
  RXDATA: 0x0e,
  VBAT: 0x0f,
  GPIOEVT: 0x10,
};

export const GPIO_OP = {
  STATUS: 0,
  READ: 1,
  WRITE: 2,
  TOGGLE: 3,
  CONFIG: 4,
  PWM: 5,
  PWM_OFF: 6,
  ANALOG_READ: 7,
  RGB: 8,        // set WS2812 RGB colour: (pin, R, G, B)
};

export const GPIOEVT_OP = {
  QUERY: 0,
  ENABLE: 1,
  DISABLE: 2,
  CLEAR: 3,
};

export const GPIOEVT_EVENT_FLAG = 0x80;
export const HEAD_CRC_ADD_LEN = 13;
export const MAX_ADV_PAYLOAD = 229;
export const FRAME_RESPONSE_MARKER = 0xff;
export const ADV_DATA_MAX_LEN = 31;
export const TXDATA_MAX_LEN = 20;
export const INVALID_TEMP_RAW = -32768;
export const TEMP_SIGN_BIT = 0x8000;
export const TEMP_SIGN_EXT = 0x10000;
export const TXADV_MIN_UNITS = 0x20;
export const PARSER_MAX_BUFFER_BYTES = 32768;
export const PARSER_MAX_FRAME_BYTES = MAX_ADV_PAYLOAD + HEAD_CRC_ADD_LEN;

export const COMMAND_STATUS = {
  0: "OK",
  1: "ARGS",
  2: "PIN",
  3: "DENIED",
  4: "VALUE",
};

export function commandStatusName(code) {
  return COMMAND_STATUS[code] ?? `0x${toHex(code, 2)}`;
}

export function toHex(value, width = 2) {
  return (value >>> 0).toString(16).toUpperCase().padStart(width, "0");
}

export function bytesToHex(data) {
  let out = "";
  for (let i = 0; i < data.length; i += 1) {
    out += toHex(data[i], 2);
  }
  return out;
}

export function normalizeMac(text) {
  const normalized = String(text || "").replace(/[^0-9a-fA-F]/g, "").toUpperCase();
  const n = normalized.length;
  if (n === 0 || n % 2 !== 0 || n > 12) {
    throw new Error("MAC must be 2-12 hex chars (1-6 bytes)");
  }
  return normalized;
}

export function macToWire(text) {
  const n = normalizeMac(text);
  const bytes = hexToBytes(n);
  return bytes.reverse();
}

export function macFromWire(bytes) {
  return bytesToHex(Uint8Array.from(bytes).reverse());
}

export function hexToBytes(hex) {
  const norm = String(hex || "").replace(/[^0-9a-fA-F]/g, "");
  if (norm.length % 2 !== 0) {
    throw new Error("Hex string must have even length");
  }
  const out = new Uint8Array(norm.length / 2);
  for (let i = 0; i < norm.length; i += 2) {
    out[i / 2] = parseInt(norm.slice(i, i + 2), 16);
  }
  return out;
}

export function crc16Modbus(data) {
  let crc = 0xffff;
  for (let i = 0; i < data.length; i += 1) {
    crc ^= data[i];
    for (let bit = 0; bit < 8; bit += 1) {
      if (crc & 1) {
        crc = (crc >>> 1) ^ 0xa001;
      } else {
        crc >>>= 1;
      }
      crc &= 0xffff;
    }
  }
  return crc;
}

export function appendCrc(payload) {
  const crc = crc16Modbus(payload);
  const out = new Uint8Array(payload.length + 2);
  out.set(payload, 0);
  out[payload.length] = crc & 0xff;
  out[payload.length + 1] = (crc >>> 8) & 0xff;
  return out;
}

export function scanUnitsFromMs(ms) {
  const units = Math.floor(Number(ms) / 0.625);
  if (!Number.isFinite(units)) {
    return 0;
  }
  return Math.max(0, Math.min(0x7fff, units));
}

export function buildScanPayload(config = {}) {
  const {
    ownAddressType = 0,
    filterRandomAddresses = false,
    filterPrivateAddresses = false,
    duplicateFilter = false,
    activeScan = false,
    phy1m = true,
    phyCoded = true,
    windowMs = 30,
    windowCodedMs = 0,
  } = config;

  let flag = (ownAddressType & 0x03) << 6;
  if (filterRandomAddresses) flag |= 1 << 5;
  if (filterPrivateAddresses) flag |= 1 << 4;
  if (duplicateFilter) flag |= 1 << 3;
  if (activeScan) flag |= 1 << 2;
  if (phyCoded) flag |= 1 << 1;
  if (phy1m) flag |= 1;

  const u1m = scanUnitsFromMs(windowMs);
  const codedMs = windowCodedMs > 0 ? windowCodedMs : windowMs;
  if (phyCoded && Math.abs(codedMs - windowMs) > 0.1) {
    const uc = scanUnitsFromMs(codedMs);
    return new Uint8Array([
      CMD.SCAN,
      flag,
      u1m & 0xff,
      (u1m >>> 8) & 0xff,
      uc & 0xff,
      (uc >>> 8) & 0xff,
    ]);
  }

  return new Uint8Array([
    CMD.SCAN,
    flag,
    u1m & 0xff,
    (u1m >>> 8) & 0xff,
  ]);
}

const CMD_NAME_BY_VALUE = Object.fromEntries(
  Object.entries(CMD).map(([name, value]) => [value, name]),
);

function commandName(cmd) {
  return CMD_NAME_BY_VALUE[cmd] ?? `0x${toHex(cmd, 2)}`;
}

export class FrameParser {
  constructor() {
    this.buffer = new Uint8Array(4096);
    this.start = 0;
    this.end = 0;
    this.synced = false;
  }

  _length() {
    return this.end - this.start;
  }

  _compact() {
    if (this.start === 0) {
      return;
    }
    if (this.start === this.end) {
      this.start = 0;
      this.end = 0;
      return;
    }
    this.buffer.copyWithin(0, this.start, this.end);
    this.end -= this.start;
    this.start = 0;
  }

  _ensureCapacity(extraLen) {
    const need = this._length() + extraLen;
    if (need <= this.buffer.length) {
      return;
    }
    this._compact();
    if (need <= this.buffer.length) {
      return;
    }
    let cap = this.buffer.length;
    while (cap < need) {
      cap *= 2;
    }
    const next = new Uint8Array(cap);
    next.set(this.buffer.subarray(this.start, this.end), 0);
    this.end = this._length();
    this.start = 0;
    this.buffer = next;
  }

  _dropHead(n) {
    this.start += n;
    if (this.start >= this.end) {
      this.start = 0;
      this.end = 0;
    }
  }

  _peekByte(offset = 0) {
    return this.buffer[this.start + offset];
  }

  _peekSlice(len) {
    return this.buffer.slice(this.start, this.start + len);
  }

  _appendChunk(chunk) {
    this._ensureCapacity(chunk.length);
    this.buffer.set(chunk, this.end);
    this.end += chunk.length;
  }

  feed(chunk) {
    if (!(chunk instanceof Uint8Array)) {
      chunk = new Uint8Array(chunk || []);
    }
    if (chunk.length === 0) {
      return [];
    }
    this._appendChunk(chunk);

    const out = [];
    while (this._length() >= HEAD_CRC_ADD_LEN) {
      const payloadLen = this._peekByte(0);
      const totalLen = payloadLen + HEAD_CRC_ADD_LEN;

      if (payloadLen > MAX_ADV_PAYLOAD || totalLen > PARSER_MAX_FRAME_BYTES) {
        out.push({ type: "crc_error", discard: payloadLen, reason: "payload_len" });
        this._dropHead(1);
        continue;
      }
      if (this._length() < totalLen) {
        break;
      }

      const packet = this._peekSlice(totalLen);
      if (crc16Modbus(packet) !== 0) {
        out.push({ type: "crc_error", discard: payloadLen, reason: "crc" });
        this._dropHead(1);
        continue;
      }

      this.synced = true;
      out.push(this._parsePacket(packet));
      this._dropHead(totalLen);
    }

    if (this._length() > PARSER_MAX_BUFFER_BYTES) {
      out.push({ type: "crc_error", discard: this._peekByte(0), reason: "buffer_overflow" });
      const keep = PARSER_MAX_BUFFER_BYTES / 2;
      const drop = this._length() - keep;
      this._dropHead(drop);
    }

    return out;
  }

  _parseAdvPacket(packet) {
    const payloadLen = packet[0];
    const rssiRaw = packet[1];
    const addressType = packet[2];
    const eventType = packet[3];
    const phys = packet[4];
    const macRaw = packet.slice(5, 11);
    const payload = packet.slice(11, 11 + payloadLen);
    return {
      type: "adv",
      timestamp: Date.now(),
      payloadLen,
      rssi: (rssiRaw & 0x80) ? rssiRaw - 256 : rssiRaw,
      addressType,
      eventType,
      phys,
      mac: macFromWire(macRaw),
      payload,
      payloadHex: bytesToHex(payload),
      raw: packet,
    };
  }

  _parseCommandResponsePacket(packet) {
    const command = packet[1];
    const status = packet[2];
    const dataLen = packet[3];
    const combined = new Uint8Array(6 + packet[0]);
    combined.set(packet.slice(5, 11), 0);
    combined.set(packet.slice(11, 11 + packet[0]), 6);
    const data = combined.slice(0, Math.min(dataLen, combined.length));

    const evt = {
      type: "response",
      timestamp: Date.now(),
      command,
      commandName: commandName(command),
      status,
      statusName: commandStatusName(status),
      dataLen,
      data,
      raw: packet,
    };

    if (command === CMD.INFO) {
      evt.info = this._decodeInfo(status, data);
    }

    if (command === CMD.GPIOEVT) {
      evt.gpioevt = this._decodeGpioEvt(status, data);
    }
    if (command === CMD.VBAT) {
      evt.vbat = this._decodeVbat(status, data);
    }
    return evt;
  }

  _parsePacket(packet) {
    const phys = packet[4];
    if (phys === FRAME_RESPONSE_MARKER) {
      return this._parseCommandResponsePacket(packet);
    }
    return this._parseAdvPacket(packet);
  }

  _decodeVbat(status, data) {
    let mv = null;
    let tempC = null;
    if (data.length >= 2) {
      mv = data[0] | (data[1] << 8);
    }
    if (data.length >= 4) {
      let t = data[2] | (data[3] << 8);
      if (t & TEMP_SIGN_BIT) {
        t -= TEMP_SIGN_EXT;
      }
      if (t !== INVALID_TEMP_RAW) {
        tempC = t;
      }
    }
    return {
      status,
      statusName: commandStatusName(status),
      mv,
      tempC,
    };
  }

  _decodeInfo(status, data) {
    let localMac = null;
    if (data.length >= 6) {
      localMac = macFromWire(data.slice(0, 6));
    }
    return {
      status,
      statusName: commandStatusName(status),
      fwVersion: status,
      localMac,
    };
  }

  _decodeGpioEvt(statusOrEventFlag, data) {
    if (statusOrEventFlag & GPIOEVT_EVENT_FLAG) {
      if (data.length < 6) {
        return { mode: "event", error: "short", data };
      }
      const pin = data[0];
      const level = data[1];
      const tsMs = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
      return { mode: "event", pin, level, tsMs, edge: level ? "RISE" : "FALL" };
    }

    if (data.length >= 5) {
      const pin = data[0];
      const mask = data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24);
      return { mode: "ack", status: statusOrEventFlag, statusName: commandStatusName(statusOrEventFlag), pin, mask };
    }
    if (data.length >= 4) {
      const mask = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
      return { mode: "query", status: statusOrEventFlag, statusName: commandStatusName(statusOrEventFlag), mask };
    }
    return { mode: "ack", status: statusOrEventFlag, statusName: commandStatusName(statusOrEventFlag), data };
  }
}

export class Adv2UartApi {
  constructor(transport, options = {}) {
    if (!transport) {
      throw new Error("Transport is required");
    }
    this.transport = transport;
    this.parser = new FrameParser();
    this.listeners = new Map();
    this.pendingByCmd = new Map();
    this.lastRxTs = 0;
    this.options = {
      crcDebug: Boolean(options.crcDebug),
    };

    this.transport.setDataHandler((chunk) => {
      this._onData(chunk);
    });
    this.transport.setErrorHandler((err) => {
      this.emit("error", { error: err });
    });
  }

  on(eventName, handler) {
    if (!this.listeners.has(eventName)) {
      this.listeners.set(eventName, new Set());
    }
    this.listeners.get(eventName).add(handler);
    return () => this.off(eventName, handler);
  }

  off(eventName, handler) {
    const set = this.listeners.get(eventName);
    if (!set) {
      return;
    }
    set.delete(handler);
  }

  emit(eventName, payload) {
    const set = this.listeners.get(eventName);
    if (!set) {
      return;
    }
    for (const fn of Array.from(set)) {
      try {
        fn(payload);
      } catch (error) {
        if (eventName !== "error") {
          this.emit("error", { error });
        }
      }
    }
  }

  once(eventName, handler) {
    const wrapped = (payload) => {
      this.off(eventName, wrapped);
      handler(payload);
    };
    return this.on(eventName, wrapped);
  }

  async open(config) {
    await this.transport.open(config);
    this.emit("open", { config });
  }

  async close() {
    await this.transport.close();
    this.emit("close", {});
  }

  async sendPayload(payload) {
    const body = payload instanceof Uint8Array ? payload : new Uint8Array(payload);
    const frame = appendCrc(body);
    await this.transport.write(frame);
    this.emit("tx", {
      payload: body,
      frame,
      payloadHex: bytesToHex(body),
      frameHex: bytesToHex(frame),
      command: body[0],
      commandName: commandName(body[0]),
    });
  }

  waitForResponse(command, timeoutMs = 4000, options = {}) {
    const { matcher = null, correlationId = null } = options;
    return new Promise((resolve, reject) => {
      const item = { resolve, reject, timeout: null, matcher, correlationId };
      if (!this.pendingByCmd.has(command)) {
        this.pendingByCmd.set(command, []);
      }
      this.pendingByCmd.get(command).push(item);

      item.timeout = setTimeout(() => {
        const queue = this.pendingByCmd.get(command) || [];
        const idx = queue.indexOf(item);
        if (idx >= 0) {
          queue.splice(idx, 1);
        }
        const age = this.lastRxTs ? (Date.now() - this.lastRxTs) : -1;
        const hint = age < 0
          ? "no RX data seen"
          : `last RX ${age} ms ago`;
        reject(new Error(`Timeout waiting for command response 0x${toHex(command)} (${hint})`));
      }, timeoutMs);
    });
  }

  async request(commandPayload, timeoutMs = 4000, options = {}) {
    const command = commandPayload[0];
    const waiter = this.waitForResponse(command, timeoutMs, options);
    await this.sendPayload(commandPayload);
    return waiter;
  }

  _resolveWaiter(evt) {
    const queue = this.pendingByCmd.get(evt.command);
    if (!queue || queue.length === 0) {
      return;
    }

    let idx = -1;
    for (let i = 0; i < queue.length; i += 1) {
      const waiter = queue[i];
      if (typeof waiter.matcher === "function") {
        try {
          if (waiter.matcher(evt)) {
            idx = i;
            break;
          }
        } catch (_err) {
          // Ignore matcher failures and keep searching a matching waiter.
        }
      }
    }
    if (idx < 0) {
      idx = queue.findIndex((waiter) => typeof waiter.matcher !== "function");
    }
    if (idx < 0) {
      idx = 0;
    }

    const [waiter] = queue.splice(idx, 1);
    clearTimeout(waiter.timeout);
    waiter.resolve(evt);
  }

  _onData(chunk) {
    this.lastRxTs = Date.now();
    this.emit("rx_raw", {
      timestamp: this.lastRxTs,
      len: chunk.length,
      hex: bytesToHex(chunk),
    });
    const events = this.parser.feed(chunk);
    for (const evt of events) {
      if (evt.type === "crc_error") {
        if (evt.reason === "crc") {
          this.emit("log", { level: "warn", message: `[CRC/WARN] discard 0x${toHex(evt.discard)}` });
        } else if (this.options.crcDebug) {
          this.emit("log", { level: "debug", message: `[CRC/DBG] discard 0x${toHex(evt.discard)}` });
        }
        this.emit("crc_error", evt);
      } else if (evt.type === "response") {
        this.emit("response", evt);
        this._resolveWaiter(evt);
      } else if (evt.type === "adv") {
        this.emit("adv", evt);
      }
    }
  }

  async info() {
    return this.request(new Uint8Array([CMD.INFO]));
  }

  async version() {
    return this.request(new Uint8Array([CMD.VERSION]));
  }

  async clearMacList() {
    return this.request(new Uint8Array([CMD.CLRM]));
  }

  async addWhitelist(macOrPrefix) {
    const wire = macToWire(macOrPrefix);
    const payload = new Uint8Array(1 + wire.length);
    payload[0] = CMD.WMAC;
    payload.set(wire, 1);
    return this.request(payload);
  }

  async addBlacklist(macOrPrefix) {
    const wire = macToWire(macOrPrefix);
    const payload = new Uint8Array(1 + wire.length);
    payload[0] = CMD.BMAC;
    payload.set(wire, 1);
    return this.request(payload);
  }

  async scanStart(config = {}) {
    return this.request(buildScanPayload(config));
  }

  async scanStop() {
    return this.request(new Uint8Array([CMD.SCAN, 0x00, 0x00, 0x00]));
  }

  async readVbat() {
    return this.request(new Uint8Array([CMD.VBAT]));
  }

  async uartStatus() {
    return this.request(new Uint8Array([CMD.UART, 0]));
  }

  async uartPing(sequence = 0, pattern = 0x5a) {
    const seq = Number(sequence) & 0xffff;
    return this.request(new Uint8Array([
      CMD.UART,
      1,
      seq & 0xff,
      (seq >>> 8) & 0xff,
      Number(pattern) & 0xff,
    ]));
  }

  async uartSetBaud(index = 0) {
    return this.request(new Uint8Array([CMD.UART, 2, Number(index) & 0xff]));
  }

  async rfStatus() {
    return this.request(new Uint8Array([CMD.RFSDK, 0]));
  }

  async rfSetPower(value = 0) {
    return this.request(new Uint8Array([CMD.RFSDK, 1, Number(value) & 0xff]));
  }

  async rfSetCap(value = 0) {
    return this.request(new Uint8Array([CMD.RFSDK, 2, Number(value) & 0xff]));
  }

  async rfSetChannels(ch0 = 37, ch1 = 38, ch2 = 39) {
    return this.request(new Uint8Array([
      CMD.RFSDK,
      3,
      Number(ch0) & 0xff,
      Number(ch1) & 0xff,
      Number(ch2) & 0xff,
    ]));
  }

  async rfSetCodedMinWindow(minWindow10ms = 3) {
    return this.request(new Uint8Array([
      CMD.RFSDK,
      4,
      Number(minWindow10ms) & 0xff,
    ]));
  }

  async gpioStatus(pin) {
    const p = pin & 0xff;
    return this.request(new Uint8Array([CMD.GPIO, GPIO_OP.STATUS, p]), 4000, {
      matcher: (evt) => evt?.data?.length < 2 || evt?.data?.[0] === p || evt?.data?.[1] === p,
    });
  }

  async gpioRead(pin) {
    const p = pin & 0xff;
    return this.request(new Uint8Array([CMD.GPIO, GPIO_OP.READ, p]), 4000, {
      matcher: (evt) => evt?.data?.length < 2 || evt?.data?.[0] === p || evt?.data?.[1] === p,
    });
  }

  async gpioWrite(pin, level) {
    const p = pin & 0xff;
    return this.request(new Uint8Array([CMD.GPIO, GPIO_OP.WRITE, p, level ? 1 : 0]), 4000, {
      matcher: (evt) => evt?.data?.length < 2 || evt?.data?.[0] === p || evt?.data?.[1] === p,
    });
  }

  async gpioToggle(pin) {
    const p = pin & 0xff;
    return this.request(new Uint8Array([CMD.GPIO, GPIO_OP.TOGGLE, p]), 4000, {
      matcher: (evt) => evt?.data?.length < 2 || evt?.data?.[0] === p || evt?.data?.[1] === p,
    });
  }

  async gpioConfig(pin, inEnable, outEnable, pullMode) {
    const p = pin & 0xff;
    return this.request(new Uint8Array([
      CMD.GPIO,
      GPIO_OP.CONFIG,
      p,
      inEnable ? 1 : 0,
      outEnable ? 1 : 0,
      pullMode & 0xff,
    ]), 4000, {
      matcher: (evt) => evt?.data?.length < 2 || evt?.data?.[0] === p || evt?.data?.[1] === p,
    });
  }

  async gpioAnalogRead(pin) {
    const p = pin & 0xff;
    return this.request(new Uint8Array([CMD.GPIO, GPIO_OP.ANALOG_READ, p]), 4000, {
      matcher: (evt) => evt?.data?.length < 2 || evt?.data?.[0] === p || evt?.data?.[1] === p,
    });
  }

  async gpioPwm(pin, duty, periodUs) {
    const p = pin & 0xff;
    return this.request(new Uint8Array([
      CMD.GPIO,
      GPIO_OP.PWM,
      p,
      duty & 0xff,
      periodUs & 0xff,
      (periodUs >>> 8) & 0xff,
    ]), 4000, {
      matcher: (evt) => evt?.data?.length < 2 || evt?.data?.[0] === p || evt?.data?.[1] === p,
    });
  }

  async gpioPwmOff(pin) {
    const p = pin & 0xff;
    return this.request(new Uint8Array([CMD.GPIO, GPIO_OP.PWM_OFF, p]), 4000, {
      matcher: (evt) => evt?.data?.length < 2 || evt?.data?.[0] === p || evt?.data?.[1] === p,
    });
  }

  async gpioRgb(pin, red, green, blue) {
    const p = pin & 0xff;
    return this.request(new Uint8Array([CMD.GPIO, GPIO_OP.RGB, p, red & 0xff, green & 0xff, blue & 0xff]), 4000, {
      matcher: (evt) => evt?.data?.length < 2 || evt?.data?.[0] === p || evt?.data?.[1] === p,
    });
  }

  async gpioEventQuery() {
    return this.request(new Uint8Array([CMD.GPIOEVT, GPIOEVT_OP.QUERY]));
  }

  async gpioEventEnable(pin) {
    const p = pin & 0xff;
    return this.request(new Uint8Array([CMD.GPIOEVT, GPIOEVT_OP.ENABLE, p]), 4000, {
      matcher: (evt) => evt?.gpioevt?.pin === p || evt?.data?.[0] === p,
    });
  }

  async gpioEventDisable(pin) {
    const p = pin & 0xff;
    return this.request(new Uint8Array([CMD.GPIOEVT, GPIOEVT_OP.DISABLE, p]), 4000, {
      matcher: (evt) => evt?.gpioevt?.pin === p || evt?.data?.[0] === p,
    });
  }

  async gpioEventClear() {
    return this.request(new Uint8Array([CMD.GPIOEVT, GPIOEVT_OP.CLEAR]));
  }

  async txAdvStart({ phy = 0, intervalMs = 100, payloadHex = "020106" } = {}) {
    const advData = hexToBytes(payloadHex);
    if (advData.length === 0 || advData.length > ADV_DATA_MAX_LEN) {
      throw new Error(`TXADV payload must be 1..${ADV_DATA_MAX_LEN} bytes`);
    }
    const units = Math.max(TXADV_MIN_UNITS, Math.floor(Number(intervalMs) / 0.625));
    const payload = new Uint8Array(6 + advData.length);
    payload[0] = CMD.TXADV;
    payload[1] = 1;
    payload[2] = phy & 0xff;
    payload[3] = units & 0xff;
    payload[4] = (units >>> 8) & 0xff;
    payload[5] = advData.length;
    payload.set(advData, 6);
    return this.request(payload);
  }

  async txAdvStop() {
    return this.request(new Uint8Array([CMD.TXADV, 0]));
  }

  async txAdvStatus() {
    return this.request(new Uint8Array([CMD.TXADV, 2]));
  }

  async connStatus() {
    return this.request(new Uint8Array([CMD.CONN, 0]));
  }

  async connOpen({ coded = false, addrType = 0, mac }) {
    const wire = macToWire(mac);
    if (wire.length !== 6) {
      throw new Error("Connection MAC must be 6 bytes");
    }
    const payload = new Uint8Array(9);
    payload[0] = CMD.CONN;
    payload[1] = coded ? 2 : 1;
    payload[2] = addrType & 0xff;
    payload.set(wire, 3);
    return this.request(payload);
  }

  async connDisconnect() {
    return this.request(new Uint8Array([CMD.CONN, 3]));
  }

  async connCancel() {
    return this.request(new Uint8Array([CMD.CONN, 4]));
  }

  async txData(attHandle, dataHex) {
    const h = Number(attHandle) & 0xffff;
    const data = hexToBytes(String(dataHex || ""));
    const chunk = data.slice(0, TXDATA_MAX_LEN);
    const payload = new Uint8Array(4 + chunk.length);
    payload[0] = CMD.TXDATA;
    payload[1] = h & 0xff;
    payload[2] = (h >>> 8) & 0xff;
    payload[3] = chunk.length;
    payload.set(chunk, 4);
    return this.request(payload);
  }

}
