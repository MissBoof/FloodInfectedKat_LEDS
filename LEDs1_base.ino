#include <Adafruit_NeoPixel.h>
#include <math.h>

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
float pulsePhase = 0.0;
int flowIndex = 0;

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
// SECTIONED GROSS BREATHING
// -----------------------------

void infectedPulseEffect() {
  static bool initialized = false;
  static float ledOffset[NUM_PULSE];
  static float ledDrift[NUM_PULSE];

  // one-time setup
  if (!initialized) {
    for (int i = 0; i < NUM_PULSE; i++) {
      // small random phase offsets so not everything matches
      ledOffset[i] = random(0, 4000) / 1000.0;

      // tiny individual drift so timing slowly evolves
      ledDrift[i] = random(-10, 11) / 1000.0;
    }
    initialized = true;
  }

  // slow master time
  float t = millis() / 8500.0;

  for (int i = 0; i < NUM_PULSE; i++) {
    // nearby LEDs are related, but not identical
    int cluster = i / 5;

    float localT = t
                 + (cluster * 0.22)
                 + ledOffset[i];

    // subtle evolving drift
    localT += sin(millis() / 5000.0 + i * 0.35) * ledDrift[i] * 40.0;

    // wrap
    localT = localT - floor(localT);

    // triangle breath
    float breath;
    if (localT < 0.5) {
      breath = localT * 2.0;
    } else {
      breath = (1.0 - localT) * 2.0;
    }

    // soften curve so it swells more naturally
    float shaped = breath * breath * (1.2 - 0.2 * breath);

    // horror-style wake-up flicker near the low end
    if (breath > 0.03 && breath < 0.16) {
      if (random(0, 100) < 35) {
        shaped *= random(20, 85) / 100.0;
      }
    }

    // make every LED able to get very dim, but not permanently off
    int red   = 2  + (shaped * 120);
    int green = 4  + (shaped * 125);
    int blue  = 0;

    // more rotten orange/red at brighter points
    if (shaped > 0.35) {
      red += (shaped - 0.35) * 130;
    }

    // slight low ember when nearly dark
    if (breath < 0.08) {
      red   = max(1, red / 4);
      green = max(1, green / 6);
    }

    red = constrain(red, 0, 210);
    green = constrain(green, 0, 165);

    pulseStrip.setPixelColor(i, pulseStrip.Color(red, green, blue));
  }

  pulseStrip.show();
}

// -----------------------------
// LOOP
// -----------------------------
void loop() {
  updateFlowEffect();
  grossFloodPulseEffect();
  infectedPulseEffect();

  delay(30);
}