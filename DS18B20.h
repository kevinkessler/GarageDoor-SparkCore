#ifndef DS18B20_H_
#define DS18B20_H_

#include "OneWire.h"
#include "application.h"

#define MAX_NAME 8

class DS18B20{
private:
    OneWire* ds;
    byte data[12];
    byte addr[8];
    byte type_s;
    byte chiptype;
    char szName[MAX_NAME];
    
public:
    DS18B20(uint16_t pin);
    DS18B20(uint16_t pin,uint8_t *addr);

    boolean search();
    void resetsearch();
    void getROM(char szROM[]);
    byte getChipType();
    char* getChipName();
    float getTemperature();
    float convertToFahrenheit(float celsius);
};

#endif
