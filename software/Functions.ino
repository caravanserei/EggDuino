
void makeComInterface(){
	SCmd.addCommand("v",sendVersion);
	SCmd.addCommand("EM",enableMotors);
	SCmd.addCommand("SC",stepperModeConfigure);
	SCmd.addCommand("SP",setPen);
	SCmd.addCommand("SM",stepperMove);
	SCmd.addCommand("TP",togglePen); // toggle engraver as well if active 
  SCmd.addCommand("SE",setEngraver); //set engraver PWM (if never used, engraver always applies 0%/OFF or 100%/ON after Reset) //AS2
  SCmd.addCommand("PO",switchEngraver); //engraver control on/off by original Eibot-command //AS4
  SCmd.addCommand("PD",ignore); //Pin direction of EiBot can be ignored (but be acknoledged!) on Eggduino/GRBL! //AS4
	SCmd.addCommand("NI",nodeCountIncrement);
	SCmd.addCommand("ND",nodeCountDecrement);
	SCmd.addCommand("SN",setNodeCount);
	SCmd.addCommand("QN",queryNodeCount);
	SCmd.addCommand("SL",setLayer);
	SCmd.addCommand("QL",queryLayer);
	SCmd.addCommand("QP",queryPen);
	SCmd.addCommand("QB",queryButton);  //"PRG" Button
  SCmd.addCommand("QC",queryCurrent);  //"VIN" Monitor //AS3
	SCmd.setDefaultHandler(unrecognized); // Handler for command that isn't matched (says "What?")
}

//AS4
//writes servo position direct or inverted with respect to mounting position. Depends on servoInvert true/false (speaks for itself I think)
void writeServo( int x){
  if (servoInvert)
    penServo.write(180-x);
  else
    penServo.write(x); 
}

//AS3
void queryCurrent() {
  char state;
  int value;
  value = analogRead(analogPort);
  if(value<NET_VOLTAGE_MIN)
    netVoltageState=0;
  else  
    netVoltageState=1;
  netVoltage = value;
#ifndef analogPort
    //overwrite negative response (if any) if function is outcommented
    netVoltageState=1;
    netVoltage=NET_VOLTAGE_MIN;
#endif
  Serial.print(String(RA0_DUMMY)+","+String(netVoltage)+"\n"); //just gives a raw reading to emulate EiBotBoard
  // Serial.print(String(netVoltageState) + ": " + String(netVoltage) + "\n"); // DEBUG
  sendAck();
}

void queryPen() {
	char state;
	if (penState==penUpPos)
		state='1';
	else
		state='0';
	Serial.print(String(state)+"\r\n");
	sendAck();
}

void queryButton() {
	Serial.print(String(prgButtonState) +"\r\n");
	sendAck();
	prgButtonState = 0;
}

void queryLayer() {
	Serial.print(String(layer) +"\r\n");
	sendAck();
} 

void setLayer() {
	uint32_t value=0;
	char *arg1;
	arg1 = SCmd.next();
	if (arg1 != NULL) {
		value = atoi(arg1);
		layer=value;
		sendAck();
	}
	else
		sendError();
}

void queryNodeCount() {
	Serial.print(String(nodeCount) +"\r\n");
	sendAck();

}

void setNodeCount() {
	uint32_t value=0;
	char *arg1;
	arg1 = SCmd.next();
	if (arg1 != NULL) {
		value = atoi(arg1);
		nodeCount=value;
		sendAck();
	}
	else
		sendError();
}

void nodeCountIncrement() {
	nodeCount=nodeCount++;
	sendAck();	
}

void nodeCountDecrement() {
	nodeCount=nodeCount--;
	sendAck();
}

void stepperMove() {
	uint16_t duration=0; //in ms
	int penStepsEBB=0; //Pen
	int rotStepsEBB=0; //Rot

	moveToDestination();

	if (!parseSMArgs(&duration, &penStepsEBB, &rotStepsEBB)) {
		sendError();
		return;
	}

	sendAck();

	if ( (penStepsEBB==0) && (rotStepsEBB==0) ) {
		delay(duration);
		return;
	}

	prepareMove(duration, penStepsEBB, rotStepsEBB);
}

void setPen(){
	int cmd;
	int value;
	char *arg;
  int penDistance,x,servoDelay;

	moveToDestination();

	arg = SCmd.next();
	if (arg != NULL) {
		cmd = atoi(arg);
/*//DEBUG    
    Serial.println(servoFullSpeed);
    Serial.println(servoRateDown);
    Serial.println(servoRateUp);
//DEBUG*/
		switch (cmd) {
			case 0: //-->servo_max-->DOWN!
        //AS5
        servoDelay = SERVO_MAXSCALE*SERVO_FULLSPEED - servoRateDown*SERVO_FULLSPEED; //Express as a scaling down of a delay that matches the actual speed of the servo  
        penDistance = penDownPos-penState;  
        if(penDistance < 0) //coming from high value down  
          for(x=penState; x > penDownPos; x--) { //This needs the sign from penDistance in the x > penDownPos check  
            delayMicroseconds(servoDelay);
            writeServo(x);
          }    
        else if(penDistance > 0) //coming from a low value up  
          for(x=penState; x < penDownPos; x++) { //This needs the sign from penDistance in the x > penDownPos check  
            delayMicroseconds(servoDelay);                
            writeServo(x);
          }  
          
        //AS5 corrected to DOWN
        // write final position
        writeServo(penDownPos);
        penState=penDownPos;
        
        //AS4
        /* controlled by script yet (applies to a laser, too!)
        if(engraverEnabled)
          engraverOff();
				*/
				break;

			case 1: //-->servo_min-->UP!
				//AS5
        servoDelay = SERVO_MAXSCALE*SERVO_FULLSPEED - servoRateUp*SERVO_FULLSPEED; //Express as a scaling down of a delay that matches the actual speed of the servo  
        penDistance = penUpPos-penState;  
        if(penDistance < 0) //coming from high value down  
          for(x=penState; x > penUpPos; x--) //This needs the sign from penDistance in the x > penDownPos check  
            {  
            delayMicroseconds(servoDelay);
            writeServo(x);
            }  
        else if(penDistance > 0) //coming from a low value up  
          for(x=penState; x < penUpPos; x++) //This needs the sign from penDistance in the x > penDownPos check  
            {  
            delayMicroseconds(servoDelay);                
            writeServo(x);
            }  
          
        //AS5 corrected to UP
        // write final position
        writeServo(penUpPos);
        penState=penUpPos;
        
				//AS4 
				/* controlled by script yet (applies to a laser, too!)
				if(engraverEnabled)
          engraverOn();
        else
          engraverOff();  
        */  
				break;

			default:
				sendError();
		}
	}
	char *val;
	val = SCmd.next();
	if (val != NULL) {
		value = atoi(val);
		sendAck();
		delay(value);//???
	}
	if (val==NULL && arg !=NULL)  {
		sendAck();
		delay(500);//???
	}
	//	Serial.println("delay");
	if (val==NULL && arg ==NULL)
		sendError();
}  

//AS2 //AS4
void setEngraver(){
  int cmd;
  int value;
  char *arg;

  arg = SCmd.next();
  if (arg != NULL) {
    cmd = atoi(arg);
    switch (cmd) {
      case 0:
        engraverEnabled=0; // switch off PWM on any SE,0 or SE,0,x
        break;
      case 1:
        engraverEnabled=1; // switch on PWM on any SE,1 or SE,1,x
        break;
      default:
        sendError(); // accept only 0 or 1 here
      }
  }
  char *val;
  val = SCmd.next();
  if (val != NULL) {
    value = atoi(val);
    if (value<=0)
      value=0;
    else if (value>ENGRAVER_MAX)
      value=ENGRAVER_MAX; 
    engraverState=value; // limit PWM
  }
  else // no parameter means 50% on Eibot
    value=ENGRAVER_DEFAULT; 
      
  if ((value==0) || (cmd==0)){
    engraverOff(); // just to be sure: besides SE,0,x any SE,1,0 or SE,0,0 switches off PWM immediately
    engraverEnabled=0;
  }
    
  if ((engraverState) && (engraverEnabled) && (penState==penDownPos))
    engraverOn(); // take over new value at once if Pen is active yet
  
    
  sendAck();
  //delay(500);
  
  //  Serial.println("delay");
  if (val==NULL && arg ==NULL)
    sendError();
}  

//AS4
void switchEngraver(){
  char port;
  int pin;
  int value;
  int error=0;
  char *arg;

  arg = SCmd.next();
  if (arg != NULL) {
    port=*arg;  
    switch (port) {
      case 'B':
        break;
      default:
        error=1; //accept port B only
     }
  }
  else
    error=1; // no port!
  
  arg = SCmd.next();
  if (arg != NULL) {
    pin=atoi(arg);  
    switch (pin) {
      case 3:
        break;
      default:
        error=1; //accept pin 3 only
     }
  }  
  else
    error=1; // no pin!
  
  arg = SCmd.next();
  if (arg != NULL) {
    value = atoi(arg);
    switch(value){
      case 0:
        engraverOff();
        engraverEnabled=0;
        break;
      case 1:
        engraverOn();
        engraverEnabled=1;
        break;
      default:
        error=1; //accept 0 and 1 only
      }   
  }
  else
    error=1; // no value!
  
  //got port & pin & value?
  if(error){
    sendError();
  }  
  else  
    sendAck();
  //delay(500);
  
}  


void togglePen(){
	int value;
	char *arg;

	moveToDestination();

	arg = SCmd.next();
	if (arg != NULL)
		value = atoi(arg);
	else
		value = 500;

	doTogglePen();
	sendAck();
	delay(value);
}

void doTogglePen() {
  int penDistance,x,servoDelay; 
  
	  if (penState==penUpPos) { // goin' down
        //AS5
        servoDelay = SERVO_MAXSCALE*SERVO_FULLSPEED - servoRateDown*SERVO_FULLSPEED; //Express as a scaling down of a delay that matches the actual speed of the servo  
        penDistance = penDownPos-penUpPos;  
        if(penDistance < 0) //coming from high value down  
          {  
          for(x=penUpPos; x > penDownPos; x--) //This needs the sign from penDistance in the x > penDownPos check  
            {  
            delayMicroseconds(servoDelay);
            writeServo(x);
            }  
          }  
        else if(penDistance > 0) //coming from a low value up  
          {  
          for(x=penUpPos; x < penDownPos; x++) //This needs the sign from penDistance in the x > penDownPos check  
            {  
            delayMicroseconds(servoDelay);                
            writeServo(x);
            }  
          }  
    // write final position
    writeServo(penDownPos);
    penState=penDownPos;

    /* controlled by script yet (applies to a laser, too!)
    if(engraverEnabled)
      engraverOn();
    else
      engraverOff();    
    */
    } 
  else
    {
    //AS5
    servoDelay = SERVO_MAXSCALE*SERVO_FULLSPEED - servoRateUp*SERVO_FULLSPEED; //Express as a scaling down of a delay that matches the actual speed of the servo  
    penDistance = penUpPos-penState;  
    if(penDistance < 0) //coming from high value down  
      {  
      for(x=penState; x > penUpPos; x--) //This needs the sign from penDistance in the x > penDownPos check  
        {  
        delayMicroseconds(servoDelay);
        writeServo(x);
        }  
      }  
    else if(penDistance > 0) //coming from a low value up  
      {  
      for(x=penState; x < penUpPos; x++) //This needs the sign from penDistance in the x > penDownPos check  
        {  
        delayMicroseconds(servoDelay);                
        writeServo(x);
        }  
      }  
    // write final position
    writeServo(penUpPos);
    penState=penUpPos;
    
    /* controlled by script yet (applies to a laser, too!)
    engraverOff();
    */
  }
}

void enableMotors(){
	int cmd;
	int value;
	char *arg;
	char *val;
	arg = SCmd.next();
	if (arg != NULL)
		cmd = atoi(arg);
	val = SCmd.next();
	if (val != NULL)
		value = atoi(val);
	//values parsed
	if ((arg != NULL) && (val == NULL)){
		switch (cmd) {
			case 0: motorsOff();
				sendAck();
				break;
			case 1: motorsOn();
				sendAck();
				break;
			default:
				sendError();
		}
	}
	//the following implementaion is a little bit cheated, because i did not know, how to implement different values for first and second argument.
	if ((arg != NULL) && (val != NULL)){
		switch (value) {
			case 0: motorsOff();
				sendAck();
				break;
			case 1: motorsOn();
				sendAck();
				break;
			default:
				sendError();
		}
	}
}

void stepperModeConfigure(){
	int cmd;
	int value;
	char *arg;
	arg = SCmd.next();
	if (arg != NULL)
		cmd = atoi(arg);
	char *val;
	val = SCmd.next();
	if (val != NULL)
		value = atoi(val);
	if ((arg != NULL) && (val != NULL)){
		switch (cmd) {
      //AS5
      //faulty assignment 4/5 to up/down fixed
			case 4: penUpPos= (int) ((float) (value-6000)/(float) 133.3); // transformation from EBB to PWM-Servo
				storePenUpPosInEE();
				sendAck();
				break;
			case 5: penDownPos= (int)((float) (value-6000)/(float) 133.3); // transformation from EBB to PWM-Servo
				storePenDownPosInEE();
				sendAck();
				break;
			case 6: //rotMin=value;    ignored
				sendAck();
				break;
			case 7: //rotMax=value;    ignored
				sendAck();
				break;
			case 11: servoRateUp=(int)value; //AS5 limit to 16bit when writing into EEPROM
         storeServoRateUpInEE();
				 sendAck();
				 break;
			case 12: servoRateDown=(int)value; //AS5 limit to 16bit when writing into EEPROM
         storeServoRateDownInEE();
				 sendAck();
				 break;
      case 99: servoInvert=(int)value; //AS5 limit to 16bit when writing into EEPROM
        storeServoInvertInEE();
        sendAck();
        break;
			default:
				 sendError();
		}
	}
}

void sendVersion(){
	Serial.print(initSting);
	Serial.print("\r\n");
}

void unrecognized(const char *command){
	sendError();
}

void ignore(){
	sendAck();
}
