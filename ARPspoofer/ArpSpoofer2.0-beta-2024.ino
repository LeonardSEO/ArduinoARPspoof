#include <EthernetENC.h>
#include <SPI.h>

#define LED_PIN 13
#define MAX_RETRIES 5
#define MAX_PACKETS 500
#define PACKET_SIZE 48

int packetRate = 50; // packets sent per second
static uint8_t mymac[] = { 0xc0, 0xab, 0x03, 0x22, 0x55, 0x99 };

volatile bool isConnected = false;
unsigned long previousTime = 0;
int packetCounter = 0;
unsigned long counterResetTime = 0;

uint8_t arpPacket[PACKET_SIZE] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   // Destination MAC
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06,   // Source MAC
  0x08, 0x06,                           // ARP type
  0x00, 0x01,                           // Hardware type (Ethernet)
  0x08, 0x00,                           // Protocol type (IP)
  0x06, 0x04,                           // Hardware size, Protocol size
  0x00, 0x01,                           // ARP request
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01,   // Sender MAC address
  0xc0, 0xa8, 0x02, 0x01,               // Sender IP address
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   // Target MAC address
  0xFF, 0xFF, 0xFF, 0xFF                // Target IP address (broadcast)
};

void generateRandomMAC(uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    mac[i] = random(0, 256);
  }
}

void sendARP() {
  unsigned long currentTime = millis();

  if (currentTime - previousTime > 1000 / packetRate) {
    digitalWrite(LED_PIN, HIGH);

    memcpy(Ethernet.buffer, arpPacket, PACKET_SIZE);
    Ethernet.sendPacket(Ethernet.buffer, PACKET_SIZE);
    previousTime = currentTime;
    packetCounter++;
    digitalWrite(LED_PIN, LOW);

    if (packetCounter > MAX_PACKETS && (currentTime - counterResetTime) < 1000) {
      packetRate = max(1, packetRate - 5); // Decrease packet rate
    }

    if (currentTime - counterResetTime > 1000) {
      packetCounter = 0;
      counterResetTime = currentTime;
      packetRate = min(100, packetRate + 1); // Increase packet rate
    }
  }
}

bool connectToNetwork() {
  int retries = 0;
  while (retries < MAX_RETRIES) {
    if (!Ethernet.begin(mymac)) {
      retries++;
      delay(1000);
    } else {
      memcpy(arpPacket + 28, Ethernet.gatewayIP(), 4); // Copy gateway IP address
      memcpy(arpPacket + 6, mymac, 6);
      memcpy(arpPacket + 22, mymac, 6);
      memcpy(Ethernet.buffer, arpPacket, PACKET_SIZE);

      return true;
    }
  }
  return false;
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  delay(2000);
  randomSeed(analogRead(0)); // Enhance randomness

  if (Ethernet.begin(mymac) == 0) {
    while (1); // Infinite loop on Ethernet controller failure
  }

  isConnected = connectToNetwork();
}

void loop() {
  if (isConnected) {
    sendARP();
  } else {
    digitalWrite(LED_PIN, LOW); // No Connection, turn off STATUS LED
  }
}
