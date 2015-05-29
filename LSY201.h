/*
 * LSY201.h
 *
 *  Created on: Feb 25, 2015
 *      Author: Kevin
 */

#ifndef LSY201_H_
#define LSY201_H_

#include "application.h"
#include "garagedoor.h"
#include "IPersister.h"

#define CAMERA_CHUNK 32
#ifdef __cplusplus
extern "C" {
#endif

void t(void);

class LSY201
{
  Stream *camera;
  IPersister *persist;
  uint8_t currentState;
  bool enabled;
  bool respError;
  uint8_t rxBuffer[129];
  uint8_t rxPtr=0;
  uint8_t timer=0;
  uint8_t storeRetry;
  uint16_t downloadOffset;
  uint16_t irLED;
  uint8_t respBuf[5];

public:

  enum Size
  {
    Small = 0x22,
    Medium = 0x11,
    Large = 0x00
  };

  enum State
  {
	  idle,reseting,settling,takingPic,readingContent,storeJpg,closeWait,eatFiveBytes,stopAfterStore,onDemandRead
  };

  LSY201(uint16_t irLED);
  void setSerial(Stream &stream);
  void setPersister(IPersister &p);
  void reset();
  void takePictureAndSave();
  uint16_t readJpegFileSize();
  void setCompressionRatio(uint8_t value);
  void setImageSize(Size size);
  void setBaudRate(unsigned long baud);
  void stopTakingPictures();
  void enterPowerSave(void);
  void exitPowerSave(void);
  void poll(void);
  void tick(void);
  bool isEnabled(void);
  void ledOn(void);
  void ledOff(void);
  uint8_t* getBuffer(void);
  uint8_t getPointer(void);
  void readJpegFileContent(uint16_t offset);

private:

	void tx(const uint8_t *bytes, uint8_t length);
	void simpleCommand(const uint8_t *bytes, uint8_t length, const uint8_t *resp );
	void jpegRead(void);
	void storeStop(void);


};

#ifdef __cplusplus
}
#endif

#endif /* LSY201_H_ */
