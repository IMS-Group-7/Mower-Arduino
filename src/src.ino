#include <avr/wdt.h>
#include <MeAuriga.h>
#include "MeEEPROM.h"
#include <Wire.h>
#include <SoftwareSerial.h>

// Global States
enum Mode {MANUAL_MODE, AUTONOMOUS_MODE};
enum MowerState {IDLE, FORWARD, BACKWARD, RIGHT, LEFT};

// Sensor initiation
MeUltrasonicSensor ultraSonic(PORT_10);
MeLineFollower greyScale(PORT_9);
MeGyro gyroMeter(1, 0x69);

// Motor
MeEncoderOnBoard Encoder_1(SLOT1);
MeEncoderOnBoard Encoder_2(SLOT2);
MeEncoderMotor encoders[2];

// Variables - Integers
int16_t moveSpeed   = 110;
int16_t turnSpeed   = 110;
int16_t minSpeed    = 45;


void setMotorPwm(int16_t pwm);

void updateSpeed(void);

// Setup inturrupts for encoders on both motors
// Invert one of them as this so both values when positive means forward
void interrupt_encoder1(void) {
  if (digitalRead(Encoder_1.getPortB()) != 0) {
    Encoder_1.pulsePosMinus();
  } else {
    Encoder_1.pulsePosPlus();
  }
}

void interrupt_encoder2(void) {
  if (digitalRead(Encoder_2.getPortB()) == 0) {
    Encoder_2.pulsePosMinus();
  } else {
    Encoder_2.pulsePosPlus();
  }
}

void setup() {
  Serial.begin(115200);
  gyroMeter.begin();
  

  encoders[0] = MeEncoderMotor(SLOT1);
  encoders[1] = MeEncoderMotor(SLOT2);

  encoders[0].begin();
  encoders[1].begin();

  attachInterrupt(Encoder_1.getIntNum(), interrupt_encoder1, RISING);
  attachInterrupt(Encoder_2.getIntNum(), interrupt_encoder2, RISING);

  wdt_reset();
  encoders[0].runSpeed(0);
  encoders[1].runSpeed(1);

}

void update(void){
  gyroMeter.update();
}

void Forward(void)
{
  Encoder_1.setMotorPwm(-moveSpeed);
  Encoder_2.setMotorPwm(moveSpeed);
}
void Backward(void)
{
  Encoder_1.setMotorPwm(moveSpeed);
  Encoder_2.setMotorPwm(-moveSpeed);
}
void TurnLeft(void)
{
  Encoder_1.setMotorPwm(-moveSpeed);
  Encoder_2.setMotorPwm(-moveSpeed);
}

void TurnRight(void)
{
  Encoder_1.setMotorPwm(moveSpeed);
  Encoder_2.setMotorPwm(moveSpeed);
}

void StopMotor(void){
  Encoder_1.setMotorPwm(0);
  Encoder_2.setMotorPwm(0);
}

double getGyroX(){
  gyroMeter.update();
  return gyroMeter.getGyroX();
}
double getAngleX(){
  gyroMeter.update();
  return gyroMeter.getAngleX();
}
double getGyroY() {
  gyroMeter.update();
  return gyroMeter.getGyroY();
}

double getAngleY() {
  gyroMeter.update();
  return gyroMeter.getAngleY();
}

double getAngleZ() {
  gyroMeter.update();
  return gyroMeter.getAngleZ();
}


void reportOdometry() {
  Encoder_1.updateCurPos();
  Encoder_2.updateCurPos();

  char charValA[20]; sprintf(charValA, ", %08d", Encoder_1.getCurPos());
  char charValB[20]; sprintf(charValB, ", %08d", Encoder_2.getCurPos());
  
  Serial.print(millis());
  Serial.print(charValA);
  Serial.println(charValB);
}

int16_t dist(){ return ultraSonic.distanceCm();}
int16_t lineFlag(){ return greyScale.readSensors();}


Mode currentMode = MANUAL_MODE;
MowerState mowerState = IDLE;

unsigned long previousMillis = 0;

void loop() {

  char cmd;
  static bool waitForRaspberry = false;
  

  if (Serial.available()>0){
      cmd = Serial.read();
      Serial.println("Serial available");
      switch (cmd)
      {
      case 'w':
        if(currentMode==MANUAL_MODE){
          Forward();
        }
        break;
      case 's':
        if(currentMode==MANUAL_MODE){
          Backward();
        }
        break;
      case 'd':
        if(currentMode==MANUAL_MODE){
          TurnRight();
        }
        break;
      case 'a':
        if(currentMode==MANUAL_MODE){
          TurnLeft();
        }
        break;
      case 'x':
        if(currentMode==MANUAL_MODE){
          StopMotor();
        }
        break;
      case 'm':
        currentMode = (currentMode == MANUAL_MODE) ? AUTONOMOUS_MODE : MANUAL_MODE;
        mowerState = IDLE;
        break;
      default:
        Serial.write("Unknown Command ");
        StopMotor();
        break;
      }
    }

    if(currentMode == AUTONOMOUS_MODE){
      
    switch(mowerState) {
      case IDLE:
        mowerState = FORWARD;
        break;

      case FORWARD:
        Forward();
        if(dist() <= 15) {
          StopMotor();
          Serial.println("CAPTURE");
          delay(500);
          mowerState = BACKWARD;
        }
        break;

      case BACKWARD:
        Backward();
        if(dist() > 25){
          StopMotor();
          Serial.println("Obstacle has been avoided");
          mowerState = LEFT;
        }
        break;

      case LEFT:
        Serial.println(previousMillis);
        TurnLeft();
        if(dist() > 200){
          StopMotor();
          mowerState = FORWARD;
        }
        break;

      case RIGHT:
        TurnRight();
        if(dist() > 200){
          StopMotor();
          mowerState = FORWARD;
        }
        break;
    }
  } else {
      if(currentMode == AUTONOMOUS_MODE){
      StopMotor();
      Serial.println("Gets here");
      Serial.println(currentMode);
      }
    }
  //reportOdometry();
  delay(50);
}

  // Use for later

  // if(dist()<=15 && !waitForRaspberry){
  //   StopMotor();
  //   Serial.println("CAPTURE");  
  //   waitForRaspberry = true;
  //   return;
  // } 

  // if(lineFlag()<=0){
  //   StopMotor();
  //   return;
  // }