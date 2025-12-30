#include <PortentaUWBShield.h>

/**
 * this demo shows how to setup the Arduino Stella tag as simple
 * Two-Way Ranging Initiator/Controller
 * It expects a counterpart setup as a Responder/Controlee
 *
 * -------------------------------------------------------------------------
 * [PHASE0 NOTE]
 * Same bring-up strategy as controller:
 *  1) Request AoA output via AOA_RESULT_REQ (0x0D)
 *  2) Enable AoA fields in RESULT_REPORT_CONFIG (0x2E)
 *  3) Print AoA fields in DS-TWR measurement struct (twr_mesr)
 */

// ===================== [PHASE0 ADDED] AoA knobs (FiRa/UCI-aligned) =====================
static const uint32_t kAoaResultReq = 1; // [PHASE0 ADDED] 0=disable, 1=enable, 2=az only, 3=el only, 0xF0=interleaved
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

static void printPeerShort(const uint8_t peer_addr[8]) { // [PHASE0 ADDED]
  printHex2(peer_addr[0]);                                // [PHASE0 ADDED]
  printHex2(peer_addr[1]);                                // [PHASE0 ADDED]
}

static void printTwrWithAoa(const uwb::twr_mesr &m) {      // [PHASE0 ADDED]
  Serial.print("peer=");                                   // [PHASE0 ADDED]
  printPeerShort(m.peer_addr);                              // [PHASE0 ADDED]

  Serial.print(" dist=");                                  // [PHASE0 ADDED]
  Serial.print(m.distance);                                // [PHASE0 ADDED]

  Serial.print(" rssi=");                                  // [PHASE0 ADDED]
  Serial.print((int)m.rssi);                               // [PHASE0 ADDED]

  Serial.print(" nlos=");                                  // [PHASE0 ADDED]
  Serial.print((int)m.nlos);                               // [PHASE0 ADDED]

  Serial.print(" az=");                                    // [PHASE0 ADDED]
  Serial.print((int)m.aoa_azimuth);                        // [PHASE0 ADDED]
  Serial.print(" azF=");                                   // [PHASE0 ADDED]
  Serial.print((int)m.aoa_azimuth_fom);                    // [PHASE0 ADDED]

  Serial.print(" el=");                                    // [PHASE0 ADDED]
  Serial.print((int)m.aoa_elevation);                      // [PHASE0 ADDED]
  Serial.print(" elF=");                                   // [PHASE0 ADDED]
  Serial.print((int)m.aoa_elevation_fom);                  // [PHASE0 ADDED]

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

static void printStatus(const char *label, uwb::Status st) { // [PHASE0 ADDED]
  Serial.print(label);                                      // [PHASE0 ADDED]
  Serial.print(": ");                                       // [PHASE0 ADDED]
  Serial.println((int)st);                                  // [PHASE0 ADDED]
}
// =================== END [PHASE0 ADDED] helpers ===================


// handler for ranging notifications
void rangingHandler(UWBRangingData &rangingData) {

  // Serial.print("GOT RANGING DATA - Type: "  );            // [PHASE0 REMOVED]
  // Serial.println(rangingData.measureType());              // [PHASE0 REMOVED]
  Serial.print("GOT RANGING DATA - Type: ");                // [PHASE0 ADDED]
  Serial.println(rangingData.measureType());                // [PHASE0 ADDED]

  // if(rangingData.measureType()==(uint8_t)uwb::MeasurementType::TWO_WAY) // [PHASE0 REMOVED]
  if (rangingData.measureType() == (uint8_t)uwb::MeasurementType::TWO_WAY) // [PHASE0 ADDED]
  {

    // RangingMeasures twr=rangingData.twoWayRangingMeasure(); // [PHASE0 REMOVED]
    RangingMeasures twr = rangingData.twoWayRangingMeasure(); // [PHASE0 ADDED]

    // for(int j=0;j<rangingData.available();j++)             // [PHASE0 REMOVED]
    for (int j = 0; j < rangingData.available(); j++)         // [PHASE0 ADDED]
    {

      // if(twr[j].status==0 && twr[j].distance!=0xFFFF)      // [PHASE0 REMOVED]
      if (twr[j].status == 0 && twr[j].distance != 0xFFFF)    // [PHASE0 ADDED]
      {
        // Serial.print("Distance: ");                        // [PHASE0 REMOVED]
        // Serial.println(twr[j].distance);                   // [PHASE0 REMOVED]
        printTwrWithAoa(twr[j]);                              // [PHASE0 ADDED]
      }
    }

  }

}

void setup() {

  Serial.begin(115200);

#if defined(ARDUINO_PORTENTA_C33)
  /* Only the Portenta C33 has an RGB LED. */
  pinMode(LEDR, OUTPUT);
  digitalWrite(LEDR, LOW);
#endif

  //define the source (this device) and destination MAC addresses, using 2-bytes MACs
  uint8_t devAddr[]={0x22,0x22};
  uint8_t destination[]={0x11,0x11};
  UWBMacAddress srcAddr(UWBMacAddress::Size::SHORT,devAddr);
  UWBMacAddress dstAddr(UWBMacAddress::Size::SHORT,destination);

  // register the ranging notification handler before starting
  UWB.registerRangingCallback(rangingHandler);

  UWB.begin(); //start the UWB stack, use Serial for the log output
  Serial.println("Starting UWB ...");

  //wait until the stack is initialised
  while(UWB.state()!=0)
    delay(10);

  Serial.println("Starting session ...");
  //setup a session with ID 0x11223344
  UWBRangingControlee myControlee(0x11223344, srcAddr, dstAddr);

  //add the session to the session manager, in case you want to manage multiple connections
  UWBSessionManager.addSession(myControlee);

  //prepare the session applying the default parameters
  // myControlee.init();                                      // [PHASE0 REMOVED]
  printStatus("init", myControlee.init());                   // [PHASE0 ADDED]

  // [PHASE0 ADDED] Ensure ranging data notifications are enabled
  printStatus("enableRangingDataNtf", myControlee.enableRangingDataNtf(1)); // [PHASE0 ADDED]

  // [PHASE0 ADDED] Request AoA results + enable AoA fields in the result report
  printStatus("set AoaResultReq(0x0D)", myControlee.appConfig(uwb::AppConfigId::AoaResultReq, kAoaResultReq)); // [PHASE0 ADDED]
  printStatus("set ResultReport(0x2E)", myControlee.appConfig(uwb::AppConfigId::ResultReport, kResultReportMask)); // [PHASE0 ADDED]

  //start the session
  // myControlee.start();                                     // [PHASE0 REMOVED]
  printStatus("start", myControlee.start());                 // [PHASE0 ADDED]
}

void loop() {
#if defined(ARDUINO_PORTENTA_C33)
  /* Only the Portenta C33 has an RGB LED. */
  digitalWrite(LEDR, !digitalRead(LEDR));
#endif
  delay(1000);
}
