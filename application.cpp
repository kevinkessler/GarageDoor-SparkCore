
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

SYSTEM_MODE(AUTOMATIC);

#define CLOSED_HALL D3
#define OPEN_HALL D4
#define DOOR_SWITCH D0
#define HOLD_SWITCH D2
#define PIR_LINE A0
#define ALARM D5

volatile uint8_t doorState=CLOSED;
volatile uint8_t tick=0;
volatile uint8_t heartbeat=0;
volatile uint8_t holdFlag=0;
volatile uint8_t holdCounter=0;
volatile uint8_t pirCounter=0;
volatile uint8_t pirTrigger=0;
uint8_t holdCalled=0;
uint8_t pirEnabled=0;
uint8_t pirPrev=0;

uint8_t tickCounter=0;
uint8_t picEnable=0;

IntervalTimer secondTimer;
RGBLed led(A4,A5,A6);
DoorController door(OPEN_HALL,CLOSED_HALL,DOOR_SWITCH,ALARM);
LSY201 cam;
NetworkSaver ns;

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
		tick=0;

		if(picEnable==3)
		{
			if(cam.isEnabled())
			{
				Spark.publish("garage-event","Power Save");
				cam.enterPowerSave();
				picEnable=4;
			}
		}

		if(picEnable==2)
		{
			if(cam.isEnabled())
			{
				Spark.publish("garage-event","Stop");
				cam.stopTakingPictures();
				picEnable=3;
			}
		}

		if(picEnable==1)
		{
			if (cam.isEnabled())
			{
				Spark.publish("garage-event","snap");
				picEnable=2;
				cam.takePictureAndSave();
			}
		}

		if(picEnable==0)
			if(cam.isEnabled())
			{
				Spark.publish("garage-event","Set Size");
				picEnable=1;
				cam.setImageSize(LSY201::Large);
			}
		sprintf((char *)buffer,"Ptr=%d",cam.getPointer());
		Spark.publish("garage-event",(char *)buffer);
	}

//	led.setColor(door.getLedColor());
//	checkHold();
//	checkPIR();
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
		pirCounter=0;
		if(pirTrigger==1)
		{
			pirEnabled=0;
			Spark.publish("garagedoor-event","MOTION");
		}
	}


}

void doTrans()
{
	byte server[]={192,168,1,13};
	TCPClient c;

	if(c.connect(server,12345))
	{
		for (int n=0;n<10;n++)
			c.write('A'+n);

		delay(100);
		c.stop();
	}
	else
	{
		Spark.publish("garage-event","Connect Fail");
	}
}

