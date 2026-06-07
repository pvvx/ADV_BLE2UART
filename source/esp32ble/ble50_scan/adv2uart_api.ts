export type CmdName =
  | "INFO"
  | "VBAT"
  | "GPIO"
  | "GPIO_EVT"
  | "RF"
  | "TX_ADV"
  | "CONSOLE"
  | "UNKNOWN";

export type StatusName =
  | "OK"
  | "ERR"
  | "NOT_SUPPORTED"
  | "INVALID_ARGS"
  | "BUSY"
  | "TIMEOUT"
  | "UNKNOWN";

export interface BaseEvent {
  readonly ts: number;
}

export interface InfoPayload {
  readonly statusName: StatusName;
  readonly fwVersion: number;
  readonly localMac: string;
  readonly raw: Uint8Array;
}

export interface VbatPayload {
  readonly statusName: StatusName;
  readonly mv?: number;
  readonly tempC?: number;
  readonly raw: Uint8Array;
}

export interface GpioPayload {
  readonly op: number;
  readonly opName: string;
  readonly statusName: StatusName;
  readonly pin?: number;
  readonly level?: number;
  readonly flags?: number;
  readonly analog?: number;
  readonly raw: Uint8Array;
}

export interface GpioEventPayload {
  readonly statusName: StatusName;
  readonly pinMaskLow: number;
  readonly pinMaskHigh: number;
  readonly raw: Uint8Array;
}

export interface RfPayload {
  readonly op: number;
  readonly opName: string;
  readonly statusName: StatusName;
  readonly localMac?: string;
  readonly remoteMac?: string;
  readonly packetType?: number;
  readonly payloadHex?: string;
  readonly raw: Uint8Array;
}

export interface TxAdvPayload {
  readonly statusName: StatusName;
  readonly running?: boolean;
  readonly txPhy?: number;
  readonly intervalMs?: number;
  readonly raw: Uint8Array;
}

export interface ConsolePayload {
  readonly statusName: StatusName;
  readonly line?: string;
  readonly raw: Uint8Array;
}

export interface GenericResponseEvent extends BaseEvent {
  readonly kind: "response";
  readonly command: number;
  readonly commandName: CmdName;
  readonly status: number;
  readonly statusName: StatusName;
  readonly data: Uint8Array;
}

export interface InfoResponseEvent extends BaseEvent {
  readonly kind: "response";
  readonly command: number;
  readonly commandName: "INFO";
  readonly status: number;
  readonly statusName: StatusName;
  readonly data: Uint8Array;
  readonly info: InfoPayload;
}

export interface VbatResponseEvent extends BaseEvent {
  readonly kind: "response";
  readonly command: number;
  readonly commandName: "VBAT";
  readonly status: number;
  readonly statusName: StatusName;
  readonly data: Uint8Array;
  readonly vbat: VbatPayload;
}

export interface GpioResponseEvent extends BaseEvent {
  readonly kind: "response";
  readonly command: number;
  readonly commandName: "GPIO";
  readonly status: number;
  readonly statusName: StatusName;
  readonly data: Uint8Array;
  readonly gpio: GpioPayload;
}

export interface GpioEventResponseEvent extends BaseEvent {
  readonly kind: "response";
  readonly command: number;
  readonly commandName: "GPIO_EVT";
  readonly status: number;
  readonly statusName: StatusName;
  readonly data: Uint8Array;
  readonly gpioevt: GpioEventPayload;
}

export interface RfResponseEvent extends BaseEvent {
  readonly kind: "response";
  readonly command: number;
  readonly commandName: "RF";
  readonly status: number;
  readonly statusName: StatusName;
  readonly data: Uint8Array;
  readonly rf: RfPayload;
}

export interface TxAdvResponseEvent extends BaseEvent {
  readonly kind: "response";
  readonly command: number;
  readonly commandName: "TX_ADV";
  readonly status: number;
  readonly statusName: StatusName;
  readonly data: Uint8Array;
  readonly txadv: TxAdvPayload;
}

export interface ConsoleResponseEvent extends BaseEvent {
  readonly kind: "response";
  readonly command: number;
  readonly commandName: "CONSOLE";
  readonly status: number;
  readonly statusName: StatusName;
  readonly data: Uint8Array;
  readonly console: ConsolePayload;
}

export type ResponseEvent =
  | InfoResponseEvent
  | VbatResponseEvent
  | GpioResponseEvent
  | GpioEventResponseEvent
  | RfResponseEvent
  | TxAdvResponseEvent
  | ConsoleResponseEvent
  | GenericResponseEvent;

export interface AdvReportEvent extends BaseEvent {
  readonly kind: "adv_report";
  readonly timestampUs: number;
  readonly packetType: number;
  readonly channel: number;
  readonly rssi: number;
  readonly senderMac: string;
  readonly targetMac: string;
  readonly payloadHex: string;
}

export interface CrcErrorEvent extends BaseEvent {
  readonly kind: "crc_error";
  readonly expected: number;
  readonly actual: number;
  readonly raw: Uint8Array;
}

export interface ParseErrorEvent extends BaseEvent {
  readonly kind: "parse_error";
  readonly message: string;
  readonly raw?: Uint8Array;
}

export type Adv2UartEvent = ResponseEvent | AdvReportEvent | CrcErrorEvent | ParseErrorEvent;

export function isResponseEvent(evt: Adv2UartEvent): evt is ResponseEvent {
  return evt.kind === "response";
}

export function isInfoResponse(evt: Adv2UartEvent): evt is InfoResponseEvent {
  return evt.kind === "response" && evt.commandName === "INFO";
}

export function isGpioResponse(evt: Adv2UartEvent): evt is GpioResponseEvent {
  return evt.kind === "response" && evt.commandName === "GPIO";
}

export function isTxAdvResponse(evt: Adv2UartEvent): evt is TxAdvResponseEvent {
  return evt.kind === "response" && evt.commandName === "TX_ADV";
}
