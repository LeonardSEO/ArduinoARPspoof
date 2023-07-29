# Arduino ARP-Spoof
Perform an ARP spoofing attack to disrupt communication within your LAN using an Arduino Nano and an ENC28J60 Ethernet controller.

![ENC28J60 Ethernet Shield + Arduino Nano plugged in and working](https://raw.githubusercontent.com/spacehuhn/enc28j60_ARPspoofer/master/images/1.jpg)  

## Contents
- [Introduction](#introduction)
  - [What it is](#what-it-is)
  - [How it works](#how-it-works)
  - [What an ENC28J60 is](#what-an-enc28j60-is)
  - [How to protect against it](#how-to-protect-against-it)
- [Disclaimer](#disclaimer)
- [Installation](#installation)
- [How to use it](#how-to-use-it)
- [License](#license)
- [Sources and additional links](#sources-and-additional-links)

## Introduction ##

### What it is

Using an Arduino Nano with an Ethernet controller, this device will perform an [ARP spoofing](https://en.wikipedia.org/wiki/ARP_spoofing) attack to block communication from every client device in your LAN to the gateway.  

### How it works

The device sends ARP replies to every device in the LAN and informs them that the gateway is at a random MAC address.  
The gateway is the link between your local network and the Internet. By indicating to all devices that the gateway is at a non-existent address, communication to the gateway is cut off, causing all devices to lose their connection.  

The code is optimized to balance efficiency and stealth, sending packets at an adaptive rate that is adjusted based on network conditions and randomizing MAC addresses to make detection more difficult.

### What an ENC28J60 is

The ENC28J60 is a cost-effective 10mbit SPI Ethernet controller for Arduino. Because it has a very open and easily hackable library, it's perfect for this project and you could even program a man-in-the-middle attack or other funny stuff with it.

![ENC28J60 Ethernet Shield and an Arduino Nano](https://raw.githubusercontent.com/spacehuhn/enc28j60_ARPspoofer/master/images/2.jpg)

### How to protect against it

Use a router, network switch, or software that provides protection against ARP spoofing.  
Note: This has not been tested on such protected hardware yet.  

## Disclaimer

**Use it only for testing purposes on your own network!**  

## Installation

You will need an Arduino Nano and an ENC28J60.  
If you buy an Arduino Ethernet shield, make sure it **doesn't** use a Wiznet controller (e.g., W5100 or W5500), **this project will only work with an ENC28J60!**

**1. Assemble the Hardware**

Take the Arduino Nano and slide the ENC28J60 onto it. They should fit together perfectly.

**2. Connect to Your Computer**

Take a USB cable and plug the Arduino into your computer.

**3. Install the Drivers**

If your computer does not recognize the Arduino Nano, you might need to install additional drivers. You can download them [here](http://www.wch.cn/download/CH341SER_EXE.html).

**4. Install the Arduino IDE**

Open the Arduino IDE program. If you don't have it, you can download it from the [official website](https://www.arduino.cc/en/software).

**5. Install the EtherCard Library**

You will need to add the EtherCard library in Arduino.  
How to do that is explained here: [EtherCard Library](https://github.com/jcw/ethercard)

**6. Upload the Code**

Open the ARP spoofing code in the Arduino IDE. Verify the code and then upload it to your Arduino. You're done!

## How to use it

Once the Arduino is powered over USB and an Ethernet cable is plugged in, the device will start the ARP spoofing attack automatically.

## License

This project is licensed under the MIT License - see the [license file](LICENSE) for details.

## Sources and additional links

ARP spoofing: https://en.wikipedia.org/wiki/ARP_spoofing  
ENC28J60: http://www.microchip.com/wwwproducts/en/en022889

**Created by LeonardSEO.**
