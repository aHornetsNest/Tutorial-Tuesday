#include <AccelStepper.h>

// // Stepper motor control pins ASI
// #define motorPin1 48
// #define motorPin2 46
// #define motorPin3 50
// #define motorPin4 52

// Stepper motor control pins ALT
#define motorPin1 38
#define motorPin2 40
#define motorPin3 42
#define motorPin4 44

// // Stepper motor control pins VVI
// #define motorPin1 32
// #define motorPin2 30
// #define motorPin3 34
// #define motorPin4 36

// // ASI Optical sensor pin
// #define zeroSensorPin A0

// ALT Optical sensor pin
#define zeroSensorPin A1

// // VVI Optical sensor pin
// #define zeroSensorPin A2

// Create AccelStepper instance (full 4-wire drive)
AccelStepper stepper(AccelStepper::FULL4WIRE, motorPin1, motorPin2, motorPin3, motorPin4);

// Variables
long stepCount = 0;
bool firstZeroDetected = false;

void setup() {
  Serial.begin(9600);

  // Setup optical sensor pin
  pinMode(zeroSensorPin, INPUT_PULLUP);

  // Set stepper parameters
  stepper.setMaxSpeed(300);
  stepper.setAcceleration(200);

  Serial.println("Calibrating steps per revolution...");
}

void loop() {
  
  // Run the motor one step at a time
  stepper.moveTo(stepper.currentPosition() + 1);
  stepper.runToPosition(); // Blocking move — moves one step

  stepCount++;

  // Check optical sensor state
  int sensorState = digitalRead(zeroSensorPin);

  if (sensorState == LOW && !firstZeroDetected) {
    // First time passing zero point
    firstZeroDetected = true;
    Serial.println("First zero detected. Starting count...");
    stepCount = 0;  // Reset counter at first detection
    delay(50);      // Debounce delay
  }
  else if (sensorState == LOW && firstZeroDetected && stepCount > 5) {
    // Second time hitting zero after stepping
    Serial.print("Full revolution step count: ");
    Serial.println(stepCount);
    Serial.println("Calibration complete.");

    // Stop the motor
    while (true);  // Halt program
  }

  delay(2);  // Tiny delay to avoid overshooting
}
