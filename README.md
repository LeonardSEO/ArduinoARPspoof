# Arduino ENC28J60 ARP Tool
Gratuitous ARP broadcaster for lab validation using an Arduino Nano paired with an ENC28J60 Ethernet controller.

![ENC28J60 Ethernet Shield + Arduino Nano plugged in and working](https://raw.githubusercontent.com/spacehuhn/enc28j60_ARPspoofer/master/images/1.jpg)

---

## At a Glance
- **MCU**: Arduino Nano (ATmega328P)  
- **Ethernet**: ENC28J60 (SPI)  
- **Firmware**: `ArpSpoofer3.0-beta-2025.ino` (EtherCard stack)  
- **Operation**: Broadcasts 60-byte gratuitous ARP announcements at configurable packets-per-second  
- **Scope**: Educational use on isolated lab networks only

> ⚠️  Run this firmware solely on networks you own or have explicit authorization to test. Disconnect from production environments.

---

## Feature Highlights
- Correct RFC 5227 gratuitous ARP request (THA zeroed, SPA = TPA) with 60-byte Ethernet framing
- EtherCard-based raw frame transmit path compatible with ENC28J60
- Micros()-paced rate control with per-second cap and serial CLI (`R<n>`)
- Watchdog-safe DHCP with exponential backoff and automatic reconnection logging
- Stable locally administered unicast MAC (LAA) and PROGMEM frame template to conserve SRAM

---

## Hardware Requirements
- Arduino Nano (ATmega328P) or pin-compatible board  
- ENC28J60 Ethernet module/shield (3.3 V with proper level shifting)  
- USB cable for power and programming  
- Isolated Ethernet segment for validation tests

![ENC28J60 Ethernet Shield and an Arduino Nano](https://raw.githubusercontent.com/spacehuhn/enc28j60_ARPspoofer/master/images/2.jpg)

---

## Firmware Layout
| Sketch | Stack | Status | Notes |
| --- | --- | --- | --- |
| `ARPspoofer.ino` | EtherCard | Legacy | Original proof-of-concept, deprecated |
| `ArpSpoofer2.0-beta-2024.ino` | EthernetENC | Legacy | Uses undocumented raw APIs, not maintained |
| `ArpSpoofer3.0-beta-2025.ino` | EtherCard | Recommended | Lab-ready gratuitous ARP broadcaster |

---

## Getting Started
1. **Install Arduino IDE**  
   Download from the [official Arduino website](https://www.arduino.cc/en/software).

2. **Add the EtherCard Library**  
   Follow the instructions in the [EtherCard repository](https://github.com/jcw/ethercard) to install the library.

3. **Assemble the Hardware**  
   Mount the ENC28J60 module on the Arduino Nano, ensuring proper power and SPI wiring.

4. **Connect via USB**  
   Plug the Nano into your workstation. Install CH340 drivers if required: [CH341SER](http://www.wch.cn/download/CH341SER_EXE.html).

5. **Load the Firmware**  
   Open `ArpSpoofer3.0-beta-2025.ino` in the Arduino IDE, select the correct board/port, verify, and upload.

---

## Operation
- Power the device via USB and connect Ethernet to an isolated lab switch.  
- The sketch acquires a DHCP lease, sends one-time gratuitous ARP announcements, and continues at the configured `packetsPerSecond`.  
- Adjust the transmit rate over the serial console (115200 baud) with:

  ```text
  R120
  ```

  The device responds with the applied packets-per-second value.  
- LED on pin 13 toggles while frames are transmitted.  
- DHCP loss or link drop triggers backoff and automatic recovery; successful re-leases print the new IP to the console.

---

## Safety and Compliance
- Validate only on networks you own or control.  
- Ensure compliance with local laws, corporate policies, and lab safety procedures.  
- Maintain physical isolation from production or public infrastructure.

---

## Troubleshooting
- **DHCP lease not obtained**  
  Confirm the lab DHCP server is reachable and that the ENC28J60 has link (check cabling and power).
- **Link LED off**  
  Inspect 3.3 V supply and ensure the RJ45 jack is connected to an active switch port.
- **Serial commands ignored**  
  Verify baud rate (115200) and send commands terminated with newline (e.g., `R75\n`).
- **Network tooling detects short frames**  
  Ensure you are running the 3.0 firmware; earlier versions do not pad to 60 bytes.

---

## Roadmap (4.0 Concepts)
- State machine covering init, DHCP probing, announce, run, and degraded recovery
- Token-bucket rate limiting with configurable burst budget
- RFC 5227 announce sequences with optional jitter
- PHY link-status monitoring and pause-resume logic
- EEPROM-backed configuration for MAC, rate, and announce policy

---

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

## Sources and References
- ARP spoofing overview: https://en.wikipedia.org/wiki/ARP_spoofing  
- ENC28J60 data sheet: http://www.microchip.com/wwwproducts/en/en022889  
- RFC 5227 – IPv4 Address Conflict Detection: https://www.rfc-editor.org/rfc/rfc5227  
- EtherCard library: https://github.com/jcw/ethercard

**Created by LeonardSEO.**
