/*
  ArpSpoofer3.0-beta-2025
  Platform  : Arduino Nano + ENC28J60
  Purpose   : Broadcast correct gratuitous ARP frames at a configurable rate
  Features  : EtherCard stack, 60-byte frames, micros()-based pacing,
              watchdog, DHCP backoff, stable locally administered MAC.
  Notes     : Educational use only. Deploy on isolated lab networks and
              comply with local policies and regulations.
*/

#include <SPI.h>
#include <EtherCard.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

#define LED_PIN          13
#define ENC_CS_PIN       10
#define FRAME_LEN        60
#define ETH_HDR_LEN      14
#define ARP_PAYLOAD_LEN  28
#define DEFAULT_PPS      50
#define MAX_PPS          200
#define DHCP_BASE_MS     500UL
#define DHCP_MAX_BACKOFF 8000UL
#define DHCP_INITIAL_ATTEMPTS 5

// EtherCard shared buffer (keep modest to preserve SRAM on AVR)
byte Ethernet::buffer[350];

// Stable locally administered unicast MAC address (bit1 set, bit0 cleared)
static uint8_t myMac[6] = { 0x02, 0xAB, 0x03, 0x22, 0x55, 0x99 };
static uint8_t myIp[4]  = { 0, 0, 0, 0 };

static uint16_t packetsPerSecond = DEFAULT_PPS;
static uint32_t intervalUs = 1000000UL / DEFAULT_PPS;
static uint32_t nextSendUs = 0;
static uint32_t counterResetMs = 0;
static uint16_t packetsThisSecond = 0;

static bool isConnected = false;
static uint32_t lastDhcpAttemptMs = 0;
static uint8_t dhcpRetries = 0;

// Frame layout offsets
enum {
  OFF_DEST_MAC = 0,
  OFF_SRC_MAC  = 6,
  OFF_TYPE     = 12,
  OFF_HTYPE    = 14,
  OFF_PTYPE    = 16,
  OFF_HLEN     = 18,
  OFF_PLEN     = 19,
  OFF_OPER     = 20,
  OFF_SHA      = 22,
  OFF_SPA      = 28,
  OFF_THA      = 32,
  OFF_TPA      = 38
};

// Gratuitous ARP request template (fields patched at runtime)
const uint8_t ARP_TEMPLATE[FRAME_LEN] PROGMEM = {
  // Ethernet header
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,           // Destination MAC (broadcast)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,           // Source MAC placeholder
  0x08, 0x06,                                   // Ethertype (ARP)
  // ARP payload
  0x00, 0x01,                                   // Hardware type (Ethernet)
  0x08, 0x00,                                   // Protocol type (IPv4)
  0x06,                                         // Hardware size
  0x04,                                         // Protocol size
  0x00, 0x01,                                   // Operation (request)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,           // Sender hardware address placeholder
  0x00, 0x00, 0x00, 0x00,                       // Sender protocol address placeholder
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,           // Target hardware address (zero for GARP)
  0x00, 0x00, 0x00, 0x00,                       // Target protocol address placeholder
  // Padding to reach 60 bytes (minimum Ethernet frame size without FCS)
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

static_assert(ETH_HDR_LEN + ARP_PAYLOAD_LEN == 42, "ARP frame payload mismatch");
static_assert(FRAME_LEN == 60, "Ethernet frame must be 60 bytes");

static void safeDelay(uint32_t ms) {
  const uint32_t start = millis();
  while ((uint32_t)(millis() - start) < ms) {
    wdt_reset();
    delay(10);
  }
}

static inline void setPacketsPerSecond(uint16_t newValue) {
  if (newValue == 0) newValue = 1;
  if (newValue > MAX_PPS) newValue = MAX_PPS;
  packetsPerSecond = newValue;
  intervalUs = 1000000UL / packetsPerSecond;
}

static inline void stampFrame(void) {
  // Copy template into the EtherCard buffer
  memcpy_P(Ethernet::buffer, ARP_TEMPLATE, FRAME_LEN);
  // Populate dynamic fields
  memcpy(Ethernet::buffer + OFF_SRC_MAC, myMac, 6);
  memcpy(Ethernet::buffer + OFF_SHA, myMac, 6);
  memcpy(Ethernet::buffer + OFF_SPA, myIp, 4);
  memcpy(Ethernet::buffer + OFF_TPA, myIp, 4);
}

static bool acquireDhcpLeaseOnce(void) {
  if (ether.dhcpSetup()) {
    memcpy(myIp, ether.myip, 4);
    return true;
  }
  return false;
}

static bool acquireDhcpLeaseBlocking(uint8_t attempts) {
  for (uint8_t i = 0; i < attempts; ++i) {
    if (acquireDhcpLeaseOnce()) {
      return true;
    }
    safeDelay(1000U << (i < 6 ? i : 6));  // Progressive delay that keeps the WDT alive
  }
  return false;
}

static void scheduleDhcpRetry(void) {
  const uint32_t nowMs = millis();
  uint32_t backoff = DHCP_BASE_MS << (dhcpRetries < 6 ? dhcpRetries : 6);
  if (backoff > DHCP_MAX_BACKOFF) backoff = DHCP_MAX_BACKOFF;
  if (nowMs - lastDhcpAttemptMs < backoff) return;

  lastDhcpAttemptMs = nowMs;
  if (acquireDhcpLeaseOnce()) {
    dhcpRetries = 0;
    isConnected = true;
    counterResetMs = nowMs;
    Serial.print(F("Reconnected, IP="));
    Serial.print(myIp[0]); Serial.print('.');
    Serial.print(myIp[1]); Serial.print('.');
    Serial.print(myIp[2]); Serial.print('.');
    Serial.println(myIp[3]);
  } else {
    if (dhcpRetries < 255) {
      ++dhcpRetries;
    }
  }
}

static inline void handleSerialCommands(void) {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'R' || c == 'r') {
      uint16_t requested = Serial.parseInt();
      setPacketsPerSecond(requested);
      Serial.print(F("pps="));
      Serial.println(packetsPerSecond);
    }
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  delay(25);

  wdt_enable(WDTO_2S);

  if (ether.begin(sizeof Ethernet::buffer, myMac, ENC_CS_PIN) == 0) {
    Serial.println(F("ENC28J60 init failed"));
    while (true) {
      wdt_reset();
    }
  }

  if (acquireDhcpLeaseBlocking(DHCP_INITIAL_ATTEMPTS)) {
    isConnected = true;
    dhcpRetries = 0;
  } else {
    isConnected = false;
    dhcpRetries = 1;
    lastDhcpAttemptMs = millis();
    Serial.println(F("DHCP lease unavailable, retrying..."));
  }

  setPacketsPerSecond(DEFAULT_PPS);
  nextSendUs = micros() + intervalUs;
  counterResetMs = millis();

  Serial.print(F("MAC: "));
  for (uint8_t i = 0; i < 6; ++i) {
    if (i) Serial.print(':');
    if (myMac[i] < 16) Serial.print('0');
    Serial.print(myMac[i], HEX);
  }
  Serial.println();

  Serial.print(F("IP: "));
  Serial.print(myIp[0]);
  Serial.print('.');
  Serial.print(myIp[1]);
  Serial.print('.');
  Serial.print(myIp[2]);
  Serial.print('.');
  Serial.println(myIp[3]);
}

void loop() {
  wdt_reset();
  handleSerialCommands();

  if (!isConnected) {
    digitalWrite(LED_PIN, LOW);
    scheduleDhcpRetry();
    return;
  }

  const uint32_t nowMs = millis();
  if (nowMs - counterResetMs >= 1000UL) {
    counterResetMs = nowMs;
    packetsThisSecond = 0;
  }

  const uint32_t nowUs = micros();
  if ((int32_t)(nowUs - nextSendUs) >= 0) {
    if (packetsThisSecond < MAX_PPS) {
      stampFrame();
      digitalWrite(LED_PIN, HIGH);
      ether.packetSend(FRAME_LEN);
      digitalWrite(LED_PIN, LOW);
      packetsThisSecond++;
    }
    nextSendUs += intervalUs;
  }

  // Detect lost lease (e.g., DHCP server revoked address)
  if (ether.myip[0] == 0 && ether.myip[1] == 0 &&
      ether.myip[2] == 0 && ether.myip[3] == 0) {
    isConnected = false;
    dhcpRetries = 1;
    lastDhcpAttemptMs = millis();
  }
}
