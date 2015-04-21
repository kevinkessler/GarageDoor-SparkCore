
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

volatile uint8_t tick=0;
volatile uint8_t holdFlag=0;
volatile uint8_t holdCounter=0;
volatile uint8_t pirCounter=0;
volatile uint16_t pirTrigger=0;
uint8_t holdCalled=0;
uint8_t pirEnabled=0;
uint8_t camAvailable=1;
uint8_t camCounter=0;
uint8_t camPhase=idle;
uint8_t dsAddr[8];
uint8_t tempCounter=0;

uint8_t toggle=0;

IntervalTimer secondTimer;

RGBLed led(RED_LED,GREEN_LED,BLUE_LED,HOLD_LED);
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
	pirTrigger++;
}

int command(String function)
{
	if(function=="picture")
	{
		if(camAvailable)
			takePicture();
		else
			return -1;
	}
	else if(function=="open")
	{
		//return door.openDoor();
		digitalWrite(DOOR_SWITCH,HIGH);
		delay(500);
		digitalWrite(DOOR_SWITCH,LOW);

		return 0;
	}
	else if(function=="close")
	{
		return door.closeDoor();
	}
	else if(function=="light")
	{

		digitalWrite(LIGHT_PIN,HIGH);
		delay(500);
		digitalWrite(LIGHT_PIN,LOW);

		return 0;
	}
	else if(function.substring(0,7)=="config~")
	{
		return ns.setServer(function.substring(7));
	}
	else
		return -1;

	return 0;
}

/* This function is called once at start up ----------------------------------*/
void setup()
{

	pinMode(HOLD_SWITCH,INPUT);
	attachInterrupt(HOLD_SWITCH,hold_isr,RISING);

	secondTimer.begin(second_isr,2000,hmSec,TIMER2);

	led.setColor(door.getLedColor());
	led.on();

	pinMode(PIR_LINE,INPUT);
	attachInterrupt(PIR_LINE,pir_isr,RISING);

	pinMode(LIGHT_PIN,OUTPUT);

	Serial1.begin(38400);

	cam.setSerial(Serial1);
	cam.setPersister(ns);
	cam.reset();

	OneWire *search=new OneWire(TEMP_PIN);
	search->reset();

	if(search->search(dsAddr))
		ds=new DS18B20(TEMP_PIN,dsAddr);

	Spark.function("command",command);
	Spark.publish("garagedoor-event","CONFIGURE");
}

/* This function loops forever --------------------------------------------*/
void loop()
{
	door.poll();
	cam.poll();
	if(tick>0)
	{
		door.tick();
		cam.tick();
		led.toggle();
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


	led.setColor(door.getLedColor());
	checkCam();
	checkHold();
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
		if(pirTrigger)
		{
			pirEnabled=0;
			pirTrigger=0;
			takePicture();
			Spark.publish("garagedoor-event","MOTION");
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
	if(++tempCounter==30)
	{
		tempCounter=0;
		if(ds!=NULL)
		{

			float celsius = ds->getTemperature();
			float fahrenheit = ds->convertToFahrenheit(celsius);

			char buffer[30];
			sprintf(buffer,"Temp:%f",fahrenheit);
			Spark.publish("garagedoor-event",buffer);
		}
		else
			Spark.publish("garagedoor-event","DS Null");
	}
}
