/*
 * DoorState.cpp
 *
 *  Created on: Feb 9, 2015
 *      Author: Kevin
 */

#include "DoorController.h"

uint8_t hallFlag;

void hall_isr()
{

	hallFlag=1;

}

// Public Methods
DoorController::DoorController(uint16_t open, uint16_t closed, uint16_t dSwitch, uint16_t alarm, uint16_t hold) {
	openHall=open;
	closedHall=closed;
	doorSwitch=dSwitch;
	alarmSwitch = alarm;
	holdSwitch=hold;

	pinMode(openHall, INPUT);
	attachInterrupt(openHall,hall_isr,CHANGE);
	pinMode(closedHall, INPUT);
	attachInterrupt(closedHall,hall_isr,CHANGE);

	pinMode(doorSwitch,OUTPUT);
	digitalWrite(doorSwitch,LOW);
	pinMode(alarmSwitch,OUTPUT);
	pinMode(HOLD_SWITCH,INPUT);

	alarmOff();


}

void DoorController::poll() {
	getState();

	if(hallFlag) {
		getState();
		Spark.publish("garagedoor-event",doorStrings[currentState]);
		hallFlag=0;
	}

	switch(currentState)
	{
	case OPEN:
		open();
		break;
	case CLOSED:
		closed();
		break;
	}

	checkHold();
}

uint32_t DoorController::getLedColor() {

	if(heartbeatError)
		return (LED_RED);

	if(forceColor != 0)
		return forceColor;

	switch(currentState)
	{
	case INVALID:
		return LED_RED;
	case MOVING:
		return (LED_BLINK | LED_CYAN);
	case OPEN:
		return (LED_BLINK | LED_YELLOW);
	case CLOSED:
		return (LED_BLINK | LED_GREEN);
	}

	return (LED_BLINK | LED_WHITE);

}

void DoorController::setErrorCondition() {
	heartbeatError=true;
}

void DoorController::resetErrorCondition() {
	heartbeatError=false;
}

void DoorController::setHold() {
	if(holdFlag != 1)
		Spark.publish("garagedoor-event","HOLD_OPEN");
	holdFlag=1;
	forceColor=(LED_BLINK | LED_BLUE);
}

void DoorController::resetHold() {
	holdFlag=0;
	if(currentState != OPEN)
		forceColor=0;
}

void DoorController::tick() {

/*	if(++heartbeat==30) {
		Spark.publish("garagedoor-event",doorStrings[currentState]);
		heartbeat=0;
	}*/

	doorTimer++;
	holdCounter++;

}

// Private Methods
void DoorController::open(void) {
	if(holdFlag)
		holdState=1;

	// Turn on timerFlag (start countdown for auto close) if the hold button hasn't been pushed, and the door was closed previously (did not previously start to close and bounce back up to open again
	// because something was blocking the door)
	if ((!timerFlag)&&(prevPos==CLOSED)&&(!holdState)) {
		timerFlag=1;
		doorTimer=0;
		prevPos=OPEN;
	}

	// Check if timer is running.  Set LED to yellow if under 5 minutes, set it to red if only 5 minutes left, sound the piezo for the last 30 seconds,
	// then close the door at 10 minutes
	if(timerFlag) {

		if(holdState) {
			timerFlag=0;
			forceColor=(LED_BLINK | LED_BLUE);
			holdFlag=0;
			alarmOff();
			return;
		}

		if(doorTimer<DOOR_WARNING_LED)
			forceColor=(LED_BLINK | LED_YELLOW);
		else
			forceColor=(LED_BLINK | LED_RED);

		if(doorTimer > DOOR_CLOSE_TIME)
		{
			alarmOff();
			Spark.publish("garagedoor-event","FORCE-CLOSE");
			closeDoor();
			timerFlag=0;
		}
		else if(doorTimer>DOOR_ALARM_TIME)
			alarmOn();

	}

}

void DoorController::closed(void) {
	prevPos=CLOSED;
	timerFlag=0;
	if(!holdFlag)
		forceColor=0;
	holdState=0;

	alarmOff();
}




uint8_t DoorController::getState(){
	uint8_t status=(digitalRead(openHall)<<1) + (digitalRead(closedHall));

	uint8_t thisState=INVALID;
	switch(status)
	{
	case 3:
		thisState=MOVING;
		break;

	case 2:
		thisState=CLOSED;
		break;

	case 1:
		thisState=OPEN;
		break;
	case 0:
		thisState=INVALID;

	}

	if(thisState!=currentState)
	{
		prevState=currentState;
		currentState=thisState;

	}

	return thisState;
}

int DoorController::closeDoor() {


	if(currentState==OPEN) {
		digitalWrite(doorSwitch,HIGH);
		delay(500);
		digitalWrite(doorSwitch,LOW);
		return 0;
	}

	return -1;
}

int DoorController::openDoor() {


	if(currentState==CLOSED) {
		digitalWrite(doorSwitch,HIGH);
		delay(500);
		digitalWrite(doorSwitch,LOW);
		return 0;
	}

	return -1;
}

void DoorController::alarmOn() {
	digitalWrite(alarmSwitch,LOW);
}

void DoorController::alarmOff() {
	digitalWrite(alarmSwitch,HIGH);
}
// Check State of hold button.  If Door Close timer is already running, stop timer.
// If not, wait for 30 seconds to see if it starts, then stop it.  Otherwise, reset the hold state.
void DoorController::checkHold()
{

	// Debounce so the button must be pushed for 50 * 5ms per loop = .25 second
	// Will have to be changed when the update the firmware to a faster loop
	if(!holding)
	{
		if(!digitalRead(HOLD_SWITCH))
		{
			holdDeBounce++;
			if(holdDeBounce==50)
			{
				holding=true;
				holdDeBounce=0;
				setHold();
				holdCounter=0;
			}
		}
		else
		{
			holdDeBounce=0;
		}
	}

	if(holding)
	{
		if(holdCounter>30) {
			holding=0;
			resetHold();
		}
	}

}
