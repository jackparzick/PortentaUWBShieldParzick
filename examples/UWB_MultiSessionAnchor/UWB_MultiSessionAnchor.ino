#include <PortentaUWBShieldParzick.h>

/**
 * Multi-Session Anchor Demo (SAFE DIAGNOSTIC)
 *
 * Key difference vs the previous diagnostic:
 *  - DOES NOT print inside the ranging callback.
 *  - Prints a once-per-second summary from loop().
 *
 * Why: printing inside the callback can block the UWB task/context and trigger
 * sendUciCommandAndWait TIMEOUT -> uwb_bus_io_irq_wait semaphore timeouts.
 */

// ---- knobs ----
#define SUMMARY_EVERY_MS     1000
#define DETAIL_EVERY_MS      5000   // one detail snapshot occasionally (still from loop, not callback)

// ---- status codes we observed ----
#define ST_OK                 0x00
#define ST_OK_NEG_DIST        0x1B   // 27 decimal
#define ST_RX_TIMEOUT         0x21   // 33 decimal

static const char* statusLabel(uint8_t st) {
  switch (st) {
    case ST_OK:          return "OK";
    case ST_OK_NEG_DIST: return "OK_NEG_DIST";
    case ST_RX_TIMEOUT:  return "RX_TIMEOUT";
    default:             return "OTHER";
  }
}

// Session objects (local to setup), handles cached globally
static uint32_t gHdl1 = 0;
static uint32_t gHdl2 = 0;

// Counters updated in callback (keep lightweight!)
static volatile uint32_t gNtf1 = 0, gNtf2 = 0;
static volatile uint32_t gMeas1 = 0, gMeas2 = 0;

static volatile uint32_t gOk1 = 0, gOk2 = 0;          // st==0 or st==0x1B & dist valid
static volatile uint32_t gTimeout1 = 0, gTimeout2 = 0; // st==0x21
static volatile uint32_t gOther1 = 0, gOther2 = 0;    // anything else

static volatile uint16_t gLastDist1 = 0xFFFF, gLastDist2 = 0xFFFF;
static volatile uint8_t  gLastSt1 = 0xFF,   gLastSt2 = 0xFF;
static volatile uint8_t  gLastSlot1 = 0xFF, gLastSlot2 = 0xFF;

static volatile uint8_t  gLastPeer0_1 = 0, gLastPeer1_1 = 0;
static volatile uint8_t  gLastPeer0_2 = 0, gLastPeer1_2 = 0;

// Helper: consider 0x1B as valid measurement too
static inline bool isOkStatus(uint8_t st) {
  return (st == ST_OK) || (st == ST_OK_NEG_DIST);
}

// Ranging callback (NO PRINTS HERE)
void rangingHandler(UWBRangingData &d) {
  if (d.measureType() != (uint8_t)uwb::MeasurementType::TWO_WAY) return;

  const uint32_t h = d.sessionHandle();
  const uint8_t avail = d.available();
  RangingMeasures twr = d.twoWayRangingMeasure();

  // attribute to sess1 or sess2
  bool s1 = (h == gHdl1);
  bool s2 = (h == gHdl2);
  if (!s1 && !s2) return;

  if (s1) { gNtf1++; gMeas1 += avail; }
  else    { gNtf2++; gMeas2 += avail; }

  for (int j = 0; j < avail; j++) {
    const uint8_t st = twr[j].status;
    const uint16_t dist = twr[j].distance;

    const bool ok = (dist != 0xFFFF) && isOkStatus(st);

    if (s1) {
      gLastSt1 = st; gLastDist1 = dist; gLastSlot1 = twr[j].slot_index;
      gLastPeer0_1 = twr[j].peer_addr[0]; gLastPeer1_1 = twr[j].peer_addr[1];

      if (ok) gOk1++;
      else if (st == ST_RX_TIMEOUT) gTimeout1++;
      else gOther1++;
    } else {
      gLastSt2 = st; gLastDist2 = dist; gLastSlot2 = twr[j].slot_index;
      gLastPeer0_2 = twr[j].peer_addr[0]; gLastPeer1_2 = twr[j].peer_addr[1];

      if (ok) gOk2++;
      else if (st == ST_RX_TIMEOUT) gTimeout2++;
      else gOther2++;
    }
  }
}

static void printHex2(uint8_t b) { if (b < 0x10) Serial.print("0"); Serial.print(b, HEX); }

void setup() {
  Serial.begin(115200);

#if defined(ARDUINO_PORTENTA_C33)
  pinMode(LEDR, OUTPUT);
  digitalWrite(LEDR, LOW);
#endif

  Serial.println("\n=== Multi-session anchor (SAFE DIAGNOSTIC) ===");
  UWB.registerRangingCallback(rangingHandler);

  UWB.begin();
  while (UWB.state() != 0) delay(10);

  // Session 1
  Serial.println("Starting session 1 ...");
  uint8_t anchor1Addr[] = {0x11, 0x11};
  uint8_t tag1Addr[]    = {0x22, 0x22};
  UWBMacAddress anchor1Mac(UWBMacAddress::Size::SHORT, anchor1Addr);
  UWBMacAddress tag1Mac   (UWBMacAddress::Size::SHORT, tag1Addr);
  UWBMultiSessionAnchor session1(0x111111, anchor1Mac, tag1Mac, 10);

  uwb::Status st = session1.init();
  Serial.print("sess1 init -> "); Serial.println((int)st);
  gHdl1 = session1.sessionHandle();
  Serial.print("sess1 handle=0x"); Serial.println(gHdl1, HEX);
  st = session1.start();
  Serial.print("sess1 start -> "); Serial.println((int)st);

  // Session 2
  Serial.println("Starting session 2 ...");
  uint8_t anchor2Addr[] = {0x33, 0x33};
  uint8_t tag2Addr[]    = {0x44, 0x44};
  UWBMacAddress anchor2Mac(UWBMacAddress::Size::SHORT, anchor2Addr);
  UWBMacAddress tag2Mac   (UWBMacAddress::Size::SHORT, tag2Addr);
  UWBMultiSessionAnchor session2(0x222222, anchor2Mac, tag2Mac, 11);

  st = session2.init();
  Serial.print("sess2 init -> "); Serial.println((int)st);
  gHdl2 = session2.sessionHandle();
  Serial.print("sess2 handle=0x"); Serial.println(gHdl2, HEX);
  st = session2.start();
  Serial.print("sess2 start -> "); Serial.println((int)st);

  Serial.println("Multi-session anchor ready!");
}

void loop() {
#if defined(ARDUINO_PORTENTA_C33)
  digitalWrite(LEDR, !digitalRead(LEDR));
#endif

  static uint32_t lastSum = 0;
  static uint32_t lastDetail = 0;

  const uint32_t now = millis();

  if (now - lastSum >= SUMMARY_EVERY_MS) {
    lastSum = now;

    Serial.print("SUMMARY ");
    Serial.print("ntf(s1/s2)="); Serial.print((uint32_t)gNtf1); Serial.print("/"); Serial.print((uint32_t)gNtf2);
    Serial.print(" meas(s1/s2)="); Serial.print((uint32_t)gMeas1); Serial.print("/"); Serial.print((uint32_t)gMeas2);

    Serial.print(" ok(s1/s2)="); Serial.print((uint32_t)gOk1); Serial.print("/"); Serial.print((uint32_t)gOk2);
    Serial.print(" to(s1/s2)="); Serial.print((uint32_t)gTimeout1); Serial.print("/"); Serial.print((uint32_t)gTimeout2);
    Serial.print(" other(s1/s2)="); Serial.print((uint32_t)gOther1); Serial.print("/"); Serial.println((uint32_t)gOther2);

    Serial.print("  last1 peer="); printHex2(gLastPeer0_1); printHex2(gLastPeer1_1);
    Serial.print(" st=0x"); Serial.print(gLastSt1, HEX);
    Serial.print(" "); Serial.print(statusLabel(gLastSt1));
    Serial.print(" dist="); Serial.print(gLastDist1);
    Serial.print(" slot="); Serial.print(gLastSlot1);

    Serial.print("  last2 peer="); printHex2(gLastPeer0_2); printHex2(gLastPeer1_2);
    Serial.print(" st=0x"); Serial.print(gLastSt2, HEX);
    Serial.print(" "); Serial.print(statusLabel(gLastSt2));
    Serial.print(" dist="); Serial.print(gLastDist2);
    Serial.print(" slot="); Serial.println(gLastSlot2);
  }

  // A slightly more detailed snapshot occasionally (still NO callback printing)
  if (now - lastDetail >= DETAIL_EVERY_MS) {
    lastDetail = now;
    Serial.println("DETAIL: (If you start seeing TIMEOUT storms, reduce log level or stop polling UCI state.)");
  }

  delay(50);
}
