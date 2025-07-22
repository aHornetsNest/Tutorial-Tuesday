#include "AHN_Gauge.h"

// ===================================================================================
// External Variables
// ===================================================================================

extern int climbRate;

// ===================================================================================
// Update Target Position for a Gauge
// ===================================================================================

void updateTargetPosition(Gauge* gauge, unsigned int newValue) {
  // Special case: Altimeter with climb rate-based rotation logic
  if (gauge->name == "ALT") {
    int targetPos  = map(newValue, 0, 65535, 0, gauge->stepsPerRev - 1);
    int currentPos = (gauge->stepper->currentPosition() % gauge->stepsPerRev + gauge->stepsPerRev) % gauge->stepsPerRev;

    int cwDistance  = (targetPos - currentPos + gauge->stepsPerRev) % gauge->stepsPerRev;
    int ccwDistance = (currentPos - targetPos + gauge->stepsPerRev) % gauge->stepsPerRev;

    int finalMove;

    if (climbRate >= 0) {
      // Move CW if climbing or level
      finalMove = cwDistance;
    } else {
      // Move CCW if descending
      finalMove = -ccwDistance;
    }

    gauge->stepper->move(finalMove);
  }

  // Standard linear gauge logic (ASI, VVI, etc.)
  else {
    int targetPos    = map(newValue, 0, 65535, gauge->minTravel, gauge->maxTravel);
    int stepsToMove  = targetPos - gauge->currentPosition;

    gauge->targetPosition = gauge->currentPosition + stepsToMove;
    gauge->stepper->moveTo(gauge->targetPosition);
  }
}

// ===================================================================================
// Run and Zero-Check Gauge Stepper
// ===================================================================================

void updateGauge(Gauge* gauge) {
  gauge->stepper->run();

  // Track current position continuously
  gauge->currentPosition = gauge->stepper->currentPosition();

  // Optional zero sensor check
  if (gauge->zeroable) {
    int zeroState = digitalRead(gauge->zeroPin);
    unsigned long now = millis();

    if (zeroState == LOW && (now - gauge->lastZeroTriggerTime > 1000)) {
      // Debounced zero sensor trigger
      gauge->stepper->setCurrentPosition(gauge->homePosition);
      gauge->currentPosition = gauge->stepper->currentPosition();
      gauge->stepper->moveTo(gauge->targetPosition);
      gauge->stepper->run();

      gauge->lastZeroTriggerTime = now;
    }
  }
}
