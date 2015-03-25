/*
 * RGBLed.h
 *
 *  Created on: Feb 3, 2015
 *      Author: kkessler
 */

#ifndef CORE_FIRMWARE_APPLICATIONS_GARAGEDOOR_RGBLED_H_
#define CORE_FIRMWARE_APPLICATIONS_GARAGEDOOR_RGBLED_H_

#include "application.h"

#define LED_RED 0x00FFFF
#define LED_BLUE 0xFFFF00
#define LED_GREEN 0xFF00FF
#define LED_YELLOW 0x0000FF
#define LED_WHITE 0x000000
#define LED_CYAN 0xFF0000
#define LED_MAGENTA 0x00FF00
#define LED_BLINK 0x01000000

#ifdef __cplusplus
extern "C" {
#endif

class RGBLed {
private:
	uint16_t redPin;
	uint16_t greenPin;
	uint16_t bluePin;
	uint16_t holdPin;
	uint32_t curColor;
	uint32_t initialColor;
	uint8_t currentState;
	uint8_t blink=1;

public:
	RGBLed(uint16_t, uint16_t, uint16_t, uint16_t);
	void off(void);
	void on(void);
	void setColor(uint32_t color);
	void toggle(void);

	virtual ~RGBLed();
};

#ifdef __cplusplus
}
#endif

#endif /* CORE_FIRMWARE_APPLICATIONS_GARAGEDOOR_RGBLED_H_ */
