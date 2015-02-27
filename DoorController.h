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

#ifdef __cplusplus
extern "C" {
#endif

enum {OPEN,CLOSED,MOVING,INVALID};


void hall_isr(void);

class DoorController {
private:
	// Flag to indicate hall ISR called

	// Digital pins
	uint16_t openHall;
	uint16_t closedHall;
	uint16_t doorSwitch;
	uint16_t alarmSwitch;

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

	void getState(void);
	void open(void);
	void closed(void);
	void closeDoor(void);
	void alarmOn(void);
	void alarmOff(void);

	const char * const doorStrings[4]={"OPEN","CLOSED","MOVING","INVALID"};

public:

	DoorController(uint16_t openHall, uint16_t closedHall, uint16_t doorSwitch, uint16_t alarm);
	void poll(void);
	void tick(void);
	uint32_t getLedColor(void);
	void setHold(void);
	void resetHold(void);
};

#ifdef __cplusplus
}
#endif

#endif /* DOORCONTROLLER_H_ */
