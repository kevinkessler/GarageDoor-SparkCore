/*
 * CameraController.cpp
 *
 *  Created on: May 19, 2015
 *      Author: Kevin
 */

#include "CameraController.h"

CameraController::CameraController(uint16_t ir) {
	cam=new LSY201(ir);

	Serial1.begin(38400);

	cam->setSerial(Serial1);
	cam->setPersister(ns);
	cam->reset();

	cameraAvailable=true;
	camPhase=idle;

}

int8_t CameraController::takePicture() {
	if(cameraAvailable)
	{
		camPhase=powerup;
		cameraAvailable=false;
		return 0;
	}
	else
		return -1;
}

int8_t CameraController::takePictureWithDelay()
{
	if(cameraAvailable)
	{
		camDelay=true;
		camCounter=0;
		cameraAvailable=false;
		return 0;
	}
	else
		return -1;
}

int8_t CameraController::setSaverServer(String serverAndPort)
{
	return ns.setServer(serverAndPort);
}

void CameraController::poll()
{
	cam->poll();
	switch(camPhase)
	{
	case idle:
		break;

	case powerup:
		if(cam->isEnabled())
		{
			cam->ledOn();
#ifdef DEBUG_DOOR
			Spark.publish("garagedoor-event","PowerUp");
#endif
			//cam->exitPowerSave();
			camPhase=stop;
		}
		break;

	case stop:
		if(cam->isEnabled())
		{
			//cam->ledOn();
#ifdef DEBUG_DOOR
			Spark.publish("garagedoor-event","Stop");
#endif
			cam->stopTakingPictures();
			camPhase=picture;
		}
		break;

	case picture:
		if(cam->isEnabled())
		{
#ifdef DEBUG_DOOR
			Spark.publish("garagedoor-event","Picture");
#endif

			cam->takePictureAndSave();
			camPhase=powerdown;
		}
		break;


	case powerdown:
		if(cam->isEnabled())
		{
			cam->ledOff();
			Spark.publish("garagedoor-event","PICTURE-SAVED");
//			cam->enterPowerSave();
			camPhase=idle;
			cameraAvailable=true;
		}

		break;
	}
}

void CameraController::tick()
{
	cam->tick();

	if(camDelay)
	{
		++camCounter;
		if(camCounter > CAM_DELAY)
		{
			camDelay=false;
			camPhase=powerup;
		}
	}

}

