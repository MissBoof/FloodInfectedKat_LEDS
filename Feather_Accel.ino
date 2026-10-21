
#include <Adafruit_NeoPixel.h>
#include <math.h>

// -----------------------------
// ESP32-S3 FEATHER PINS
// -----------------------------
#define PIN_FLOW   A2  // GPIO 16
#define PIN_GROSS  A3  // GPIO 15
#define PIN_PULSE  A1  // GPIO 17
#define MOTION_INPUT 5  // Feather D5
#define TRANSFORM_INPUT 6  // Feather D6

bool lastTriggerSignal = false;
bool transformationActive = false;

unsigned long lastTransformationStart = 0;
unsigned long lastTransformationFrame = 0;

const unsigned long transformationDuration = 20000;

// Adjustable audio synchronization points
const unsigned long GUNSHOT_TIME = 2700;
const unsigned long CHAOS_TIME = 3000;
const unsigned long HEADACHE_TIME = 7000;
const unsigned long ZOMBIE_TIME = 13200;
const unsigned long CORRUPTION_TIME = 16000;

int lastTransformationStage = -1;

// -----------------------------
// LED COUNTS
// -----------------------------
#define NUM_FLOW   60
#define NUM_GROSS  60
#define NUM_PULSE  60

Adafruit_NeoPixel flowStrip(
  NUM_FLOW, PIN_FLOW, NEO_GRB + NEO_KHZ800
);

Adafruit_NeoPixel grossStrip(
  NUM_GROSS, PIN_GROSS, NEO_GRB + NEO_KHZ800
);

Adafruit_NeoPixel pulseStrip(
  NUM_PULSE, PIN_PULSE, NEO_GRB + NEO_KHZ800
);



bool nanoMoving = false;
unsigned long lastMotionReport = 0;

// Animation timing
float grossPhase = 0.0;
float pulsePhase = 0.0;


float infectionPhase = 0.0;
float infectionBreath = 0.0;

float infectionCenters[3] = {8, 30, 52};
float infectionSpeeds[3] = {0.07, -0.05, 0.09};

float surgePosition = -20;
bool surgeActive = false;

unsigned long nextSurge = 0;
unsigned long lastInfectionFrame = 0;

const unsigned long infectionFrameInterval = 35;


// -----------------------------
// SETUP
// -----------------------------
void setup() {

  Serial.begin(115200);

  pinMode(MOTION_INPUT, INPUT_PULLDOWN);
  pinMode(TRANSFORM_INPUT, INPUT_PULLDOWN);

  Serial.println("Waiting for Nano motion signal...");

  flowStrip.begin();
  grossStrip.begin();
  pulseStrip.begin();

  flowStrip.setBrightness(120);
  grossStrip.setBrightness(100);
  pulseStrip.setBrightness(100);

  flowStrip.clear();
  grossStrip.clear();
  pulseStrip.clear();

  flowStrip.show();
  grossStrip.show();
  pulseStrip.show();

  randomSeed(analogRead(A0));

  Serial.println("Flood Kat LED Controller Ready!");
  Serial.println("ESP32-S3 Feather");
  Serial.println("All three effects active!");
}


void checkNanoMovement() {

  bool newState = digitalRead(MOTION_INPUT);

  if (newState != nanoMoving) {
    nanoMoving = newState;

    if (nanoMoving) {
      Serial.println("NANO SAYS: MOVING!");
    } else {
      Serial.println("NANO SAYS: STILL!");
    }
  }

  // Report current state every 2 seconds
  if (millis() - lastMotionReport >= 2000) {
    lastMotionReport = millis();

    Serial.print("Feather uptime: ");
    Serial.print(millis() / 1000);

    Serial.print(" | Nano: ");
    Serial.println(
      nanoMoving ? "MOVING" : "STILL"
    );
  }
}



void checkTransformationTrigger() {

  bool signal = digitalRead(TRANSFORM_INPUT);

  // Only react to a NEW trigger pulse
  if (signal && !lastTriggerSignal) {

    if (!transformationActive) {

      transformationActive = true;
      lastTransformationStart = millis();
      lastTransformationFrame = 0;
      lastTransformationStage = -1;

      // Immediately turn everything off
      flowStrip.clear();
      grossStrip.clear();
      pulseStrip.clear();

      flowStrip.show();
      grossStrip.show();
      pulseStrip.show();

      Serial.println("======================");
      Serial.println("TRANSFORMATION STARTED!");
      Serial.println("======================");
    }
  }

  lastTriggerSignal = signal;

  if (transformationActive) {

    if (millis() - lastTransformationStart >=
        transformationDuration) {

      transformationActive = false;

      Serial.println("TRANSFORMATION FINISHED!");
      Serial.println("Returning to normal Flood effects");
    }
  }
}


// -----------------------------
// PINK FLOW FADE
// -----------------------------

void needlerInfectionEffect() {

  // Animation frame timing
  if (millis() - lastInfectionFrame <
      infectionFrameInterval) {
    return;
  }

  lastInfectionFrame = millis();

  // --------------------------------
  // MOVEMENT-BASED SETTINGS
  // --------------------------------

  // Moving = faster breathing
  // Still = slow, eerie breathing
  float breathSpeed = nanoMoving ? 0.035 : 0.008;

  // Moving = infection spreads faster
  float spreadSpeed = nanoMoving ? 1.0 : 0.15;

  // Moving = brighter infection
  float brightness = nanoMoving ? 1.0 : 0.25;

  // --------------------------------
  // BREATHING
  // --------------------------------

  infectionBreath += breathSpeed;

  if (infectionBreath > 6.28318)
    infectionBreath -= 6.28318;

  float breath =
    (sin(infectionBreath) + 1.0) * 0.5;

  // --------------------------------
  // SPREADING INFECTION
  // --------------------------------

  for (int j = 0; j < 3; j++) {

    infectionCenters[j] +=
      infectionSpeeds[j] * spreadSpeed;

    if (infectionCenters[j] >= NUM_FLOW ||
        infectionCenters[j] < 0) {

      infectionSpeeds[j] *= -1;

      infectionCenters[j] = constrain(
        infectionCenters[j],
        0,
        NUM_FLOW - 1
      );
    }
  }

  // --------------------------------
  // BRIGHT ENERGY SURGES
  // Only when moving
  // --------------------------------

  if (nanoMoving &&
      !surgeActive &&
      millis() >= nextSurge) {

    surgeActive = true;
    surgePosition = -8;
  }

  // Cancel surge when Kat stops
  if (!nanoMoving) {
    surgeActive = false;
  }

  // --------------------------------
  // DRAW LED COLORS
  // --------------------------------

  for (int i = 0; i < NUM_FLOW; i++) {

    // Dark purple background
    float intensity = 0.10 + breath * 0.15;

    // Living infection pockets
    for (int j = 0; j < 3; j++) {

      float distance =
        abs(i - infectionCenters[j]);

      float spread =
        exp(-(distance * distance) / 60.0);

      intensity +=
        spread * (0.25 + breath * 0.30);
    }

    intensity = constrain(
      intensity, 0.0, 1.0
    );

    // Pink/purple Needler colors
    int red = 65 + intensity * 170;
    int green = 0;
    int blue = 75 + intensity * 115;

    // Organic flickering
    int flicker = nanoMoving ?
      random(-12, 13) :
      random(-3, 4);

    red += flicker;
    blue += flicker / 2;

    // Occasional bright traveling surge
    if (surgeActive) {

      float d = abs(i - surgePosition);

      if (d < 6) {

        float surgeStrength =
          1.0 - d / 6.0;

        red += surgeStrength * 90;
        blue += surgeStrength * 50;
        green += surgeStrength * 25;
      }
    }

    // Dim everything while still
    red *= brightness;
    green *= brightness;
    blue *= brightness;

    red = constrain(red, 0, 255);
    green = constrain(green, 0, 255);
    blue = constrain(blue, 0, 255);

    flowStrip.setPixelColor(
      i,
      flowStrip.Color(red, green, blue)
    );
  }

  flowStrip.show();

  // --------------------------------
  // ADVANCE ENERGY SURGE
  // --------------------------------

  if (surgeActive) {

    surgePosition += 1.4;

    if (surgePosition > NUM_FLOW + 8) {

      surgeActive = false;

      nextSurge =
        millis() + random(5000, 13000);
    }
  }
}

// -----------------------------
// GROSS FLOOD EFFECT
// -----------------------------
void grossFloodPulseEffect() {

  grossPhase += 0.05;

  if (grossPhase > 6.28318)
    grossPhase = 0.0;

  float pulse = (sin(grossPhase) + 1.0) * 0.5;

  for (int i = 0; i < NUM_GROSS; i++) {

    float localPhase = grossPhase + (i * 0.22);

    float localPulse =
      (sin(localPhase) + 1.0) * 0.5;

    int wobble = random(-18, 19);

    int red =
      25 + (pulse * 55) +
      ((1.0 - localPulse) * 45) + wobble;

    int green =
      35 + (pulse * 110) +
      (localPulse * 45) + wobble;

    int blue = random(0, 8);

    if (random(0, 100) < 18) {
      green -= random(10, 35);
      red += random(5, 20);
    }

    if (random(0, 100) < 12) {
      red += random(15, 35);
      green += random(10, 30);
    }

    if (random(0, 100) < 10) {
      red += random(20, 45);
      green -= random(5, 20);
    }

    red = constrain(red, 0, 180);
    green = constrain(green, 0, 180);

    grossStrip.setPixelColor(
      i,
      grossStrip.Color(red, green, blue)
    );
  }

  grossStrip.show();
}

// -----------------------------
// CLEAN GREEN/YELLOW PULSE
// -----------------------------
void cleanPulseEffect() {

  pulsePhase += 0.02;

  if (pulsePhase > 6.28318)
    pulsePhase = 0.0;

  float pulse = (sin(pulsePhase) + 1.0) * 0.5;

  for (int i = 0; i < NUM_PULSE; i++) {

    int green = 80 + (pulse * 150);
    int red   = 20 + (pulse * 100);
    int blue  = 0;

    pulseStrip.setPixelColor(
      i,
      pulseStrip.Color(red, green, blue)
    );
  }

  pulseStrip.show();
}

// -----------------------------
// ANGRY OVERRIDE - PINK
// -----------------------------
void transformationLightShow() {

  if (!transformationActive) return;

  unsigned long elapsed =
    millis() - lastTransformationStart;

  // Limit LED updates
  if (millis() - lastTransformationFrame < 30)
    return;

  lastTransformationFrame = millis();

  // Determine the current stage
  int stage;

  if (elapsed < GUNSHOT_TIME) {
    stage = 0; // Darkness
  }
  else if (elapsed < CHAOS_TIME) {
    stage = 1; // Gunshot flash
  }
  else if (elapsed < HEADACHE_TIME) {
    stage = 2; // Violent infection
  }
  else if (elapsed < ZOMBIE_TIME) {
    stage = 3; // Purple breathing
  }
  else if (elapsed < CORRUPTION_TIME) {
    stage = 4; // New Zombie
  }
  else {
    stage = 5; // Pink and green corruption
  }

  if (stage != lastTransformationStage) {
    lastTransformationStage = stage;

    Serial.print("TRANSFORMATION STAGE: ");
    Serial.println(stage);
  }

  // Animation time in seconds
  float t = elapsed / 1000.0;

  for (int stripIndex = 0; stripIndex < 3;
       stripIndex++) {

    Adafruit_NeoPixel* strip;

    if (stripIndex == 0) strip = &flowStrip;
    else if (stripIndex == 1) strip = &grossStrip;
    else strip = &pulseStrip;

    int count = strip->numPixels();

    for (int i = 0; i < count; i++) {

      int red = 0;
      int green = 0;
      int blue = 0;

      switch (stage) {

        // -------------------------
        // 0: TOTAL DARKNESS
        // -------------------------
        case 0:
          red = 0;
          green = 0;
          blue = 0;
          break;

        // -------------------------
        // 1: GUNSHOT EXPLOSION
        // -------------------------
        case 1:
          red = 255;
          green = 35;
          blue = 165;
          break;

        // -------------------------
        // 2: VIOLENT PINK INFECTION
        // -------------------------
        case 2: {

          float wave =
            (sin(t * 19.0 + i * 0.5) + 1.0)
            * 0.5;

          int flicker = random(-65, 66);

          red = 75 + wave * 175 + flicker;
          green = random(0, 16);
          blue = 75 + wave * 145 + flicker / 2;

          // Sudden infection bursts
          if (random(100) < 9) {
            red = 255;
            green = 60;
            blue = 190;
          }

          break;
        }

        // -------------------------
        // 3: EERIE PURPLE BREATHING
        // -------------------------
        case 3: {

          float breath =
            (sin(t * 2.5 + i * 0.10) + 1.0)
            * 0.5;

          red = 15 + breath * 75;
          green = 0;
          blue = 25 + breath * 105;

          break;
        }

        // -------------------------
        // 4: NEW ZOMBIE - GREEN
        // -------------------------
        case 4: {

          float wave =
            (sin(t * 13.0 + i * 0.32) + 1.0)
            * 0.5;

          red = 0;
          green = 105 + wave * 150;
          blue = 5 + wave * 30;

          // Unstable green flashes
          if (random(100) < 12) {
            green = 255;
            red = 75;
          }

          break;
        }

        // -------------------------
        // 5: PINK WITH GREEN CORRUPTION
        // -------------------------
        case 5: {

          float breath =
            (sin(t * 3.0 + i * 0.15) + 1.0)
            * 0.5;

          red = 65 + breath * 140;
          green = 0;
          blue = 75 + breath * 110;

          // Crawling green infection clusters
          float greenWave =
            sin(i * 0.25 - t * 3.0
                + stripIndex * 1.7);

          if (greenWave > 0.65) {
            red = 10;
            green = 110 + breath * 115;
            blue = 15;
          }

          // Occasional green outbreaks
          if (random(1000) < 8) {
            red = 15;
            green = 255;
            blue = 20;
          }

          break;
        }
      }

      red = constrain(red, 0, 255);
      green = constrain(green, 0, 255);
      blue = constrain(blue, 0, 255);

      strip->setPixelColor(
        i,
        strip->Color(red, green, blue)
      );
    }

    strip->show();
  }
}


// -----------------------------
// MAIN LOOP
// -----------------------------


void loop() {

  // Always listen to the Nano
  checkNanoMovement();
  checkTransformationTrigger();

  // Cinematic transformation overrides
  // all three regular animations
  if (transformationActive) {

    transformationLightShow();

  } else {

    // Normal Flood Kat effects
    needlerInfectionEffect();
    grossFloodPulseEffect();
    cleanPulseEffect();
  }

  delay(30);
}

