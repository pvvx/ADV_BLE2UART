ESP32-C3 USB

Flash:

```
flash.cmd COM19
```

Edit the blacklist or whitelist and scan parameters in adv2uart.py.

Run:

```
# python3 adv2uart.py COM13

Press 'ESC' to exit
Connecting to COM19 ...
Open COM19, 921600 bit/s... ok
cmd: 04
Start passive scan PHY 1M, Windows 30 ms
cmd: 01033000
resp: 04 ff
resp: 01 ff 333000
adv: c4 13 01 00 381f8dd93cb6 020106030201a2141601a2016db2d52cd37c86fa958e84fb795ead4e
adv: bc 13 01 00 381f8d941e11 020106030201a2141601a2019d2de84dff21d8f26160e052d0d0b672
adv: a3 01 33 00 a4c138ae3ebf 0201060e16d2fc40008d014702b5f50323140b095453305f414533454246
adv: a7 13 01 00 381f8dd8b52d 020106030201a2141601a2013291499cc35a871c792b8e1772197a78
adv: c7 13 01 00 381f8dd93cb6 020106030201a2141601a2016db2d52cd37c86fa958e84fb795ead4e
adv: df 10 01 01 06ab94c45427 1eff06000109200610edf4c47ea3332af7ce79e8412373db8a6140cac9de4b
adv: bc 13 01 00 381f8d941e11 020106030201a2141601a2019d2de84dff21d8f26160e052d0d0b672
adv: a9 13 01 00 381f8dd8b52d 020106030201a2141601a2013291499cc35a871c792b8e1772197a78
cmd: 01000000
```

| rssi | Event Type | PHY | Address.Type | MAC | Adv.Data |
| --   | --         | --  | --       | --  | --       |
| int8 | 0x00 - ADV_IND<br>0x01 - DIR_IND<br>0x02 - SCAN_IND<br>0x03 - NONCONN_IND<br>0x04 - SCAN_RSP<br>BT5.0:<br>0x13 - ADV_IND<br>0x15 - ADV_DIRECT_IND<br>0x12 - ADV_SCAN_IND<br>0x10 - ADV_NONCON_IND<br>0x1b - SCAN_RSP_ADV_IND<br>0x1a - SCAN_RSP_ADV_SCAN_IND |  bit[0:1] - Primary PHY<br>bit[4:5] - Secondary PHY<br>1 - 1M<br>3 - Coded | bit[0:1] - Addr type<br><br>bit[4:5] - Direct addr type <br>0 - PUBLIC<br>1 - RANDOM<br>2 - PUBLIC_ID<br>3 - RANDOM_ID | 6 bytes | data |



