/*
 * DoorState.h
 *
 *  Created on: Feb 9, 2015
 *      Author: Kevin
 */

#ifndef DOORCONTROLLER_H_
#define DOORCONTROLLER_H_

#include "application.h"
#include "RGBLed.h"
#include "garagedoor.h"

#ifdef __cplusplus
extern "C" {
#endif


// Door Close timings, all in seconds.  Led turns yellow on door open, then red at DOOR_WARNING_LED, sounds an alarm at DOOR_ALARM_TIME, and them closes at DOOR_CLOSE_TIME
#define DOOR_WARNING_LED 300	// Time LED changes from Yellow to Red to warn of impending close
#define DOOR_CLOSE_TIME 600		// Time door is closed
#define DOOR_ALARM_TIME 550		// Time Alarm is sounded to warn of door close

void hall_isr(void);

class DoorController {
private:
	// Flag to indicate hall ISR called

	// Digital pins
	uint16_t openHall;
	uint16_t closedHall;
	uint16_t doorSwitch;
	uint16_t alarmSwitch;
	uint16_t holdSwitch;

	//Timers
	uint8_t heartbeat=0;
	uint16_t doorTimer=0;

	//Flags
	uint8_t timerFlag=0;
	uint8_t holdFlag=0;
	uint8_t holdState=0;

	//State indicators
	uint8_t currentState=INVALID;
	uint8_t prevState=INVALID;
	uint8_t prevPos=INVALID;

	uint32_t forceColor=0;
	bool heartbeatError=false;

	uint8_t holdDeBounce=0;
	uint8_t holding=0;
	uint8_t holdCounter=0;

	void open(void);
	void closed(void);
	void alarmOn(void);
	void alarmOff(void);
	void checkHold(void);

	const char * const doorStrings[4]={"OPEN","CLOSED","MOVING","INVALID"};

public:

	DoorController(uint16_t openHall, uint16_t closedHall, uint16_t doorSwitch, uint16_t alarm, uint16_t hold);
	void poll(void);
	void tick(void);
	uint32_t getLedColor(void);
	void setHold(void);
	void resetHold(void);
	int closeDoor(void);
	int openDoor(void);
	void setErrorCondition();
	void resetErrorCondition();
	uint8_t getState(void);
};

#ifdef __cplusplus
}
#endif

#endif /* DOORCONTROLLER_H_ */
