/*
 * NetworkSaver.h
 *
 *  Created on: Feb 25, 2015
 *      Author: kkessler
 */

#ifndef CORE_FIRMWARE_APPLICATIONS_GARAGEDOOR_NETWORKSAVER_H_
#define CORE_FIRMWARE_APPLICATIONS_GARAGEDOOR_NETWORKSAVER_H_

#include "IPersister.h"
#include "application.h"
#ifdef __cplusplus
extern "C" {
#endif

class NetworkSaver: public IPersister {
private:
	TCPClient client;
	//byte server[4]={192,168,1,18};
	char server[64];
	uint16_t port=12345;

public:
	NetworkSaver();
	virtual ~NetworkSaver();
	bool store(uint8_t *bytes, uint8_t size);
	void close();
	int setServer(String serverAndPort);
};

#ifdef __cplusplus
}
#endif

#endif /* CORE_FIRMWARE_APPLICATIONS_GARAGEDOOR_NETWORKSAVER_H_ */
