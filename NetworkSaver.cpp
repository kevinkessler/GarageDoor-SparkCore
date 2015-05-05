/*
 * NetworkSaver.cpp
 *
 *  Created on: Feb 25, 2015
 *      Author: kkessler
 */

#include "NetworkSaver.h"

NetworkSaver::NetworkSaver(){

	strcpy(server,"127.0.0.1");
}

NetworkSaver::~NetworkSaver() {

}

bool NetworkSaver::store(uint8_t *bytes, uint8_t size)
{
	uint8_t retval;
	char buffer[50];

	if(!client.connected())
	{

		retval=client.connect(server,port);
		sprintf(buffer,"Connect to %s:%d %d",server,port,retval);
		Spark.publish("garagedoor-event",buffer);
		if(retval!=1)
		{
			client.stop();
			return false;
		}
	}

	client.write(bytes,size);
	return true;

}

int NetworkSaver::setServer(String serverPort)
{
	char buffer[50];
	uint8_t pos;

	if((pos=serverPort.indexOf(':'))!=-1)
	{
		port=atoi(serverPort.substring(pos+1).c_str());
		strncpy(server,serverPort.c_str(),pos);
		server[pos]='\0';

	}
	else
	{
		port=12345;
		strcpy(server,serverPort.c_str());
	}

	sprintf(buffer,"Server Set to %s:%d",server,port);
	Spark.publish("garagedoor-event",buffer);

	return 0;
}

void NetworkSaver::close()
{

	client.stop();

	Spark.publish("garagedoor-event","Disconnect");

}
