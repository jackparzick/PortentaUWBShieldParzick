#include <PortentaUWBShieldParzick.h>

/**
 * Multi-Session Tag 2 Demo
 *
 * Matches Anchor Session 2:
 * - Session ID: 0x222222
 * - Tag MAC: 0x4444
 * - Anchor MAC: 0x3333
 * - Preamble Code: 11  (must match anchor session 2)
 */

void rangingHandler(UWBRangingData &rangingData) {
  if (rangingData.measureType() == (uint8_t)uwb::MeasurementType::TWO_WAY) {
    RangingMeasures twr = rangingData.twoWayRangingMeasure();
    for (int j = 0; j < rangingData.available(); j++) {
      if (twr[j].status == 0 && twr[j].distance != 0xFFFF) {
        Serial.print("Distance: ");
        Serial.println(twr[j].distance);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

#if defined(ARDUINO_PORTENTA_C33)
  pinMode(LEDR, OUTPUT);
  digitalWrite(LEDR, LOW);
#endif

  uint8_t devAddr[]     = {0x44, 0x44}; // Tag2
  uint8_t destination[] = {0x33, 0x33}; // Anchor2

  UWBMacAddress srcAddr(UWBMacAddress::Size::SHORT, devAddr);
  UWBMacAddress dstAddr(UWBMacAddress::Size::SHORT, destination);

  UWB.registerRangingCallback(rangingHandler);

  UWB.begin();
  while (UWB.state() != 0) delay(10);

  // IMPORTANT: preamble must match anchor session 2 (11)
  UWBMultiSessionTag myTag(0x222222, srcAddr, dstAddr, 11);

  myTag.init();
  myTag.start();
}

void loop() {
#if defined(ARDUINO_PORTENTA_C33)
  digitalWrite(LEDR, !digitalRead(LEDR));
#endif
  delay(1000);
}
