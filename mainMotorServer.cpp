//
// RESTful stepper-motor server. Controls how much cable a stepper motor has
// unrolled -- e.g. to hang a plant at a dynamically determined height, so its
// position visualises some scalar value (power use, temperature, minutes until
// rain, ...).
//
// Built on the iotsa framework. All tuning -- numMotors, STEPS_PER_MM, the GPIO
// pins for the motor(s) and the zero-detect microswitch(es) -- is compile-time;
// see the constants below and readme.md.
//

#include "iotsa.h"
#include "iotsaWifi.h"
#include "iotsaOta.h"
#include "iotsaLed.h"
#include <AccelStepper.h>

#define NEOPIXEL_PIN 15

IotsaApplication application("Iotsa Stepper Motor Server");
IotsaWifiMod wifiMod(application);
IotsaOtaMod otaMod(application);               // Over-the-air firmware updates
IotsaLedMod ledMod(application, NEOPIXEL_PIN); // Status LED

// Declaration of the Stepper Motor module
class IotsaStepperMod : public IotsaBaseModule {
public:
  IotsaStepperMod(IotsaApplication &_app) : IotsaBaseModule(_app) {}
  void setup() override;
  void lateSetup() override;
  void loop() override;
  String info() override;
private:
  void handleMotorIndex();
  String handleMotorStatus(int num);
  void handleMotor(int num);
};

//
// Steppermotor declarations
//
#define numMotors 1
#define STEPS_PER_MM  (-104) // This depends on spool size, motor factors and more.
#define MOTOR_MAX_SPEED 700 // In steps per second, positive
#define MOTOR_ACCEL 100
#define MOTOR_KEEP_POWER  // Define to keep power on motors when not moving (may help against slipping)
#define STEPPER_TIMEOUT 300000 // 5 minutes max for positioning motor

// Pinout for first motor. Note (below) that the 28BYJ-48 needs pins to be stepped 1-3-2-4.
#define motor1Pin1 4  // GPIO 4 -> ULN2003 IN1 -> 28BYJ-48 blue
#define motor1Pin2 13  // GPIO 13 -> ULN2003 IN2 -> 28BYJ-48 pink
#define motor1Pin3 12  // GPIO 12 -> ULN2003 IN3 -> 28BYJ-48 yellow
#define motor1Pin4 14  // GPIO 14 -> ULN2003 IN4 -> 28BYJ-48 orange
#define motor1ZeroPin 16 // GPIO 16 connected to zeroing input

AccelStepper stepperMot[numMotors]= {
  AccelStepper(AccelStepper::HALF4WIRE, motor1Pin1, motor1Pin3, motor1Pin2, motor1Pin4, true)
};

int zeroDetectPin[numMotors] = {
  motor1ZeroPin
};

long motorLimit[numMotors] = {
  0
};


void
IotsaStepperMod::handleMotorIndex() {
  String message = "{\"numMotors\":";
  message += String(numMotors);
  message += ", \"uptime\":";
  message += String(millis());
  message += "}";
  app.server->send(200, "application/json", message);
}

void IotsaStepperMod::setup() {
  for (int i=0;i<=numMotors-1;i++) {
    stepperMot[i].setMaxSpeed(MOTOR_MAX_SPEED);
    stepperMot[i].setAcceleration(MOTOR_ACCEL);

    if (zeroDetectPin[i]) {
      // This motor has a zero-detection pin. Move motor in negative direction until we
      // detect zero (pin goes to GND), then slowly zero in.
      IotsaSerial.println("pinMode");
      pinMode(zeroDetectPin[i], INPUT_PULLUP);
      IotsaSerial.println("To minus 5 meter");
      stepperMot[i].setSpeed(0);
      stepperMot[i].setCurrentPosition(0);
      stepperMot[i].moveTo(-5000 * STEPS_PER_MM);
      unsigned long startTime = millis();
      IotsaSerial.println("Moving back");
      while(digitalRead(zeroDetectPin[i]) != LOW && millis() < startTime + STEPPER_TIMEOUT) {
        stepperMot[i].run();
        delay(1);
      }
      IotsaSerial.println("To 1 cm");
      stepperMot[i].setSpeed(0);
      stepperMot[i].setCurrentPosition(0);
      stepperMot[i].moveTo(10 * STEPS_PER_MM);
      IotsaSerial.println("Moving forward");
      while(stepperMot[i].distanceToGo() != 0) {
        stepperMot[i].run();
        delay(1);
      }
      IotsaSerial.println("done");
      if (millis() >= startTime + STEPPER_TIMEOUT) {
        IFDEBUG IotsaSerial.println("Motor timeout");
      }
      if (digitalRead(zeroDetectPin[i]) == LOW) {
        IFDEBUG IotsaSerial.println("Zero-switch still engaged");
      }
      stepperMot[i].setSpeed(0);
      stepperMot[i].setCurrentPosition(0);
    } else {
      // No zero-detection input. Go to position zero, whatever that was (probably just where we are now)
      stepperMot[i].setSpeed(0);
      stepperMot[i].moveTo(0);
    }
#ifndef MOTOR_KEEP_POWER
    // Disable outputs
    stepperMot[i].disableOutputs();
#endif
  IFDEBUG IotsaSerial.println("Stepper initialized");
  }
}

String
IotsaStepperMod::handleMotorStatus(int num) {
  String message = "{\"id\":\"";
  message += String(num);
  message += "\", \"pos\":";
  message += String(stepperMot[num].currentPosition() / STEPS_PER_MM);
  message += ", \"target\":";
  message += String(stepperMot[num].targetPosition() / STEPS_PER_MM);
  message += ", \"speed\":";
  message += String(stepperMot[num].speed() / STEPS_PER_MM);
  if (motorLimit[num]) {
    message += ", \"limit\":";
    message += String(motorLimit[num]);
  }
  if (zeroDetectPin[num]) {
    message += ", \"inrange\":";
    message += String(digitalRead(zeroDetectPin[num]));
  }
  message += "}";
  return message;
}

void
IotsaStepperMod::handleMotor(int num) {
  long pos = 0;
  for (uint8_t i=0; i<app.server->args(); i++){
    if( app.server->argName(i) == "pos") {
      pos = atol(app.server->arg(i).c_str());
      if (pos < 0) pos = 0;
      if (motorLimit[num] && pos > motorLimit[num]) pos = motorLimit[num];
      stepperMot[num].moveTo(pos*STEPS_PER_MM);
#ifndef MOTOR_KEEP_POWER
      stepperMot[num].enableOutputs();
#endif
    }
  }
  app.server->send(200, "application/json", handleMotorStatus(num));
}

unsigned long ignoreZeroDetectUntil = 0;

void IotsaStepperMod::loop() {
  if (millis() > ignoreZeroDetectUntil) ignoreZeroDetectUntil = 0;
  for(int i=0; i<numMotors; i++) {
    bool shouldRun = (stepperMot[i].distanceToGo() != 0);
    stepperMot[i].run();
    if (shouldRun ) {
#ifndef MOTOR_KEEP_POWER
      if (stepperMot[i].distanceToGo() == 0) {
        // Disable outputs if we have reached the target
        stepperMot[i].disableOutputs();
      } else
#endif
      if (zeroDetectPin[i] && digitalRead(zeroDetectPin[i]) == LOW && ignoreZeroDetectUntil > 0) {
        // We were moving backward. Back off and re-zero, move out 5 cm.
        IFDEBUG IotsaSerial.println("Zero-detect switch hit. Re-init motor.");
        setup();
      }
    }
  }
}

void IotsaStepperMod::lateSetup() {
  app.server->on("/stepper", std::bind(&IotsaStepperMod::handleMotorIndex, this));
  for (int i=0; i < numMotors; i++) {
    String loc = "/stepper/" + String(i);
    app.server->on(loc.c_str(), std::bind(&IotsaStepperMod::handleMotor, this, i));
  }
}

String IotsaStepperMod::info() {
  // Return some information about this module, for the main page of the web server.
  String rv = "<p>See <a href='/stepper'>/stepper</a> for info";
  for (int i=0; i < numMotors; i++) {
    rv += ", <a href='/stepper/" + String(i) + "'>/stepper/" + String(i) + "</a> for motor " + String(i);
  }
  rv += ".</p>";
  return rv;
}


// Instantiate the module, and install it in the framework
IotsaStepperMod stepperMod(application);

// Standard setup() method, hands off most work to the application framework
void setup(void){
  application.setup();
  application.lateSetup();
#ifndef ESP32
  ESP.wdtEnable(WDTO_120MS);
#endif
}

// Standard loop() routine, hands off most work to the application framework
void loop(void){
  application.loop();
}
