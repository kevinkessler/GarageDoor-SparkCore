/*
 * NetworkSaver.cpp
 *
 *  Created on: Feb 25, 2015
 *      Author: kkessler
 */

#include "NetworkSaver.h"

NetworkSaver::NetworkSaver() {

}

NetworkSaver::~NetworkSaver() {

}

void NetworkSaver::store(uint8_t *bytes, uint8_t size)
{

	if(!client.connected())
		client.connect(server,12345);

	client.write(bytes,size);



}
void NetworkSaver::close()
{
	if(client.connected())
		client.stop();
}
