/*
 * CameraController.h
 *
 *  Created on: May 19, 2015
 *      Author: Kevin
 */

#ifndef CAMERACONTROLLER_H_
#define CAMERACONTROLLER_H_

#include "application.h"
#include "LSY201.h"
#include "garagedoor.h"
#include "NetworkSaver.h"


enum cameraPhases {idle,powerup,resetBuffer,picture,stop,powerdown,reset };

class CameraController {
private:
	LSY201 *cam;
	bool cameraAvailable;
	uint8_t camPhase;
	uint8_t camCounter=0;
	bool camDelay=0;
	NetworkSaver ns;

public:
	CameraController(uint16_t ir);
	int8_t takePicture(void);
	int8_t takePictureWithDelay(void);
	void tick(void);
	void poll(void);
	int8_t setSaverServer(String serverAndPort);
};

#endif /* CAMERACONTROLLER_H_ */
