/* Eggduino-Firmware by Joachim Cerny, 2014

   Thanks for the nice libs ACCELSTEPPER and SERIALCOMMAND, which made this project much easier.
   Thanks to the Eggbot-Team for such a funny and enjoable concept!
   Thanks to my wife and my daughter for their patience. :-)
   Thanks to NEBARMIX from GITHUB for the lean SERVORATE method.

*/

// implemented Eggbot-Protocol-Version v13
// EBB-Command-Reference, I sourced from: http://www.schmalzhaus.com/EBB/EBBCommands.html
// no homing sequence, switch-on position of pen will be taken as reference point.
// No collision-detection!!
// Supported Servos: I do not know, I use Arduino Servo Lib with TG9e- standard servo.
// Note: Maximum-Speed in Inkscape is 1000 Steps/s. You could enter more, but then Pythonscript sends nonsense.
// EBB-Coordinates are coming in for 16th-Microstepmode. The Coordinate-Transforms are done in weired integer-math. Be careful, when you diecide to modify settings.

/* TODOs:
   1	collision control via penMin/penMax
   2	implement homing sequence via microswitch or optical device
*/

// 
// AS2 - 28.02.2016 Implemented Set Engraver SE and adopted pin assignment to GRBL/CNC-v3.00 motor shields
// AS3 - 03.03.2016 Implemented Voltage Query QC (needs a little extension of Python scripts of the Inkscape extension)
//                  Implemented an emulation of Port Access PO/PD to control engraver the "hard way" the inkscape extension does
// AS5 - 06.03.2016 Slightly adopted NEBARMIX's Servorate Control (Laser Control left out because you could use Engraver Control for a laser as well ;-)
//                  As Servos (not the Engraver) are driven by a software-PWM, it is easy to control the lift/set rate:
//                  Values 0...100% are stored as 0...500 and will lead to an artificial loop-delay now. MicroSeconds are used now for finer adjustment. 
//
//  Pin assignment of Arduino changed so that a GRBL-Motor-Shield (CNC-Motor-Shield v3.00) can be used:
//  X = Rotation Motor (changed STEP/DIR)
//  Z = Pivot Motor (changed STEP/DIR)
//  Y = Pen Servo und Engraver PWM (use of STEP/DIR independantly)
//  - Assigment of pen servo and engraver PWM to channel Y was choosen because only there both can make use real (hardwired) PWM.
//  - Engraver is assigned to Pin 3 because of known issues (see reference of analogWrite() ) on Pin 6. Servo is NOT affected due to use of soft-PWM.
//  - Buttons MOTOR-ENABLE, PEN UP/DOWN and PRG are assigned to GRBL standard button on ports AN0...AN2 now (normally open/low active).
//  - Motor Enable is common to all motors; EN was treated by software that way before.
//  - Hint for future expansions:  With GRBL v0.9/ CNC V3.01 the position of end stop Z (->12) and Spindle Enable (->11) swapped position.
//  - Voltage/Current query was introduced to give you the chance to plug in the main supply for the steppers BEFORE printing starts. 
//    No lock in firmware: Only the python script (if modified accordingly) shows a warning. 
//    Check NET_VOLTAGE_MIN on your hardware: "700" should mean about 8.5V at PWRIN and thus 8V at VIN. This is nothing but
//    a gross surveyance; no need to deal with the last millivolts here...
//
//


#include "AccelStepper.h" // nice lib from http://www.airspayce.com/mikem/arduino/AccelStepper/
#include <Servo.h>
#include "SerialCommand.h" //nice lib from Stefan Rado, https://github.com/kroimon/Arduino-SerialCommand
#include <avr/eeprom.h>
#include "button.h"

//AS3
//AS2
#define initSting "EBBv13_and_above - Protocol emulated by Eggduino-Firmware V1.6a-AS5"
//Rotational Stepper: X grbl
#define step1 2
#define dir1 5
#define enableRotMotor 8
#define rotMicrostep 16  //MicrostepMode, only 1,2,4,8,16 allowed, because of Integer-Math in this Sketch
//Pen Stepper: Z grbl
#define step2 4
#define dir2 7
#define enablePenMotor 8
#define penMicrostep 16 //MicrostepMode, only 1,2,4,8,16 allowed, because of Integer-Math in this Sketch

//Servo: GRBL Y-STEP
#define servoPin 6 // PWM with restrictions, but no matter: servo doesn't use timer-pwm
//Engraver: GRBL Y-DIR /  
#define engraverPin 3 // full range PWM on 3. Don't use 5 or 6 due to known issues of analogWrite() --> see Arrduino reference

// EXTRAFEATURES - UNCOMMENT TO USE THEM -------------------------------------------------------------------

// make use of GRBL Limit-Switches:
#define penToggleButton 14 // A0 Pen-Toggle on CNC-Shield-V3.00
#define motorsButton 15 // A1 motors enable button
#define prgButton 16 // A2 PRG-Button
#define analogPort A4 // Use spare analog port for a rudimentary detection of properly applied motor power at Arduino "PWRIN"
                      // !!! needs a hardware extension: 10k from VIN to A4 and a 4V1-Z-diode from A4 to GND to protect the port !!!
                      // Outcommenting just delivers an always positive response on query QB
// engraver doesn't need to be outcommented

//-----------------------------------------------------------------------------------------------------------

// assign EEPROM storage simply by size of data-types
#define penUpPosEEAddress ((uint16_t *)0)
#define penDownPosEEAddress ((uint16_t *)2)
#define servoRateUpEEAddress ((uint16_t *)4) //AS5 make use of inkscape control
#define servoRateDownEEAddress ((uint16_t *)6)  //AS5 make use of inkscape control
#define servoInvertEEAddress ((uint16_t *)8)  //AS5 prepare for later use by Python 

//AS4 //AS5 servo control
int servoInvert=true;  //AS4 software-adoptable inverted mounting position
                       //AS5 can be written by SC,99,0/1 now by the console until Python-Script is ready
#define SERVO_FULLSPEED 32 //AS5 scaling of variable servorate (Python script stores 0...500 = 0...100% -> emulate somehow). Maximal delay (0%) is 16.000µs/cycle
#define SERVORATE_DEFAULT 250 //AS5 use this in case of invalid or zero data in EEPROM (zero = full speed)
#define SERVO_MAXSCALE 500 //AS5 EEPROM value limits
#define SERVO_MINSCALE 0

//make Objects
AccelStepper rotMotor(1, step1, dir1);
AccelStepper penMotor(1, step2, dir2);
Servo penServo;
SerialCommand SCmd;
//create Buttons
#ifdef prgButton
Button prgButtonToggle(prgButton, setprgButtonState);
#endif
#ifdef penToggleButton
Button penToggle(penToggleButton, doTogglePen);
#endif
#ifdef motorsButton
Button motorsToggle(motorsButton, toggleMotors);
#endif

// Variables... be careful, by messing around here, everything has a reason and crossrelations...
int penMin = 0;
int penMax = 0;
int penUpPos = 5; //percent - can be overwritten from EBB-Command SC
int penDownPos = 95; //percent - can be overwritten from EBB-Command SC
int servoRateUp = 100; //percent - can be overwritten from EBB-Command SC
int servoRateDown = 25; //percent - can be overwritten from EBB-Command SC
long rotStepError = 0;
long penStepError = 0;
int penState = penUpPos;
uint32_t nodeCount = 0;
unsigned int layer = 0;
boolean prgButtonState = 0;
uint8_t rotStepCorrection = 16 / rotMicrostep ; //devide EBB-Coordinates by this factor to get EGGduino-Steps
uint8_t penStepCorrection = 16 / penMicrostep ; //devide EBB-Coordinates by this factor to get EGGduino-Steps
float rotSpeed = 0;
float penSpeed = 0; // these are local variables for Function SteppermotorMove-Command, but for performance-reasons it will be initialized here
boolean motorsEnabled = 0;
//AS2
#define ENGRAVER_OFF 0
#define ENGRAVER_MIN 0
#define ENGRAVER_MAX 1023
#define ENGRAVER_DEFAULT 511
int engraverEnabled = 0; // Engraver Off by default
int engraverState = ENGRAVER_MAX; // preloaded 100%-PWM = Full-Power (can be overwritten by EBB-Command "SE")
//AS3
int netVoltage=0;
int netVoltageState=0;
#define NET_VOLTAGE_MIN 700 // QC-reading: expecting at least about 9V as main supply (with 10k pre-resistor and 4V1 protection Z-diode at AN4)
#define RA0_DUMMY 150 // just a dummy value as EggDuino can't read motor current control voltage

void setup() {
  Serial.begin(9600);
  makeComInterface();
  initHardware();
}

void loop() {
  moveOneStep();

  SCmd.readSerial();

#ifdef penToggleButton
  penToggle.check();
#endif

#ifdef motorsButton
  motorsToggle.check();
#endif

#ifdef prgButton
  prgButtonToggle.check();
#endif
}
