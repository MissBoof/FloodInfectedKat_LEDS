#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <Arduino_LSM6DSOX.h>

// -----------------------------
// Pins
// -----------------------------
#define PIN_GROSS  15   // D4
#define PIN_FLOW   16   // D3
#define PIN_PULSE  21   // D9

// -----------------------------
// LED counts
// -----------------------------
#define NUM_FLOW   60
#define NUM_GROSS  60
#define NUM_PULSE  60

Adafruit_NeoPixel flowStrip(NUM_FLOW, PIN_FLOW, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel grossStrip(NUM_GROSS, PIN_GROSS, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pulseStrip(NUM_PULSE, PIN_PULSE, NEO_GRB + NEO_KHZ800);

// -----------------------------
// Globals
// -----------------------------
uint32_t hotPink;
float grossPhase = 0.0;
int flowIndex = 0;

float flowPhase = 0.0;
int flowTracePos = 0;
int flowTraceDir = 1;

bool flowSupercharge = false;
int flowSuperchargeTimer = 0;
int flowSuperchargeCooldown = 0;

// -----------------------------
// IMU / motion shock detection
// -----------------------------
float lastAX = 0;
float lastAY = 0;
float lastAZ = 0;
bool imuReady = false;

const float JOLT_THRESHOLD = 0.85;

// -----------------------------
// Shock runners (fast hot pink jolts)
// -----------------------------
#define MAX_SHOCK_RUNNERS 5

float shockRunnerPos[MAX_SHOCK_RUNNERS];
float shockRunnerSpeed[MAX_SHOCK_RUNNERS];
int shockRunnerDir[MAX_SHOCK_RUNNERS];
int shockRunnerLife[MAX_SHOCK_RUNNERS];
int shockRunnerLength[MAX_SHOCK_RUNNERS];
bool shockRunnerActive[MAX_SHOCK_RUNNERS];

// -----------------------------
// Setup
// -----------------------------
void setup() {
  flowStrip.begin();
  grossStrip.begin();
  pulseStrip.begin();

  flowStrip.setBrightness(120);
  grossStrip.setBrightness(100);
  pulseStrip.setBrightness(100);

  flowStrip.show();
  grossStrip.show();
  pulseStrip.show();

  hotPink = flowStrip.Color(255, 20, 147);

  randomSeed(micros());

  for (int i = 0; i < MAX_SHOCK_RUNNERS; i++) {
    shockRunnerActive[i] = false;
    shockRunnerPos[i] = 0;
    shockRunnerSpeed[i] = 0;
    shockRunnerDir[i] = 1;
    shockRunnerLife[i] = 0;
    shockRunnerLength[i] = 0;
  }

  if (IMU.begin()) {
    imuReady = true;

    float ax, ay, az;
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(ax, ay, az);
      lastAX = ax;
      lastAY = ay;
      lastAZ = az;
    }
  }
}

// -----------------------------
// FLOW FADE
// -----------------------------
void fadeAll(uint8_t amount) {
  for (int i = 0; i < NUM_FLOW; i++) {
    uint32_t c = flowStrip.getPixelColor(i);

    uint8_t r = (uint8_t)(c >> 16);
    uint8_t g = (uint8_t)(c >> 8);
    uint8_t b = (uint8_t)(c);

    r = (r > amount) ? r - amount : 0;
    g = (g > amount) ? g - amount : 0;
    b = (b > amount) ? b - amount : 0;

    flowStrip.setPixelColor(i, r, g, b);
  }
}

// -----------------------------
//  Hot PINK needles
// -----------------------------
void updateFlowEffect() {
  static bool initialized = false;
  static int needleState[NUM_FLOW];          // 0 = on, 1 = off/failing, 2 = flicker-back
  static unsigned long needleTimer[NUM_FLOW];
  static int needleFlickerCount[NUM_FLOW];

  static int tracePos1 = 0;
  static int tracePos2 = NUM_FLOW / 2;
  static int traceDir1 = 1;
  static int traceDir2 = -1;

  static unsigned long lastTraceMove = 0;
  static unsigned long lastStateUpdate = 0;

  if (!initialized) {
    for (int i = 0; i < NUM_FLOW; i++) {
      needleState[i] = 0;
      needleTimer[i] = 0;
      needleFlickerCount[i] = 0;
    }
    initialized = true;
  }

  unsigned long now = millis();

    bool shockActiveOnFlow = false;
  for (int r = 0; r < MAX_SHOCK_RUNNERS; r++) {
    if (shockRunnerActive[r]) {
      shockActiveOnFlow = true;
      break;
    }
  }

  // -----------------------------
  // Move the brighter pink trace areas
  // -----------------------------
  if (now - lastTraceMove > 55) {
    tracePos1 += traceDir1;
    tracePos2 += traceDir2;

    if (tracePos1 <= 0 || tracePos1 >= NUM_FLOW - 1) traceDir1 *= -1;
    if (tracePos2 <= 0 || tracePos2 >= NUM_FLOW - 1) traceDir2 *= -1;

    lastTraceMove = now;
  }

  // -----------------------------
  // Occasionally make random LEDs fail/off
  // -----------------------------
  if (now - lastStateUpdate > 80) {
    for (int i = 0; i < NUM_FLOW; i++) {
      if (needleState[i] == 0) {
        // occasional dropout
        if (random(0, 1000) < 10) {
          needleState[i] = 1;
          needleTimer[i] = now + random(150, 600);
        }
      } else if (needleState[i] == 1) {
        // when off time ends, go into flicker-back mode
        if (now >= needleTimer[i]) {
          needleState[i] = 2;
          needleFlickerCount[i] = random(3, 7);
          needleTimer[i] = now + random(35, 70);
        }
      } else if (needleState[i] == 2) {
        // haunted-house flicker back on
        if (now >= needleTimer[i]) {
          needleFlickerCount[i]--;

          if (needleFlickerCount[i] <= 0) {
            needleState[i] = 0;
          } else {
            needleTimer[i] = now + random(25, 80);
          }
        }
      }
    }

    lastStateUpdate = now;
  }

  // -----------------------------
  // Draw the strip
  // -----------------------------
  for (int i = 0; i < NUM_FLOW; i++) {
    // base dim hot pink
        int red = 0;
    int green = 0;
    int blue = 0;

    // only draw the normal needle effect when no shock is active
    if (!shockActiveOnFlow) {
      red = 75;
      green = 0;
      blue = 55;

          // slight breathing glow so needles aren't dead static
      float slowGlow = (sin((millis() / 900.0) + i * 0.15) + 1.0) * 0.5;
      red  += slowGlow * 18;
      blue += slowGlow * 14;

      // small infected contamination, but subtle
      if (random(0, 1000) < 6) {
        red   += random(8, 20);
        green += random(4, 14);
        blue  -= random(0, 8);
      }

      // -----------------------------
      // Bright tracing pink areas
      // -----------------------------
      float d1 = fabs(i - tracePos1);
      float d2 = fabs(i - tracePos2);

      if (d1 < 2.5) {
        float strength = 1.0 - (d1 / 2.5);
        red  += strength * 120;
        blue += strength * 95;
      }

      if (d2 < 2.0) {
        float strength = 1.0 - (d2 / 2.0);
        red  += strength * 105;
        blue += strength * 85;
      }

      // tiny infected fringe around traces
      if ((d1 < 4.0 || d2 < 3.5) && random(0, 100) < 12) {
        red   += random(5, 15);
        green += random(3, 10);
      }

      // -----------------------------
      // Broken / off / flicker-back behavior
      // -----------------------------
      if (needleState[i] == 1) {
        red = red / 10;
        green = 0;
        blue = blue / 10;
      } else if (needleState[i] == 2) {
        bool onThisBeat = random(0, 100) < 55;
        if (!onThisBeat) {
          red = red / 12;
          green = 0;
          blue = blue / 12;
        } else {
          red += 25;
          blue += 18;
        }
      }
    }

    red = constrain(red, 0, 255);
    green = constrain(green, 0, 45);
    blue = constrain(blue, 0, 255);

    flowStrip.setPixelColor(i, flowStrip.Color(red, green, blue));
  }

  // keep your accelerometer pink runners on top
  applyShockRunnersToStrip(flowStrip, NUM_FLOW, true);

  flowStrip.show();
}

// -----------------------------
// GROSS FLOOD EFFECT
// -----------------------------
void grossFloodPulseEffect() {
  grossPhase += 0.05;
  if (grossPhase > 6.28318) grossPhase = 0.0;

  float pulse = (sin(grossPhase) + 1.0) * 0.5;

  for (int i = 0; i < NUM_GROSS; i++) {
    float localPhase = grossPhase + (i * 0.22);
    float localPulse = (sin(localPhase) + 1.0) * 0.5;

    int wobble = random(-18, 19);

    int red   = 25 + (pulse * 55) + ((1.0 - localPulse) * 45) + wobble;
    int green = 35 + (pulse * 110) + (localPulse * 45) + wobble;
    int blue  = random(0, 8);

    if (random(0, 100) < 18) {
      green -= random(10, 35);
      red   += random(5, 20);
    }

    if (random(0, 100) < 12) {
      red   += random(15, 35);
      green += random(10, 30);
    }

    if (random(0, 100) < 10) {
      red   += random(20, 45);
      green -= random(5, 20);
    }

    red = constrain(red, 0, 180);
    green = constrain(green, 0, 180);
    blue = constrain(blue, 0, 20);

    grossStrip.setPixelColor(i, grossStrip.Color(red, green, blue));
  }

  grossStrip.show();
}

// -----------------------------
// Spawn fast hot pink runners
// -----------------------------
void spawnShockRunners(float strength) {
  int howMany = 2 + (int)(strength * 3.0);
  if (howMany > MAX_SHOCK_RUNNERS) howMany = MAX_SHOCK_RUNNERS;

  for (int n = 0; n < howMany; n++) {
    for (int i = 0; i < MAX_SHOCK_RUNNERS; i++) {
      if (!shockRunnerActive[i]) {
        shockRunnerActive[i] = true;
        shockRunnerPos[i] = random(0, NUM_PULSE);
        shockRunnerDir[i] = (random(0, 100) < 50) ? 1 : -1;
        shockRunnerSpeed[i] = random(220, 420) / 100.0; // 2.2 to 4.2 LEDs/frame
        shockRunnerLife[i] = random(2, 5);              // brief
        shockRunnerLength[i] = random(1, 4);            // short streak
        break;
      }
    }
  }
}

// -----------------------------
// IMU SHOCK DETECTION
// -----------------------------
void updateMotionShock() {
  if (!imuReady) return;
  if (!IMU.accelerationAvailable()) return;

  float ax, ay, az;
  IMU.readAcceleration(ax, ay, az);

  float dx = ax - lastAX;
  float dy = ay - lastAY;
  float dz = az - lastAZ;

  float delta = sqrt(dx * dx + dy * dy + dz * dz);

  if (delta > JOLT_THRESHOLD) {
    float shockStrength = delta;
    if (shockStrength > 2.5) shockStrength = 2.5;
    shockStrength = shockStrength / 2.5;

    spawnShockRunners(shockStrength);
  }

  lastAX = ax;
  lastAY = ay;
  lastAZ = az;
}

// -----------------------------
// Helper for LEDS to turn PINK on all LED strips
// -----------------------------
void applyShockRunnersToStrip(Adafruit_NeoPixel &strip, int numPixels, bool suppressGreen) {
  for (int i = 0; i < numPixels; i++) {
    uint32_t existing = strip.getPixelColor(i);

    int red   = (uint8_t)(existing >> 16);
    int green = (uint8_t)(existing >> 8);
    int blue  = (uint8_t)(existing);

    for (int r = 0; r < MAX_SHOCK_RUNNERS; r++) {
      if (!shockRunnerActive[r]) continue;

      float dist = fabs(i - shockRunnerPos[r]);
      if (dist < shockRunnerLength[r]) {
        float strength = 1.0 - (dist / shockRunnerLength[r]);
        int pinkAmount = (int)(strength * 255.0);

        red = max(red, 100 + pinkAmount / 2);
        blue = max(blue, 85 + pinkAmount / 2);

        if (suppressGreen) {
          green = min(green, 12 + pinkAmount / 25);
        } else {
          green = min(255, green + pinkAmount / 18);
        }
      }
    }

    red = constrain(red, 0, 255);
    green = constrain(green, 0, 255);
    blue = constrain(blue, 0, 255);

    strip.setPixelColor(i, strip.Color(red, green, blue));
  }
}


// -----------------------------
// ORGANIC INFECTED BREATHING
// -----------------------------
void infectedPulseEffect() {
  static bool initialized = false;
  static float ledOffset[NUM_PULSE];
  static float ledDrift[NUM_PULSE];

  if (!initialized) {
    for (int i = 0; i < NUM_PULSE; i++) {
      ledOffset[i] = random(0, 4000) / 1000.0;
      ledDrift[i] = random(-10, 11) / 1000.0;
    }
    initialized = true;
  }

  float t = millis() / 6500.0;

  for (int i = 0; i < NUM_PULSE; i++) {
    int cluster = i / 3;

    float localT = t + (cluster * 0.22) + ledOffset[i];
    localT += sin(millis() / 5000.0 + i * 0.35) * ledDrift[i] * 40.0;
    localT = localT - floor(localT);

    float breath;
    if (localT < 0.5) {
      breath = localT * 2.0;
    } else {
      breath = (1.0 - localT) * 2.0;
    }

    float shaped = breath * breath * (1.2 - 0.2 * breath);

    if (breath > 0.03 && breath < 0.16) {
      if (random(0, 100) < 35) {
        shaped *= random(20, 85) / 100.0;
      }
    }

    int red   = 2  + (shaped * 120);
    int green = 4  + (shaped * 125);
    int blue  = 0;

    if (shaped > 0.35) {
      red += (shaped - 0.35) * 130;
    }

    if (breath < 0.08) {
      red   = max(1, red / 4);
      green = max(1, green / 6);
    }

    red = constrain(red, 0, 255);
    green = constrain(green, 0, 165);
    blue = constrain(blue, 0, 255);

    pulseStrip.setPixelColor(i, pulseStrip.Color(red, green, blue));
  }

  // move shock runners after drawing
  for (int r = 0; r < MAX_SHOCK_RUNNERS; r++) {
    if (!shockRunnerActive[r]) continue;

    shockRunnerPos[r] += shockRunnerSpeed[r] * shockRunnerDir[r];
    shockRunnerLife[r]--;

    if (shockRunnerLife[r] <= 0 || shockRunnerPos[r] < 0 || shockRunnerPos[r] >= NUM_PULSE) {
      shockRunnerActive[r] = false;
    }
  }
  applyShockRunnersToStrip(pulseStrip, NUM_PULSE, true);
  pulseStrip.show();
}

// -----------------------------
// LOOP
// -----------------------------
void loop() {
  updateMotionShock();
  updateFlowEffect();
  grossFloodPulseEffect();
  infectedPulseEffect();

  delay(30);
}