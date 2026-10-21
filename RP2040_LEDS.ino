
#include <Arduino.h>
#include <Arduino_LSM6DSOX.h>
#include <math.h>

// Physical Nano pin numbers
#define MOTION_OUT 3
#define BUTTON_PIN 4
#define TRIGGER_OUT 5

const float motionThreshold = 0.12;
const unsigned long stillnessDelay = 2000;

// Special transformation settings
const unsigned long autoStillTime = 10000;
const unsigned long autoCooldown = 600000;
const unsigned long triggerPulseTime = 300;
const unsigned long debounceTime = 40;

// Motion tracking
float lastX = 0, lastY = 0, lastZ = 0;
float motionAmount = 0;

bool firstReading = true;
bool isMoving = false;
bool hasMoved = false;
bool autoArmed = false;

// Automatic cooldown
bool hasAutoTriggered = false;
unsigned long lastAutoTrigger = 0;

// Manual button debounce
bool lastRawButton = HIGH;
bool stableButton = HIGH;
unsigned long lastButtonChange = 0;

// Trigger pulse
bool triggerActive = false;
unsigned long triggerStart = 0;

unsigned long lastMovement = 0;
unsigned long lastPrint = 0;

void startTransformation(const char* reason) {
  if (triggerActive) return;

  triggerActive = true;
  triggerStart = millis();

  digitalWrite(TRIGGER_OUT, HIGH);

  Serial.print("TRANSFORMATION: ");
  Serial.println(reason);
}

void checkButton() {
  bool rawButton = digitalRead(BUTTON_PIN);

  if (rawButton != lastRawButton) {
    lastButtonChange = millis();
    lastRawButton = rawButton;
  }

  if (millis() - lastButtonChange >= debounceTime) {
    if (rawButton != stableButton) {
      stableButton = rawButton;

      // Button pressed
      if (stableButton == LOW) {
        startTransformation("MANUAL BUTTON");
      }
    }
  }
}

void checkMovement() {
  if (IMU.accelerationAvailable()) {
    float x, y, z;
    IMU.readAcceleration(x, y, z);

    if (!firstReading) {
      float dx = x - lastX;
      float dy = y - lastY;
      float dz = z - lastZ;

      motionAmount = sqrt(
        dx * dx + dy * dy + dz * dz
      );

      if (motionAmount > motionThreshold) {
        lastMovement = millis();
        hasMoved = true;
      }
    }

    lastX = x;
    lastY = y;
    lastZ = z;
    firstReading = false;
  }

  bool newState =
    hasMoved &&
    (millis() - lastMovement < stillnessDelay);

  if (newState != isMoving) {
    isMoving = newState;

    digitalWrite(
      MOTION_OUT,
      isMoving ? HIGH : LOW
    );

    if (isMoving) {
      // Moving rearms the next stop
      autoArmed = true;
      Serial.println("SENDING: MOVING");
    } else {
      Serial.println("SENDING: STILL");
    }
  }
}

void checkAutomaticTrigger() {
  if (!autoArmed || isMoving) return;
  if (triggerActive) return;

  // Must have been still for 10 seconds
  if (!hasMoved ||
      millis() - lastMovement < autoStillTime) {
    return;
  }

  // Only cooldown automatic triggers
  if (hasAutoTriggered &&
      millis() - lastAutoTrigger < autoCooldown) {
    return;
  }

  autoArmed = false;
  hasAutoTriggered = true;
  lastAutoTrigger = millis();

  startTransformation("AUTO - 10 SEC STILL");
}

void setup() {
  Serial.begin(115200);

  pinMode(MOTION_OUT, OUTPUT);
  pinMode(TRIGGER_OUT, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(MOTION_OUT, LOW);
  digitalWrite(TRIGGER_OUT, LOW);

  if (!IMU.begin()) {
    Serial.println("Accelerometer failed!");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("Flood Kat trigger controller ready!");
}

void loop() {
  checkMovement();
  checkButton();

  // End 300ms trigger pulse
  if (triggerActive &&
      millis() - triggerStart >= triggerPulseTime) {
    digitalWrite(TRIGGER_OUT, LOW);
    triggerActive = false;
  }

  checkAutomaticTrigger();

  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();

    Serial.print("UPTIME: ");
    Serial.print(millis() / 1000);

    Serial.print(" | MOTION: ");
    Serial.print(motionAmount, 3);

    Serial.print(" | STATE: ");
    Serial.println(isMoving ? "MOVING" : "STILL");
  }
}
