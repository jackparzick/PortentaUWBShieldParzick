#include <PortentaUWBShieldParzick.h>

/**
 * Multi-Session Anchor Demo (DIAGNOSTIC)
 *
 * Adds:
 *  - prints return codes for init/start
 *  - prints session state from HAL
 *  - prints status/distance even for invalid measurements
 *  - throttles detail to avoid serial spam
 *
 * IMPORTANT FIX:
 *  - Treat status 0x1B (27) as a "valid" measurement status (OK_NEGATIVE_DISTANCE_REPORT).
 *    Otherwise you will see ranging notifications but never print distances.
 */

// ---- knobs ----
#define PRINT_DETAIL_EVERY_MS 250   // throttle detailed measurement prints
#define PRINT_INVALID_ALWAYS  1     // print invalid measures even when throttled
#define PRINT_PEER_ADDR       1

// ---- status codes we care about (from observed behavior) ----
#define ST_OK                 0x00
#define ST_OK_NEG_DIST        0x1B   // 27 decimal: OK_NEGATIVE_DISTANCE_REPORT
#define ST_RX_TIMEOUT         0x21   // 33 decimal: RX_TIMEOUT (often paired with dist=0xFFFF)

static uint32_t gHdl1 = 0;
static uint32_t gHdl2 = 0;

static volatile uint32_t gNtf1 = 0, gNtf2 = 0;
static volatile uint32_t gAvail1 = 0, gAvail2 = 0;

static uint32_t gLastDetailMs = 0;

// helper
static void printHex2(uint8_t b) { if (b < 0x10) Serial.print("0"); Serial.print(b, HEX); }

static void printPeer(const uint8_t peer[8], bool shortMode=true) {
#if PRINT_PEER_ADDR
  Serial.print("peer=");
  int n = shortMode ? 2 : 8;
  for (int i=0;i<n;i++) printHex2(peer[i]);
  Serial.print(" ");
#endif
}

static void printSessState(const char* name, uint32_t hdl) {
  uint8_t st = 0xFF;
  uwb::Status rc = UWBHAL.getSessionState(hdl, st);
  Serial.print(name);
  Serial.print(" state: rc="); Serial.print((int)rc);
  Serial.print(" st="); Serial.println((int)st);
}

static void printStatus(const char* label, uwb::Status st) {
  Serial.print(label);
  Serial.print(" -> ");
  Serial.println((int)st);  // 0 = SUCCESS
}

static const char* statusLabel(uint8_t st) {
  switch (st) {
    case ST_OK:          return "OK";
    case ST_OK_NEG_DIST: return "OK_NEG_DIST";
    case ST_RX_TIMEOUT:  return "RX_TIMEOUT";
    default:             return "OTHER";
  }
}

// Ranging callback
void rangingHandler(UWBRangingData &d) {
  const uint32_t h = d.sessionHandle();
  const uint8_t type = d.measureType();
  const uint8_t avail = d.available();

  if (h == gHdl1) { gNtf1++; gAvail1 += avail; }
  else if (h == gHdl2) { gNtf2++; gAvail2 += avail; }

  // Always show that notification happened + the key header values
  Serial.print("NTF h=0x"); Serial.print(h, HEX);
  Serial.print(" type="); Serial.print(type);
  Serial.print(" avail="); Serial.println(avail);

  // Only decode TWO_WAY measures in this demo
  if (type != (uint8_t)uwb::MeasurementType::TWO_WAY) return;

  RangingMeasures twr = d.twoWayRangingMeasure();

  // throttle detail prints
  const bool timeToPrint = (millis() - gLastDetailMs) >= PRINT_DETAIL_EVERY_MS;
  if (timeToPrint) gLastDetailMs = millis();

  for (int j=0; j<avail; j++) {
    // Pull these once so printing stays consistent
    const uint8_t  st   = twr[j].status;
    const uint16_t dist = twr[j].distance;

    // OLD (too strict): const bool valid = (twr[j].status == 0) && (twr[j].distance != 0xFFFF);
    // NEW: treat 0x1B as valid too (still a usable measurement)
    const bool valid = (dist != 0xFFFF) && (st == ST_OK || st == ST_OK_NEG_DIST);

    // print all invalid (optional), and print valid only when timeToPrint
    if (!valid) {
#if PRINT_INVALID_ALWAYS
      Serial.print("  ["); Serial.print(j); Serial.print("] ");
      printPeer(twr[j].peer_addr, true);

      Serial.print("st="); Serial.print(st);
      Serial.print(" (0x"); Serial.print(st, HEX); Serial.print(" ");
      Serial.print(statusLabel(st)); Serial.print(")");

      Serial.print(" dist="); Serial.print(dist);
      Serial.print(" slot="); Serial.print(twr[j].slot_index);
      Serial.print(" rssi="); Serial.print(twr[j].rssi);
      Serial.print(" nlos="); Serial.print(twr[j].nlos);
      Serial.println();
#endif
      continue;
    }

    if (timeToPrint) {
      Serial.print("  ["); Serial.print(j); Serial.print("] ");
      printPeer(twr[j].peer_addr, true);

      Serial.print("OK dist="); Serial.print(dist);
      Serial.print(" (st=0x"); Serial.print(st, HEX);
      Serial.print(" "); Serial.print(statusLabel(st)); Serial.print(")");

      Serial.print(" slot="); Serial.print(twr[j].slot_index);
      Serial.print(" rssi="); Serial.print(twr[j].rssi);
      Serial.println();
    }
  }
}

void setup() {
  Serial.begin(115200);

#if defined(ARDUINO_PORTENTA_C33)
  pinMode(LEDR, OUTPUT);
  digitalWrite(LEDR, LOW);
#endif

  Serial.println("\n=== Multi-session anchor (diagnostic) ===");
  UWB.registerRangingCallback(rangingHandler);

  UWB.begin();
  Serial.println("UWB.begin done");

  // NOTE: UWB.state() returns Status (0=SUCCESS) in this wrapper
  while (UWB.state() != 0) delay(10);

  // Session 1
  Serial.println("Starting session 1 ...");
  uint8_t anchor1Addr[] = {0x11, 0x11};
  uint8_t tag1Addr[]    = {0x22, 0x22};
  UWBMacAddress anchor1Mac(UWBMacAddress::Size::SHORT, anchor1Addr);
  UWBMacAddress tag1Mac   (UWBMacAddress::Size::SHORT, tag1Addr);
  UWBMultiSessionAnchor session1(0x111111, anchor1Mac, tag1Mac, 10);

  uwb::Status st = session1.init();
  printStatus("sess1 init", st);
  gHdl1 = session1.sessionHandle();
  Serial.print("sess1 handle=0x"); Serial.println(gHdl1, HEX);
  printSessState("sess1 (after init)", gHdl1);

  st = session1.start();
  printStatus("sess1 start", st);
  printSessState("sess1 (after start)", gHdl1);

  // Session 2
  Serial.println("Starting session 2 ...");
  uint8_t anchor2Addr[] = {0x33, 0x33};
  uint8_t tag2Addr[]    = {0x44, 0x44};
  UWBMacAddress anchor2Mac(UWBMacAddress::Size::SHORT, anchor2Addr);
  UWBMacAddress tag2Mac   (UWBMacAddress::Size::SHORT, tag2Addr);
  UWBMultiSessionAnchor session2(0x222222, anchor2Mac, tag2Mac, 11);

  st = session2.init();
  printStatus("sess2 init", st);
  gHdl2 = session2.sessionHandle();
  Serial.print("sess2 handle=0x"); Serial.println(gHdl2, HEX);
  printSessState("sess2 (after init)", gHdl2);

  st = session2.start();
  printStatus("sess2 start", st);
  printSessState("sess2 (after start)", gHdl2);

  Serial.println("Multi-session anchor ready!");
}

void loop() {
#if defined(ARDUINO_PORTENTA_C33)
  digitalWrite(LEDR, !digitalRead(LEDR));
#endif

  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Serial.print("HEALTH ntf(s1/s2)="); Serial.print(gNtf1);
    Serial.print("/"); Serial.print(gNtf2);
    Serial.print("  availSum(s1/s2)="); Serial.print(gAvail1);
    Serial.print("/"); Serial.println(gAvail2);

    printSessState("sess1 periodic", gHdl1);
    printSessState("sess2 periodic", gHdl2);
  }

  delay(100);
}
