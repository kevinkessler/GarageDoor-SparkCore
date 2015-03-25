/*
 * RGBLed.cpp
 *
 *  Created on: Feb 3, 2015
 *      Author: kkessler
 */

#include "RGBLed.h"

RGBLed::RGBLed(uint16_t red, uint16_t green, uint16_t blue, uint16_t hold) {
	redPin=red;
	greenPin=green;
	bluePin=blue;
	holdPin=hold;

	pinMode(redPin,OUTPUT);
	pinMode(greenPin,OUTPUT);
	pinMode(bluePin,OUTPUT);
	pinMode(holdPin,OUTPUT);

	off();
}

void RGBLed::off() {
	analogWrite(redPin,255);
	analogWrite(bluePin,255);
	analogWrite(greenPin,255);
	digitalWrite(holdPin,HIGH);
	currentState=0;

}

void RGBLed::on() {
	analogWrite(redPin,(initialColor>>16 & 0xFF));
	analogWrite(greenPin,(initialColor>>8 & 0xFF));
	analogWrite(bluePin,(initialColor&0xFF));
	currentState=1;

	if(initialColor==LED_BLUE)
		digitalWrite(holdPin,LOW);
	else
		digitalWrite(holdPin,HIGH);

}

void RGBLed::toggle() {
	if(blink) {
		if(currentState==1)
			off();
		else
			on();
	}

}
void RGBLed::setColor(uint32_t color) {

	curColor=color & 0x00FFFFFF;  //Remove Blink Attribute
	initialColor=curColor;


	if(color >> 24)
		blink=1;
	else
		blink=0;

}

RGBLed::~RGBLed() {
	off();
}

