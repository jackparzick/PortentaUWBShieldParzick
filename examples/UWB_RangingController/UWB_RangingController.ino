#include <PortentaUWBShield.h>

/**
 * this demo shows how to setup the Arduino Stella tag as simple
 * Two-Way Ranging Initiator/Controller
 * It expects a counterpart setup as a Responder/Controlee
 *
 * -------------------------------------------------------------------------
 * [PHASE0 NOTE]
 * This is an AoA bring-up patch:
 *  1) Request AoA output via Session AppConfig: AOA_RESULT_REQ (0x0D)
 *  2) Enable AoA fields in result reporting via Session AppConfig: RESULT_REPORT_CONFIG (0x2E)
 *  3) Print AoA fields that ALREADY exist in the DS-TWR measurement struct (twr_mesr)
 *
 * We are NOT changing the underlying .a firmware behavior here—only requesting
 * AoA fields and printing what the stack gives us.
 */

// ===================== [PHASE0 ADDED] AoA knobs (FiRa/UCI-aligned) =====================
// [PHASE0 ADDED] AOA_RESULT_REQ (0x0D) values (common convention):
//   0 = disable
//   1 = enable (basic)
//   2 = azimuth-only
//   3 = elevation-only
//   0xF0 = interleaved (advanced; not used in Phase 0)
static const uint32_t kAoaResultReq = 1; // [PHASE0 ADDED]

// [PHASE0 ADDED] RESULT_REPORT_CONFIG (0x2E) bitmask (common convention):
//   bit0 = ToF
//   bit1 = AoA azimuth
//   bit2 = AoA elevation
//   bit3 = AoA FOM
// 0x0F enables all four.
static const uint32_t kResultReportMask =
    (1u << 0) | // [PHASE0 ADDED] ToF
    (1u << 1) | // [PHASE0 ADDED] AoA azimuth
    (1u << 2) | // [PHASE0 ADDED] AoA elevation
    (1u << 3);  // [PHASE0 ADDED] AoA FOM

// --------------------- [PHASE0 ADDED] Small debug helpers ---------------------
static void printHex2(uint8_t b) {           // [PHASE0 ADDED]
  if (b < 0x10) Serial.print("0");           // [PHASE0 ADDED]
  Serial.print(b, HEX);                      // [PHASE0 ADDED]
}

// [PHASE0 ADDED] This project uses "SHORT" addresses (2 bytes), but the struct stores 8 bytes.
// We print the first 2 bytes so you can confirm which peer the report belongs to.
static void printPeerShort(const uint8_t peer_addr[8]) { // [PHASE0 ADDED]
  printHex2(peer_addr[0]);                                // [PHASE0 ADDED]
  printHex2(peer_addr[1]);                                // [PHASE0 ADDED]
}

// [PHASE0 ADDED] Print one DS-TWR measurement including AoA.
static void printTwrWithAoa(const uwb::twr_mesr &m) {      // [PHASE0 ADDED]
  Serial.print("peer=");                                   // [PHASE0 ADDED]
  printPeerShort(m.peer_addr);                              // [PHASE0 ADDED]

  Serial.print(" dist=");                                  // [PHASE0 ADDED]
  Serial.print(m.distance);                                // [PHASE0 ADDED]

  Serial.print(" rssi=");                                  // [PHASE0 ADDED]
  Serial.print((int)m.rssi);                               // [PHASE0 ADDED]

  Serial.print(" nlos=");                                  // [PHASE0 ADDED]
  Serial.print((int)m.nlos);                               // [PHASE0 ADDED]

  // [PHASE0 ADDED] Raw AoA fields (int16) + quality (FOM). Units depend on firmware scaling.
  Serial.print(" az=");                                    // [PHASE0 ADDED]
  Serial.print((int)m.aoa_azimuth);                        // [PHASE0 ADDED]
  Serial.print(" azF=");                                   // [PHASE0 ADDED]
  Serial.print((int)m.aoa_azimuth_fom);                    // [PHASE0 ADDED]

  Serial.print(" el=");                                    // [PHASE0 ADDED]
  Serial.print((int)m.aoa_elevation);                      // [PHASE0 ADDED]
  Serial.print(" elF=");                                   // [PHASE0 ADDED]
  Serial.print((int)m.aoa_elevation_fom);                  // [PHASE0 ADDED]

  // [PHASE0 ADDED] Optional “destination” AoA fields (often 0 unless firmware populates them)
  Serial.print(" dstAz=");                                 // [PHASE0 ADDED]
  Serial.print((int)m.aoa_dest_azimuth);                   // [PHASE0 ADDED]
  Serial.print(" dstAzF=");                                // [PHASE0 ADDED]
  Serial.print((int)m.aoa_dest_azimuth_fom);               // [PHASE0 ADDED]

  Serial.print(" dstEl=");                                 // [PHASE0 ADDED]
  Serial.print((int)m.aoa_dest_elevation);                 // [PHASE0 ADDED]
  Serial.print(" dstElF=");                                // [PHASE0 ADDED]
  Serial.print((int)m.aoa_dest_elevation_fom);             // [PHASE0 ADDED]

  Serial.println();                                        // [PHASE0 ADDED]
}

// [PHASE0 ADDED] Small helper so we don’t “drop” return codes silently
static void printStatus(const char *label, uwb::Status st) { // [PHASE0 ADDED]
  Serial.print(label);                                      // [PHASE0 ADDED]
  Serial.print(": ");                                       // [PHASE0 ADDED]
  Serial.println((int)st);                                  // [PHASE0 ADDED]
}
// =================== END [PHASE0 ADDED] helpers ===================


// handler for ranging notifications
void rangingHandler(UWBRangingData &rangingData) {

  // ---------------- ORIGINAL DEBUG PRINTS (kept but commented) ----------------
  // Serial.print("GOT RANGING DATA - Type: "  );                // [PHASE0 REMOVED]
  // Serial.println(rangingData.measureType());                  // [PHASE0 REMOVED]

  // [PHASE0 ADDED] Slightly cleaner type print (still useful for bring-up)
  Serial.print("GOT RANGING DATA - Type: ");                    // [PHASE0 ADDED]
  Serial.println(rangingData.measureType());                    // [PHASE0 ADDED]

  // if(rangingData.measureType()==(uint8_t)uwb::MeasurementType::TWO_WAY) // [PHASE0 REMOVED]
  if (rangingData.measureType() == (uint8_t)uwb::MeasurementType::TWO_WAY) // [PHASE0 ADDED]
  {

    // RangingMeasures twr=rangingData.twoWayRangingMeasure();    // [PHASE0 REMOVED]
    RangingMeasures twr = rangingData.twoWayRangingMeasure();     // [PHASE0 ADDED]

    // for(int j=0;j<rangingData.available();j++)                 // [PHASE0 REMOVED]
    for (int j = 0; j < rangingData.available(); j++)             // [PHASE0 ADDED]
    {

      // if(twr[j].status==0 && twr[j].distance!=0xFFFF)          // [PHASE0 REMOVED]
      if (twr[j].status == 0 && twr[j].distance != 0xFFFF)        // [PHASE0 ADDED]
      {
        // ---------------- ORIGINAL DISTANCE-ONLY PRINT (kept but commented) ----
        // Serial.print("Distance: ");                             // [PHASE0 REMOVED]
        // Serial.println(twr[j].distance);                        // [PHASE0 REMOVED]

        // [PHASE0 ADDED] Print distance + AoA + FOM in one line
        printTwrWithAoa(twr[j]);                                  // [PHASE0 ADDED]
      }
    }

  }

}

void setup()
{
  Serial.begin(115200);

#if defined(ARDUINO_PORTENTA_C33)
  /* Only the Portenta C33 has an RGB LED. */
  pinMode(LEDR, OUTPUT);
  digitalWrite(LEDR, LOW);
#endif

  // declare source and destination mac addresses for this session
  uint8_t devAddr[] = {0x11, 0x11};
  uint8_t destination[] = {0x22, 0x22};
  UWBMacAddress srcAddr(UWBMacAddress::Size::SHORT, devAddr);
  UWBMacAddress dstAddr(UWBMacAddress::Size::SHORT, destination);

  // register the ranging notification handler before starting
  UWB.registerRangingCallback(rangingHandler);

  // UWB.begin(); //start the UWB stack, use Serial for the log output           // [PHASE0 REMOVED]
  UWB.begin(); // start the UWB stack                                           // [PHASE0 ADDED]
  Serial.println("Starting UWB ...");

  //wait until the stack is initialised
  while(UWB.state()!=0)
    delay(10);

  Serial.println("Starting session ...");
  //setup a session with ID 0x11223344;
  UWBRangingController myController(0x11223344, srcAddr, dstAddr);
  UWBSessionManager.addSession(myController);

  // myController.init();                                                       // [PHASE0 REMOVED]
  // myController.start();                                                      // [PHASE0 REMOVED]

  // ---------------- [PHASE0 ADDED] init -> config -> start ----------------
  // [PHASE0 ADDED] init() must run before appConfig() because session handle is created/validated during init.
  printStatus("init", myController.init());                                     // [PHASE0 ADDED]

  // [PHASE0 ADDED] Ensure ranging notifications are enabled (safe even if defaulted on)
  printStatus("enableRangingDataNtf", myController.enableRangingDataNtf(1));    // [PHASE0 ADDED]

  // [PHASE0 ADDED] Request AoA results (AOA_RESULT_REQ = 0x0D)
  printStatus("set AoaResultReq(0x0D)", myController.appConfig(uwb::AppConfigId::AoaResultReq, kAoaResultReq)); // [PHASE0 ADDED]

  // [PHASE0 ADDED] Enable ToF + AoA az/el + AoA FOM fields in reports (RESULT_REPORT_CONFIG = 0x2E)
  printStatus("set ResultReport(0x2E)", myController.appConfig(uwb::AppConfigId::ResultReport, kResultReportMask)); // [PHASE0 ADDED]

  // [PHASE0 ADDED] start ranging AFTER configs are applied
  printStatus("start", myController.start());                                    // [PHASE0 ADDED]
}

void loop()
{
#if defined(ARDUINO_PORTENTA_C33)
  /* Only the Portenta C33 has an RGB LED. */
  digitalWrite(LEDR, !digitalRead(LEDR));
#endif
  delay(1000);
}
