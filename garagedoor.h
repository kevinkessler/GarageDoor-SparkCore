/*
 * garagedoor.h
 *
 *  Created on: Feb 6, 2015
 *      Author: Kevin
 */

#ifndef GARAGEDOOR_H_
#define GARAGEDOOR_H_

// Pin Defines
#define HOLD_SWITCH D1
#define HOLD_LED D2
#define CLOSED_HALL D3
#define OPEN_HALL D4
#define ALARM D5
#define DOOR_SWITCH D6
#define LIGHT_PIN D7
#define PIR_LINE A0
#define TEMP_PIN A1
#define CAM_IR A2
#define RED_LED A4
#define GREEN_LED A5
#define BLUE_LED A6

#define PIR_THROTTLE 120
#define CAM_DELAY 5
#define HEARTBEAT 5*3600

enum camraPhases {idle,powerup,picture,stop,powerdown };

void checkHold(void);
void checkPIR(void);
void checkCam(void);
void takePicture(void);
void getTemp(void);
void lightOn(void);

#endif /* GARAGEDOOR_H_ */
