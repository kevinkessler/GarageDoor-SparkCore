/*
 * IPersister.h
 *
 *  Created on: Feb 25, 2015
 *      Author: Kevin
 */

#ifndef IPERSISTER_H_
#define IPERSISTER_H_

#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

class IPersister
{
public:
	virtual ~IPersister() {}
	virtual bool store(uint8_t *bytes, uint8_t size) =0;
	virtual void close()=0;
};

#ifdef __cplusplus
}
#endif




#endif /* IPERSISTER_H_ */
