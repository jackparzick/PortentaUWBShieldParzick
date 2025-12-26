#include <PortentaUWBShieldParzick.h>

static const uint32_t RUN_MS  = 15000;
static const uint32_t IDLE_MS =  5000;
static const uint32_t INIT_GAP_MS = 200;

#define ENABLE_RECOVERY_RESET 1

// Tag2: 0x4444 -> Anchor2: 0x3333, session 0x222222, preamble 11
static uint8_t kTagAddr[]    = {0x44, 0x44};
static uint8_t kAnchorAddr[] = {0x33, 0x33};

static UWBMacAddress gTagMac   (UWBMacAddress::Size::SHORT, kTagAddr);
static UWBMacAddress gAnchorMac(UWBMacAddress::Size::SHORT, kAnchorAddr);

// IMPORTANT: Tag2 preamble must match anchor Session 2 preamble (11)
static UWBMultiSessionTag gSess(0x222222, gTagMac, gAnchorMac, 11);

static uint32_t gHdl = 0;
static volatile uint32_t gCount = 0;

enum Phase { PH_START, PH_RUN, PH_STOP, PH_DEINIT, PH_IDLE };
static Phase gPhase = PH_START;
static uint32_t gPhaseT0 = 0;
static uint32_t gCycle = 0;

static void phaseSet(Phase p) { gPhase = p; gPhaseT0 = millis(); }

static void recoveryReset(const char* why) {
#if ENABLE_RECOVERY_RESET
  Serial.print("RECOVERY reset: "); Serial.println(why);
  (void)UWBHAL.reset();
  delay(200);
#endif
}

void rangingHandler(UWBRangingData &d) {
  if (d.measureType() != (uint8_t)uwb::MeasurementType::TWO_WAY) return;
  if (d.sessionHandle() == gHdl) gCount += d.available();
}

void setup() {
  Serial.begin(115200);
#if defined(ARDUINO_PORTENTA_C33)
  pinMode(LEDR, OUTPUT);
  digitalWrite(LEDR, LOW);
#endif

  Serial.println("UWB MultiSession TAG2 STRESS starting...");
  UWB.registerRangingCallback(rangingHandler);

  UWB.begin();
  delay(INIT_GAP_MS);
  phaseSet(PH_START);
}

void loop() {
#if defined(ARDUINO_PORTENTA_C33)
  digitalWrite(LEDR, (millis() / (gPhase == PH_RUN ? 150 : 600)) % 2);
#endif

  switch (gPhase) {
    case PH_START: {
      Serial.print("\\n=== TAG2 CYCLE "); Serial.print(gCycle); Serial.println(" START ===");
      gCount = 0;

      uwb::Status st = gSess.init();
      if (st != uwb::Status::SUCCESS) {
        Serial.print("init failed: "); Serial.println((int)st);
        recoveryReset("init failed");
        UWB.end(); delay(200); UWB.begin(); delay(INIT_GAP_MS);
        phaseSet(PH_IDLE);
        break;
      }
      gHdl = gSess.sessionHandle();
      Serial.print("handle=0x"); Serial.println(gHdl, HEX);

      st = gSess.start();
      if (st != uwb::Status::SUCCESS) {
        Serial.print("start failed: "); Serial.println((int)st);
        recoveryReset("start failed");
        UWB.end(); delay(200); UWB.begin(); delay(INIT_GAP_MS);
        phaseSet(PH_IDLE);
        break;
      }

      phaseSet(PH_RUN);
      break;
    }

    case PH_RUN:
      if (millis() - gPhaseT0 >= RUN_MS) {
        Serial.print("Cycle "); Serial.print(gCycle);
        Serial.print(" count="); Serial.println((uint32_t)gCount);
        phaseSet(PH_STOP);
      }
      break;

    case PH_STOP:
      Serial.print("stop -> "); Serial.println((int)gSess.stop());
      phaseSet(PH_DEINIT);
      break;

    case PH_DEINIT:
      Serial.print("deInit -> "); Serial.println((int)gSess.deInit());
      gSess.sessionHandle(0);
      phaseSet(PH_IDLE);
      break;

    case PH_IDLE:
      if (millis() - gPhaseT0 >= IDLE_MS) {
        gCycle++;
        phaseSet(PH_START);
      }
      break;
  }

  delay(5);
}
