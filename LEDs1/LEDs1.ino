#include <Adafruit_NeoPixel.h>
#include <math.h>

// -----------------------------
// Pins
// -----------------------------
#define PIN_FLOW   16  // D4
#define PIN_GROSS  15  // D3
#define PIN_PULSE  17  // D5

// -----------------------------
// LED counts
// -----------------------------
#define NUM_FLOW   60
#define NUM_GROSS  60
#define NUM_PULSE  60

Adafruit_NeoPixel flowStrip(NUM_FLOW, PIN_FLOW, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel grossStrip(NUM_GROSS, PIN_GROSS, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pulseStrip(NUM_PULSE, PIN_PULSE, NEO_GRB + NEO_KHZ800);

// Colors
uint32_t hotPink;

// Timing
float grossPhase = 0.0;
float pulsePhase = 0.0;

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

  randomSeed(analogRead(A0));
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
// GROSS FLOOD EFFECT (unchanged)
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
      red += random(20, 45);
      green -= random(5, 20);
    }

    if (red < 0) red = 0;
    if (red > 180) red = 180;

    if (green < 0) green = 0;
    if (green > 180) green = 180;

    grossStrip.setPixelColor(i, grossStrip.Color(red, green, blue));
  }

  grossStrip.show();
}

// -----------------------------
// CLEAN GREEN/YELLOW PULSE
// -----------------------------
void cleanPulseEffect() {
  pulsePhase += 0.02; // slow pulse
  if (pulsePhase > 6.28318) pulsePhase = 0.0;

  float pulse = (sin(pulsePhase) + 1.0) * 0.5;

  for (int i = 0; i < NUM_PULSE; i++) {
    // smooth color between green and yellow
    int green = 80 + (pulse * 150);
    int red   = 20 + (pulse * 100);
    int blue  = 0;

    pulseStrip.setPixelColor(i, pulseStrip.Color(red, green, blue));
  }

  pulseStrip.show();
}

// -----------------------------
// LOOP
// -----------------------------
void loop() {
  for (int i = 0; i < NUM_FLOW; i++) {

    // Pink flow
    fadeAll(20);
    flowStrip.setPixelColor(i, hotPink);

    if (i > 0) flowStrip.setPixelColor(i - 1, flowStrip.Color(180, 10, 100));
    if (i > 1) flowStrip.setPixelColor(i - 2, flowStrip.Color(80, 0, 40));

    flowStrip.show();

    // Other strips run alongside
    grossFloodPulseEffect();
    cleanPulseEffect();

    delay(30);
  }
}