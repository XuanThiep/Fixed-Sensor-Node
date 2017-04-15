#ifndef __HDC1080_H
#define __HDC1080_H

#include "Arduino.h"


#define         HDC_1080_ADD               0x40

typedef enum 
{
	TEMPERATURE = 0x00,
	HUMIDITY = 0x01,
	CONFIGURATION = 0x02
}HDC1080_REGISTER;

/* Define temperature resolution */
typedef enum
{
	Temperature_Resolution_14_bit = 0,
	Temperature_Resolution_11_bit = 1
}Temp_Reso;

/* Define humidity resolution */
typedef enum
{
	Humidity_Resolution_14_bit = 0,
	Humidity_Resolution_11_bit = 1,
	Humidity_Resolution_8_bit =2
}Humi_Reso;

class hdc1080
{
public:
	hdc1080(void);
	void Init(Temp_Reso Temperature_Resolution_x_bit,Humi_Reso Humidity_Resolution_x_bit);
	void Start_measurement(float* temperature, unsigned char* humidity);
	
};


#endif
