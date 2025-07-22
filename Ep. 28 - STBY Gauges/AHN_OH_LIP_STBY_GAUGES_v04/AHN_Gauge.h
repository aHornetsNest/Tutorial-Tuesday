#ifndef GAUGE_H
#define GAUGE_H

#include <Arduino.h>
#include <AccelStepper.h>

// ===================================================================================
// Gauge Class Declaration
// ===================================================================================

class Gauge {
public:
  // ---------------------------------------------------------------------------------
  // Gauge Attributes
  // ---------------------------------------------------------------------------------
  String name;
  int stepsPerRev;

  float homePosition;
  float initialPosition;
  float maxTravel;
  float minTravel;

  int zeroPin;
  bool zeroable;

  int currentPosition;
  int targetPosition;
  unsigned int lastNewValue;
  unsigned long lastZeroTriggerTime = 0;

  String motorType;

  // ---------------------------------------------------------------------------------
  // Motor Control Pins and Stepper Pointer
  // ---------------------------------------------------------------------------------
  int pin1, pin2, pin3, pin4;
  AccelStepper* stepper;

  // ---------------------------------------------------------------------------------
  // Constructor
  // ---------------------------------------------------------------------------------
  Gauge(String name, int steps, float home, float initial, float maxTravel, float minTravel,
        int zeroPin, bool zeroable, int pin1, int pin2, int pin3, int pin4, String motorType);

  // ---------------------------------------------------------------------------------
  // Methods
  // ---------------------------------------------------------------------------------
  void printData();
  void zero();
  void initializeStepper();
};

#endif  
