
/*
 * application.cpp
 *
 *  Created on: Dec 29, 2014
 *      Author: Kevin
 */

/* Things to do
 * 1. Refactor into CameraController
 * 2. Check returned values from camera
 * 3. Put DEBUG publishes in ifdefs
 * 4. Take picture before close and after
 * 5. Send temperature to ThingSpeak
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
volatile uint8_t holdCounter=0;
volatile uint8_t pirCounter=0;
volatile uint16_t pirTrigger=0;
volatile uint16_t heartbeatTimer;
volatile uint16_t lightTimer;
uint8_t holdDeBounce=0;
uint8_t holding=0;
uint8_t pirEnabled=0;
uint8_t camAvailable=1;
uint8_t camCounter=0;
uint8_t camPhase=idle;
bool camDelay=0;
uint8_t dsAddr[8];
uint8_t tempCounter=0;
bool lightAct=false;


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
	heartbeatTimer++;
	lightTimer++;
}


void pir_isr()
{
	pirTrigger=1;
}

int command(String function)
{
	door.resetErrorCondition();
	heartbeatTimer=0;

	if(function=="picture")
	{
		if(camAvailable)
			takePicture();
		else
			return -1;
	}
/*	else if(function=="open")
	{
		return door.openDoor();

	}*/
	else if(function=="close")
	{
		return door.closeDoor();
	}
	else if(function=="light")
	{
		lightOn();
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

		if(camDelay)
		{
			++camCounter;
			if(camCounter > CAM_DELAY)
			{
				camDelay=false;
				takePicture();
			}
		}

		getTemp();
		tick=0;

		if(heartbeatTimer>HEARTBEAT)
			door.setErrorCondition();

	}


	led.setColor(door.getLedColor());
	checkCam();
	checkHold();
	checkPIR();
	checkLight();
}


// Check State of hold button.  If Door Close timer is already running, stop timer.
// If not, wait for 30 seconds to see if it starts, then stop it.  Otherwise, reset the hold state.
void checkHold()
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
				door.setHold();
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
			door.resetHold();
		}
	}

}

// Check state of PIR motion sensor and trigger light if no motion sensed in a minute
void checkPIR()
{

	if(pirEnabled==0)
	{
		if(pirCounter>PIR_THROTTLE)
		{
			pirCounter=0;
			pirEnabled=1;
		}

		if(pirTrigger)
			pirCounter=0;

		pirTrigger=0;
	}
	else
	{
		if(pirTrigger)
		{
			lightOn();
			pirEnabled=0;
			pirTrigger=0;
			camDelay=true;
			camCounter=0;
			pirCounter=0;
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
#ifdef DEBUG_DOOR
			Spark.publish("garagedoor-event","PowerUp");
#endif
			cam.exitPowerSave();
			camPhase=picture;
		}
		break;

	case picture:
		if(cam.isEnabled())
		{
#ifdef DEBUG_DOOR
			Spark.publish("garagedoor-event","Picture");
#endif
			led.off();
			cam.takePictureAndSave();
			camPhase=powerdown;
		}
		break;


	case powerdown:
		if(cam.isEnabled())
		{
			Spark.publish("garagedoor-event","PICTURE-SAVED");
			cam.enterPowerSave();
			camPhase=idle;
		}
		camAvailable=1;
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

void checkLight()
{
	if(lightAct)
	{
		if(lightTimer>LIGHT_TIME)
		{
			lightAct=false;
			lightToggle();
		}

	}
}

void lightOn()
{
	lightAct=true;
	lightTimer=0;
	lightToggle();
}

void lightToggle()
{
	digitalWrite(LIGHT_PIN,HIGH);
	delay(500);
	digitalWrite(LIGHT_PIN,LOW);
	Spark.publish("garagedoor-event","Light");
}
