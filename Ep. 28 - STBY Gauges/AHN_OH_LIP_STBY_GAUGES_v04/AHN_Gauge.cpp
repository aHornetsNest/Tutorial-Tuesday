#include "AHN_Gauge.h"

// Constructor implementation
Gauge::Gauge(String name, int steps, float home, float initial, float maxTravel, float minTravel,
             int zeroPin, bool zeroable, int coil1, int coil2, int coil3, int coil4, String motorType) {
  this->name = name;
  this->stepsPerRev = steps;
  this->homePosition = round((home * steps) / 360.0);
  this->initialPosition = round((initial * steps) / 360.0);
  this->maxTravel = round((maxTravel * steps) / 360.0);
  this->minTravel = round((minTravel * steps) / 360.0);
  this->zeroPin = zeroPin;
  this->zeroable = zeroable;
  this->currentPosition = 0;
  this->targetPosition;

  int userPins[4] = { coil1, coil2, coil3, coil4 };
  int reorderedPins[4];

  // Rearrange pins based on motor type
  if (motorType == "BKA-30") {
    reorderedPins[0] = userPins[2];
    reorderedPins[1] = userPins[3];
    reorderedPins[2] = userPins[0];
    reorderedPins[3] = userPins[1];
  } else if (motorType == "VID-29") {
    reorderedPins[0] = userPins[0];
    reorderedPins[1] = userPins[1];
    reorderedPins[2] = userPins[2];
    reorderedPins[3] = userPins[3];
  } else {
    // Default: same as VID-29
    reorderedPins[0] = userPins[0];
    reorderedPins[1] = userPins[1];
    reorderedPins[2] = userPins[2];
    reorderedPins[3] = userPins[3];
  }

  // Save final pins to member vars if you want to access them elsewhere
  pin1 = reorderedPins[0];
  pin2 = reorderedPins[1];
  pin3 = reorderedPins[2];
  pin4 = reorderedPins[3];


  // Create the Stepper instance dynamically
  stepper = new AccelStepper(AccelStepper::FULL4WIRE, pin1, pin2, pin3, pin4, true);
}

// Method to print the gauge data
// void Gauge::printData() {
//   Serial.println("Gauge: " + name);
//   Serial.println("  Steps Per Rev: " + String(stepsPerRev));
//   Serial.println("  Home Position: " + String(homePosition, 2));
//   Serial.println("  Initial Position: " + String(initialPosition, 2));
//   Serial.println("  Max Travel: " + String(maxTravel, 2));
//   Serial.println("  Min Travel: " + String(minTravel, 2));
//   Serial.println("  Zero Pin: " + String(zeroPin));
//   Serial.println("  Zeroable: " + String(zeroable ? "Yes" : "No"));
//   Serial.println("  Motor Pins: " + String(pin1) + ", " + String(pin2) + ", " + String(pin3) + ", " + String(pin4));
//   Serial.println("-----------------------------");
// }

// Initialize stepper motor
void Gauge::initializeStepper() {
  // printData();
  stepper->setMaxSpeed(600);
  stepper->setAcceleration(400);
  pinMode(zeroPin, INPUT_PULLUP);
  // Serial.println(name + " stepper initialized.");
}

// Zeroing method
void Gauge::zero() {
  if (!zeroable) {
    return;  // Not zeroable, exit early
  }

  stepper->setMaxSpeed(300);
  stepper->setAcceleration(200);

  // Move forward slightly to ensure we're off the zero sensor
  stepper->moveTo(stepper->currentPosition() + 40);
  while (stepper->distanceToGo() != 0) {
    stepper->run();
  }

  // Move backward continuously until zero sensor triggers
  stepper->moveTo(stepper->currentPosition() - 800);
  while (digitalRead(zeroPin) == HIGH) {
    stepper->run();
  }

  // Immediately stop movement (no smooth decel)
  stepper->stop();  // this just sets target to current pos
  stepper->setCurrentPosition(homePosition);
  // No waiting for isRunning()

  delay(500);

  // Move forward to clear zero sensor (if needed)
  int offsetSteps = 100;
  stepper->moveTo(stepper->currentPosition() + offsetSteps);
  while (stepper->distanceToGo() != 0) {
    stepper->run();
  }

  delay(500);

  // Move backward again until zero sensor triggers
  stepper->moveTo(stepper->currentPosition() - 800);
  while (digitalRead(zeroPin) == HIGH) {
    stepper->run();
  }

  // Immediate stop again
  stepper->stop();

  delay(50);

  // Set zero position
  stepper->setCurrentPosition(homePosition);
  currentPosition = homePosition;

  // Move to initial position
  int stepsToMove = initialPosition - homePosition;
  if (stepsToMove != 0) {
    stepper->moveTo(stepper->currentPosition() + stepsToMove);
    while (stepper->distanceToGo() != 0) {
      stepper->run();
    }
    currentPosition = initialPosition;
  }
  stepper->setMaxSpeed(500);
  stepper->setAcceleration(250);
}
