
/*
 * application.cpp
 *
 *  Created on: Dec 29, 2014
 *      Author: Kevin
 */


#include <stdlib.h>
#include <stdio.h>
#include "application.h"
#include "garagedoor.h"
#include "SparkIntervalTimer.h"
#include "DoorController.h"
#include "RGBLed.h"
#include "LSY201.h"
#include "NetworkSaver.h"
#include "OneWire.h"
#include "DS18B20.h"

SYSTEM_MODE(AUTOMATIC);

#define CLOSED_HALL D3
#define OPEN_HALL D4
#define DOOR_SWITCH D0
#define HOLD_SWITCH D2
#define PIR_LINE A0
#define ALARM D5
#define CAM_THROTTLE 120
#define CAM_IR D6
#define TEMP_PIN A1

enum camraPhases {idle,powerup,picture,stop,powerdown };

volatile uint8_t tick=0;
volatile uint8_t holdFlag=0;
volatile uint8_t holdCounter=0;
volatile uint8_t pirCounter=0;
volatile uint8_t pirTrigger=0;
uint8_t holdCalled=0;
uint8_t pirEnabled=0;
uint8_t camAvailable=1;
uint8_t camCounter=0;
uint8_t camPhase=idle;
uint8_t dsAddr[8];
uint8_t tempCounter=0;

uint16_t pirHits=0;

IntervalTimer secondTimer;
RGBLed led(A4,A5,A6);
DoorController door(OPEN_HALL,CLOSED_HALL,DOOR_SWITCH,ALARM);
LSY201 cam(CAM_IR);
NetworkSaver ns;

DS18B20 *ds=NULL;

void second_isr()
{
	tick++;
	holdCounter++;
	pirCounter++;
}


void hold_isr()
{
	holdFlag=1;
}

void pir_isr()
{
	pirTrigger=1;
}

/* This function is called once at start up ----------------------------------*/
void setup()
{

	pinMode(HOLD_SWITCH,INPUT);
	attachInterrupt(HOLD_SWITCH,hold_isr,CHANGE);

	secondTimer.begin(second_isr,2000,hmSec,TIMER2);

	led.setColor(door.getLedColor());
	led.on();

	pinMode(PIR_LINE,INPUT);
	attachInterrupt(PIR_LINE,pir_isr,CHANGE);

	pirEnabled=1;

	Serial1.begin(38400);

	cam.setSerial(Serial1);
	cam.setPersister(ns);
	cam.reset();

	OneWire *search=new OneWire(TEMP_PIN);
	search->reset();

	if(search->search(dsAddr))
		ds=new DS18B20(TEMP_PIN,dsAddr);

}

/* This function loops forever --------------------------------------------*/
void loop()
{
	uint8_t buffer[33];
//	door.poll();
	cam.poll();
	if(tick>0)
	{
//		door.tick();
		cam.tick();
//		led.toggle();
		if(!camAvailable)
		{
			camCounter++;
			if(camCounter>CAM_THROTTLE)
			{
				camAvailable=1;
				camCounter=0;
			}
		}

		getTemp();
		tick=0;

	}

//	led.setColor(door.getLedColor());
	checkCam();
//	checkHold();
	checkPIR();
}


// Check State of hold button.  If Door Close timer is already running, stop timer.
// If not, wait for 10 seconds to see if it starts, then stop it.  Otherwise, reset the hold state.
void checkHold()
{
	if(holdFlag)
	{
		if (!holdCalled) {
			door.setHold();
			holdCounter=0;
			holdCalled=1;
		}

		if(holdCounter>10) {
			holdCalled=0;
			holdFlag=0;
			door.resetHold();
		}
	}

}

// Check state of PIR motion sensor and trigger light at most once a minute
void checkPIR()
{
	if(pirTrigger)
		pirHits++;

	if(pirEnabled==0)
	{
		if(pirCounter>59)
		{
			pirCounter=0;
			pirEnabled=1;
		}

		pirTrigger=0;
	}
	else
	{
		pirCounter=0;
		if(pirTrigger==1)
		{
			char buffer[15];

			pirEnabled=0;
			takePicture();

			sprintf(buffer,"MOTION %d",pirHits);
			Spark.publish("garagedoor-event",buffer);
			pirHits=0;
		}
	}


}

void takePicture()
{
	if(camAvailable)
	{
		camPhase=powerup;
		camAvailable=0;
	}
}

void checkCam()
{

	switch(camPhase)
	{
	case idle:
		break;
	case powerup:
		if(cam.isEnabled())
		{
			cam.ledOn();
			Spark.publish("garagedoor-event","PowerUp");
			cam.exitPowerSave();
			camPhase=picture;
		}
		break;
	case picture:
		if(cam.isEnabled())
		{
			Spark.publish("garagedoor-event","Picture");
			cam.takePictureAndSave();
			camPhase=stop;
		}
		break;

	case stop:
		if(cam.isEnabled())
		{
			cam.ledOff();
			Spark.publish("garagedoor-event","Stop");
			cam.stopTakingPictures();
			camPhase=powerdown;
		}
		break;
	case powerdown:
		if(cam.isEnabled())
		{
			Spark.publish("garagedoor-event","PowerDown");
			cam.enterPowerSave();
			camPhase=idle;
		}
		break;
	}

}

void getTemp()
{
	if(ds!=NULL)
	{
		if(++tempCounter==30)
		{

			tempCounter=0;

			float celsius = ds->getTemperature();
			float fahrenheit = ds->convertToFahrenheit(celsius);

			char buffer[30];
			sprintf(buffer,"Temp:%f",fahrenheit);
			Spark.publish("garagedoor-event",buffer);
		}
	}
}
