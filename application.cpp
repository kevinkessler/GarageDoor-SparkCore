
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
#include "commandstrings.h"
#include "application.h"
#include "garagedoor.h"
#include "SparkIntervalTimer.h"
#include "DoorController.h"
#include "RGBLed.h"
#include "OneWire.h"
#include "DS18B20.h"
#include "CameraController.h"

SYSTEM_MODE(AUTOMATIC);

volatile uint8_t tick=0;
volatile uint8_t pirCounter=0;
volatile uint16_t pirTrigger;
volatile uint8_t doorMotionTimer;
volatile uint16_t heartbeatTimer;
volatile uint16_t lightTimer;
uint8_t pirEnabled=0;
uint8_t tempCounter=0;
bool lightAct=false;
uint8_t doorPrevState=INVALID;
uint8_t lightSupression=0;

volatile bool camPic;

IntervalTimer secondTimer;

RGBLed led(RED_LED,GREEN_LED,BLUE_LED,HOLD_LED);
DoorController door(OPEN_HALL,CLOSED_HALL,DOOR_SWITCH,ALARM,HOLD_SWITCH);
CameraController *camCtl;

DS18B20 *ds=NULL;

void second_isr()
{
	tick++;
	pirCounter++;
	heartbeatTimer++;
	lightTimer++;
	doorMotionTimer++;
}


void pir_isr()
{
	pirTrigger=1;
}

int command(String function)
{
	door.resetErrorCondition();
	heartbeatTimer=0;

	if(function==PICTURE_COMMAND)
	{
		return camCtl->takePicture();
	}
	else if(function==OPEN_COMMAND)
	{
		return door.openDoor();

	}
	else if(function==CLOSE_COMMAND)
	{
		return door.closeDoor();
	}
	else if(function==LIGHT_COMMAND)
	{
		lightOn();
		return 0;
	}
	else if(function.substring(0,strlen(CONFIG_COMMAND))==CONFIG_COMMAND)
	{
		return camCtl->setSaverServer(function.substring(strlen(CONFIG_COMMAND)));
	}

	else
		return -1;

	return 0;
}

/* This function is called once at start up ----------------------------------*/
void setup()
{
	uint8_t dsAddr[8];

	secondTimer.begin(second_isr,2000,hmSec,TIMER2);

	led.setColor(door.getLedColor());
	led.on();

	pinMode(PIR_LINE,INPUT);
	attachInterrupt(PIR_LINE,pir_isr,RISING);

	pinMode(LIGHT_PIN,OUTPUT);

	camCtl=new CameraController(CAM_IR);
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

	// Processes run each loop
	door.poll();
	camCtl->poll();
	led.setColor(door.getLedColor());
	checkPIR();
	checkLight();

	// Things to run once a second
	if(tick>0)
	{
		tick=0;

		door.tick();
		camCtl->tick();
		led.toggle();
		getTemp();

		if(heartbeatTimer>HEARTBEAT)
			door.setErrorCondition();

	}

}




// Check state of PIR motion sensor and trigger light if no motion sensed in the prior minute
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
			camCtl->takePictureWithDelay();
			pirCounter=0;
			Spark.publish("garagedoor-event","MOTION");
		}
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
	// Don't turn on the light if the door has moved in the last LIGHT_SUPRESSION_TIME seconds (because the light is already on)
	if(doorPrevState!=door.getState())
	{
		doorPrevState=door.getState();
		doorMotionTimer=0;
		lightSupression=1;
	}

	if((lightSupression==1)&&(doorMotionTimer>LIGHT_SUPRESSION_TIME))
			lightSupression=0;

	// Turn off the light after LIGHT_TIME seconds
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
	if(!lightSupression)
	{
		lightAct=true;
		lightTimer=0;
		lightToggle();
	}
}

void lightToggle()
{
	digitalWrite(LIGHT_PIN,HIGH);
	delay(500);
	digitalWrite(LIGHT_PIN,LOW);
#ifdef DEBUG_DOOR
	Spark.publish("garagedoor-event","Light");
#endif
}
