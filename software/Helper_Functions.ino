void initHardware(){
	// enable eeprom wait in avr/eeprom.h functions
	SPMCSR &= ~SELFPRGEN;
	
  loadPenPosFromEE();
  loadServoRateFromEE();
  loadServoInvertFromEE();
  
  pinMode(enableRotMotor, OUTPUT);
	pinMode(enablePenMotor, OUTPUT);
  
  //AS2
  pinMode(engraverPin, OUTPUT); // optional according reference for analogWrite()
  //AS3
  pinMode(analogPort, INPUT); // optional according reference for analogWrite()
  digitalWrite(analogPort, LOW); // 
  //AS4
  #ifdef penToggleButton
    pinMode(penToggleButton, INPUT_PULLUP);  
  #endif
  #ifdef motorsButton
    pinMode(motorsButton, INPUT_PULLUP);  
  #endif
  #ifdef prgButton
    pinMode(prgButton, INPUT_PULLUP);  
  #endif
  prgButtonState = 0;
  
  pinMode(analogPort, INPUT); // optional according reference for analogWrite()
  
	rotMotor.setMaxSpeed(2000.0);
	rotMotor.setAcceleration(10000.0);
	penMotor.setMaxSpeed(2000.0);
	penMotor.setAcceleration(10000.0);
	motorsOff();
  
  //AS2
  engraverEnabled=0;
  engraverState=ENGRAVER_MAX;
  //AS3
  if(analogRead(analogPort)<NET_VOLTAGE_MIN){
    netVoltageState=0; // running from USB -> give a warning in control-panel and keep all motors off
    //Serial.print("Insufficient supply voltage. Check before printing.\r\n");
  }
  else
    netVoltageState=1; // running from NET -> as expected
  
  penServo.attach(servoPin);
  if (servoInvert)
    penServo.write(180-penState);
  else
    penServo.write(penState);
}

inline void loadServoInvertFromEE() {
  servoInvert = eeprom_read_word(servoInvertEEAddress);
  //false means not inverted
}  

inline void storeServoInvertInEE() {
  eeprom_update_word(servoInvertEEAddress, servoInvert);
}

inline void loadServoRateFromEE() {
	servoRateUp = eeprom_read_word(servoRateUpEEAddress);
	servoRateDown = eeprom_read_word(servoRateDownEEAddress);
  //just in case you find some rubbish like $FFFF and so on:
  if(servoRateUp<=0)
    servoRateUp=SERVORATE_DEFAULT;
  if(servoRateUp>SERVO_MAXSCALE)
    servoRateUp=SERVO_MAXSCALE;
  if(servoRateDown<=0)
    servoRateDown=SERVORATE_DEFAULT;
  if(servoRateDown>SERVO_MAXSCALE)
    servoRateDown=SERVO_MAXSCALE;
}

inline void storeServoRateUpInEE() {
	eeprom_update_word(servoRateUpEEAddress, servoRateUp);
}

inline void storeServoRateDownInEE() {
	eeprom_update_word(servoRateDownEEAddress, servoRateDown);
}

inline void loadPenPosFromEE() {
  penUpPos = eeprom_read_word(penUpPosEEAddress);
  penDownPos = eeprom_read_word(penDownPosEEAddress);
  penState = penUpPos;
}

inline void storePenUpPosInEE() {
  eeprom_update_word(penUpPosEEAddress, penUpPos);
}

inline void storePenDownPosInEE() {
  eeprom_update_word(penDownPosEEAddress, penDownPos);
}

inline void sendAck(){
	Serial.print("OK\r\n");
}

inline void sendError(){
	Serial.print("unknown CMD\r\n");
}

void motorsOff() {
	digitalWrite(enableRotMotor, HIGH);
	digitalWrite(enablePenMotor, HIGH);
	motorsEnabled = 0;
}

void motorsOn() {
	digitalWrite(enableRotMotor, LOW) ;
	digitalWrite(enablePenMotor, LOW) ;
	motorsEnabled = 1;
}

void toggleMotors() {
	if (motorsEnabled) {
		motorsOff();
	} else {
		motorsOn();
	}
}

//AS2
// Set engraver power and state by SE-command. 0-1023 -> analogWrite 0-255 -> PWM 0-100%; 
void engraverOff() {
  analogWrite(engraverPin, ENGRAVER_OFF); //0% power is set with activation or with SE-command only
}

//AS2
void engraverOn() {
  //if(penState==penDownPos) // activate engraver only when pen is down --> not neccessary here, is checked by PY-script yet!
    analogWrite(engraverPin, engraverState>>2); // scale down latest PWM-value engraverState (or default value)
}

//probably never needed, but just to be complete...
//AS2
void toggleEngraver() {
  if (engraverEnabled) {
    engraverOff();
  } else {
    engraverOn(); // use last engraverState
  }
}

bool parseSMArgs(uint16_t *duration, int *penStepsEBB, int *rotStepsEBB) {
	char *arg1;
	char *arg2;
	char *arg3;
	arg1 = SCmd.next();
	if (arg1 != NULL) {
		*duration = atoi(arg1);
		arg2 = SCmd.next();
	}
	if (arg2 != NULL) {
		*penStepsEBB = atoi(arg2);
		arg3 = SCmd.next();
	}
	if (arg3 != NULL) {
		*rotStepsEBB = atoi(arg3);

		return true;
	}

	return false;
}

void prepareMove(uint16_t duration, int penStepsEBB, int rotStepsEBB) {
	if (!motorsEnabled) {
		motorsOn();
	}
	if( (1 == rotStepCorrection) && (1 == penStepCorrection) ){ // if coordinatessystems are identical
		//set Coordinates and Speed
		rotMotor.move(rotStepsEBB);
		rotMotor.setSpeed( abs( (float)rotStepsEBB * (float)1000 / (float)duration ) );
		penMotor.move(penStepsEBB);
		penMotor.setSpeed( abs( (float)penStepsEBB * (float)1000 / (float)duration ) );
	} else {
		//incoming EBB-Steps will be multiplied by 16, then Integer-maths is done, result will be divided by 16
		// This make thinks here really complicated, but floating point-math kills performance and memory, believe me... I tried...
		long rotSteps =   (  (long)rotStepsEBB * 16 / rotStepCorrection) + (long)rotStepError;	//correct incoming EBB-Steps to our microstep-Setting and multiply  by 16 to avoid floatingpoint...
		long penSteps =   (  (long)penStepsEBB * 16 / penStepCorrection) + (long)penStepError;

		int rotStepsToGo = (int) (rotSteps/16);		//Calc Steps to go, which are possible on our machine
		int penStepsToGo = (int) (penSteps/16);

		rotStepError = (long)rotSteps - ((long) rotStepsToGo * (long)16);	// calc Position-Error, if there is one
		penStepError = (long)penSteps - ((long) penStepsToGo * (long)16);

		long temp_rotSpeed =  ((long)rotStepsToGo * (long)1000 / (long)duration );	// calc Speed in Integer Math
		long temp_penSpeed =  ((long)penStepsToGo * (long)1000 / (long)duration ) ;

		float rotSpeed= (float) abs(temp_rotSpeed);	// type cast
		float penSpeed= (float) abs(temp_penSpeed);

		//set Coordinates and Speed
		rotMotor.move(rotStepsToGo);		// finally, let us set the target position...
		rotMotor.setSpeed(rotSpeed);		// and the Speed!
		penMotor.move(penStepsToGo);
		penMotor.setSpeed( penSpeed );
	}
}

void moveOneStep() {
	if ( penMotor.distanceToGo() || rotMotor.distanceToGo() ) {
		penMotor.runSpeedToPosition(); // Moving.... moving... moving....
		rotMotor.runSpeedToPosition();
	}
}

void moveToDestination() {
	while ( penMotor.distanceToGo() || rotMotor.distanceToGo() ) {
		penMotor.runSpeedToPosition(); // Moving.... moving... moving....
		rotMotor.runSpeedToPosition();
	}
}

void setprgButtonState() {
	prgButtonState = 1;
}
