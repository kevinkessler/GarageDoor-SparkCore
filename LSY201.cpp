/*
 * LSY201.cpp
 *
 *  Created on: Feb 25, 2015
 *      Author: Kevin
 */

#include "LSY201.h"

static const uint8_t TX_RESET[] = { 0x56, 0x00, 0x26, 0x00 };
//static const uint8_t RX_RESET[] = { 0x76, 0x00, 0x26, 0x00 };

static const uint8_t TX_TAKE_PICTURE[] = { 0x56, 0x00, 0x36, 0x01, 0x00 };
//static const uint8_t RX_TAKE_PICTURE[] = { 0x76, 0x00, 0x36, 0x00, 0x00 };

//static const uint8_t TX_READ_JPEG_FILE_SIZE[] = { 0x56, 0x00, 0x34, 0x01, 0x00 };
//static const uint8_t RX_READ_JPEG_FILE_SIZE[] = { 0x76, 0x00, 0x34, 0x00, 0x04, 0x00, 0x00 };

static const uint8_t TX_READ_JPEG_FILE_CONTENT[] = { 0x56, 0x00, 0x32, 0x0C, 0x00, 0x0A, 0x00, 0x00 };
//static const uint8_t RX_READ_JPEG_FILE_CONTENT[] = { 0x76, 0x00, 0x32, 0x00, 0x00 };

static const uint8_t TX_STOP_TAKING_PICTURES[] = { 0x56, 0x00, 0x36, 0x01, 0x03 };
static const uint8_t RX_STOP_TAKING_PICTURES[] = { 0x76, 0x00, 0x36, 0x00, 0x00 };

static const uint8_t TX_SET_COMPRESSION_RATIO[] = { 0x56, 0x00, 0x31, 0x05, 0x01, 0x01, 0x12, 0x04 };
static const uint8_t RX_SET_COMPRESSION_RATIO[] = { 0x76, 0x00, 0x31, 0x00, 0x00 };

static const uint8_t TX_SET_IMAGE_SIZE[] = { 0x56, 0x00, 0x31, 0x05, 0x04, 0x01, 0x00, 0x19 };
static const uint8_t RX_SET_IMAGE_SIZE[] = { 0x76, 0x00, 0x31, 0x00, 0x00 };

static const uint8_t TX_ENTER_POWER_SAVING[] = { 0x56, 0x00, 0x3E, 0x03, 0x00, 0x01, 0x01 };
static const uint8_t RX_ENTER_POWER_SAVING[] = { 0x76, 0x00, 0x3E, 0x00, 0x00 };

static const uint8_t TX_EXIT_POWER_SAVING[] = { 0x56, 0x00, 0x3E, 0x03, 0x00, 0x01, 0x00 };
static const uint8_t RX_EXIT_POWER_SAVING[] = { 0x76, 0x00, 0x3E, 0x00, 0x00 };

static const uint8_t TX_CHANGE_BAUD_RATE[] = { 0x56, 0x00, 0x24, 0x03, 0x01 };
static const uint8_t RX_CHANGE_BAUD_RATE[] = { 0x76, 0x00, 0x24, 0x00, 0x00 };

LSY201::LSY201(uint16_t LED)
{
	camera=0;
	persist=0;
	currentState=reseting;
	enabled=false;
	respError=false;
	timer=0;
	downloadOffset=0;
	irLED=LED;
	pinMode(irLED,OUTPUT);
	ledOff();

	pinMode(A7,OUTPUT);

}

void t()
{
	if(digitalRead(A7)==HIGH)
		digitalWrite(A7,LOW);
	else
		digitalWrite(A7,HIGH);
}

void LSY201::ledOn()
{
	digitalWrite(irLED,LOW);
}

void LSY201::ledOff()
{
	digitalWrite(irLED,HIGH);
}

void LSY201::poll()
{
	switch (currentState)
	{
	case idle:
		break;

	case reseting:
		while(camera->available())
		{
			uint8_t b=camera->read();
			if(b==0x0a)
			{
				rxBuffer[rxPtr]='\0';
				rxPtr=0;
				if(strcmp((char *)rxBuffer,"Init end\r")==0)
				{
					currentState=settling;
				}
			}
			else
			{
				rxBuffer[rxPtr++]=b;
				if(rxPtr>32)
					rxPtr=0;
			}

		}
		break;

	case takingPic:
		while(camera->available())
		{
			rxBuffer[rxPtr++]=camera->read();

			if(rxPtr>4)
			{
				rxPtr=0;
				downloadOffset=0;
				jpegRead();
			}
		}
		break;


	case readingContent:

		while(camera->available())
		{
			//t();
			uint8_t b=camera->read();
			rxBuffer[rxPtr++]=b;

			if(rxPtr==CAMERA_CHUNK+10)
			{
				for (int n=6;n<CAMERA_CHUNK+5;n++)
				{
					if((rxBuffer[n-1]==0xff)&&(rxBuffer[n]==0xD9))
					{
						persist->store(&(rxBuffer[5]),n-4);
						timer=0;
						storeStop();
					}
				}

				if(currentState!=stopAfterStore)
				{
					currentState=storeJpg;
					storeRetry=0;
				}

			}
		}
		break;

	// Sometimes TCPClient in the persistence does not connect to server, and it needs a trip back through the main loop to connect again.  This state handles that condition.
	//  Gives up after 5 tries
	case storeJpg:
		if(persist->store(&(rxBuffer[5]),CAMERA_CHUNK))
		{
			downloadOffset+=CAMERA_CHUNK;
			jpegRead();
		}
		else
		{
			storeRetry++;
			if(storeRetry>5)
				jpegRead();
		}


		break;

	case stopAfterStore:
	case eatFiveBytes:
		while(camera->available())
		{
			rxBuffer[rxPtr++]=camera->read();
			if(rxBuffer[rxPtr-1]!=respBuf[rxPtr-1])
			{
				respError=true;
			}


			if(rxPtr>4)
			{
#ifdef DEBUG_DOOR
				if(respError)
				{
					char buffer[50];
					sprintf(buffer,"Response Error %x %x %x %x %x",rxBuffer[0],rxBuffer[1],rxBuffer[2],rxBuffer[3],rxBuffer[4]);
					Spark.publish("garagedoor-event",buffer);
				}
#endif
				rxPtr=0;
				if(currentState==stopAfterStore)
				{
					timer=0;
					currentState=closeWait;
					enabled=false;
				}
				else
				{
					currentState=idle;
					enabled=true;
				}
				respError=false;
			}
		}
		break;
	}

}

void LSY201::tick()
{
	switch(currentState)
	{
	case reseting:
		if(timer++>60)
		{
			timer=0;
			reset();
		}
		break;
	case settling:
		if(timer++>3)
		{
			currentState=idle;
			enabled=true;
		}
		break;
	case closeWait:
		if(timer++>2)
		{
			persist->close();
			enabled=true;
			currentState=idle;
		}
		break;

	//General 60 second timeout on all operations
	case takingPic:
	case readingContent:
	case storeJpg:
	case stopAfterStore:
		if(timer++>60)
		{
			enabled=false;
			timer=0;
			persist->close();
			reset();
		}
		break;

	case eatFiveBytes:
		if(timer++>60)
		{
			enabled=false;
			timer=0;
			reset();
		}
		break;
	}
}
bool LSY201::isEnabled()
{
	return enabled;
}

uint8_t* LSY201::getBuffer()
{
	return rxBuffer;
}
uint8_t LSY201::getPointer()
{
	return rxPtr;
}
void LSY201::setSerial(Stream &stream)
{
  camera = &stream;
}

void LSY201::setPersister(IPersister &p)
{
	persist=&p;
}

void LSY201::reset()
{
	enabled=false;
	tx(TX_RESET, sizeof(TX_RESET));
	currentState=reseting;
	timer=0;
}

void LSY201::takePictureAndSave()
{
	if(enabled)
	{
		timer=0;
		while(camera->available())
			camera->read();
		tx(TX_TAKE_PICTURE, sizeof(TX_TAKE_PICTURE));
		currentState=takingPic;
		rxPtr=0;
		enabled=false;
	}
//  rx(RX_TAKE_PICTURE, sizeof(RX_TAKE_PICTURE));
}

/*uint16_t LSY201::readJpegFileSize()
{
  tx(TX_READ_JPEG_FILE_SIZE, sizeof(TX_READ_JPEG_FILE_SIZE));
  rx(RX_READ_JPEG_FILE_SIZE, sizeof(RX_READ_JPEG_FILE_SIZE));

  return (((uint16_t) readByte()) << 8) | readByte();
}*/


void LSY201::jpegRead()
{
	currentState=readingContent;
	rxPtr=0;
	tx(TX_READ_JPEG_FILE_CONTENT,sizeof(TX_READ_JPEG_FILE_CONTENT));

	  uint8_t params[] = {
	    (uint8_t)((downloadOffset & 0xFF00) >> 8),
	    (uint8_t)(downloadOffset & 0x00FF),
	    0x00,
	    0x00,
	    0x00,
	    CAMERA_CHUNK,
	    0x00,
	    0x0A
	  };

	  tx(params, sizeof(params));
}

void LSY201::storeStop()
{
	currentState=stopAfterStore;
	rxPtr=0;
	timer=0;
	memcpy(respBuf,RX_STOP_TAKING_PICTURES,5);
	tx(TX_STOP_TAKING_PICTURES, sizeof(TX_STOP_TAKING_PICTURES));
}

void LSY201::setCompressionRatio(uint8_t value)
{

	if(enabled)
	{
		timer=0;
		tx(TX_SET_COMPRESSION_RATIO, sizeof(TX_SET_COMPRESSION_RATIO));
		tx(&value, 1);

		rxPtr=0;
		currentState=eatFiveBytes;
		memcpy(respBuf,RX_SET_COMPRESSION_RATIO,5);
		enabled=false;
	}
}

void LSY201::setImageSize(Size size)
{
	if(enabled)
	{
		timer=0;
		tx(TX_SET_IMAGE_SIZE, sizeof(TX_SET_IMAGE_SIZE));
		tx((uint8_t *) &size, 1);

		rxPtr=0;
		currentState=eatFiveBytes;
		memcpy(respBuf,RX_SET_IMAGE_SIZE,5);
		enabled=false;
	}
}

void LSY201::setBaudRate(unsigned long baud)
{
	if(enabled)
	{
		timer=0;
		tx(TX_CHANGE_BAUD_RATE, sizeof(TX_CHANGE_BAUD_RATE));

		uint16_t params = 0xC8AE; /* 9600 */
		switch (baud)
		{
		case 19200: params = 0xE456; break;
		case 38400: params = 0xF22A; break;
		case 57600: params = 0x4C1C; break;
		case 115200: params = 0xA60D; break;;
		};

		tx((uint8_t *) &params, 2);

		rxPtr=0;
		currentState=eatFiveBytes;
		memcpy(respBuf,RX_CHANGE_BAUD_RATE,5);
		enabled=false;
	}
}

void LSY201::stopTakingPictures()
{
	simpleCommand(TX_STOP_TAKING_PICTURES, sizeof(TX_STOP_TAKING_PICTURES),RX_STOP_TAKING_PICTURES);

}

void LSY201::enterPowerSave()
{
	simpleCommand(TX_ENTER_POWER_SAVING,sizeof(TX_ENTER_POWER_SAVING),RX_ENTER_POWER_SAVING);

}

void LSY201::exitPowerSave()
{
	simpleCommand(TX_EXIT_POWER_SAVING,sizeof(TX_EXIT_POWER_SAVING),RX_EXIT_POWER_SAVING);
}

void LSY201::tx(const uint8_t *bytes, uint8_t length)
{

  while (length --)
  {

    camera->write(*bytes);

    bytes ++;
  }

}

void LSY201::simpleCommand(const uint8_t *bytes, uint8_t length,const uint8_t *resp)
{
	if(enabled)
	{
		timer=0;
		tx(bytes, length);
		rxPtr=0;
		memcpy(respBuf,resp,5);
		currentState=eatFiveBytes;
		enabled=false;
	}

}
