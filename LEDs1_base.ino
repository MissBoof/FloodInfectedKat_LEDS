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

void updateFlowEffect() {
  fadeAll(12);

  flowStrip.setPixelColor(flowIndex, hotPink);

  if (flowIndex > 0) {
    flowStrip.setPixelColor(flowIndex - 1, flowStrip.Color(180, 10, 100));
  }
  if (flowIndex > 1) {
    flowStrip.setPixelColor(flowIndex - 2, flowStrip.Color(80, 0, 40));
  }

  flowStrip.show();

  flowIndex++;
  if (flowIndex >= NUM_FLOW) {
    flowIndex = 0;
  }
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

    // -----------------------------
    // Hot pink shock runners overlay
    // -----------------------------
    for (int r = 0; r < MAX_SHOCK_RUNNERS; r++) {
      if (!shockRunnerActive[r]) continue;

      float dist = fabs(i - shockRunnerPos[r]);
      if (dist < shockRunnerLength[r]) {
        float strength = 1.0 - (dist / shockRunnerLength[r]);
        int pinkAmount = (int)(strength * 255.0);

        red   = max(red,   100 + pinkAmount / 2);
        green = min(green, 12 + pinkAmount / 25);
        blue  = max(blue,  85 + pinkAmount / 2);
      }
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