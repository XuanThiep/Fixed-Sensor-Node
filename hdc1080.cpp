#include "hdc1080.h"
#include <Wire.h>


hdc1080::hdc1080(void)
{

}

void hdc1080::Init(Temp_Reso Temperature_Resolution_x_bit,Humi_Reso Humidity_Resolution_x_bit)
{
	/* Temperature and Humidity are acquired in sequence, Temperature first
	 * Default:   Temperature resolution = 14 bit,
	 *            Humidity resolution = 14 bit
	 */

	/* Set the acquisition mode to measure both temperature and humidity by setting Bit[12] to 1 */
	unsigned int config_reg_value=0x1000;
	unsigned char data_send[2];

	if(Temperature_Resolution_x_bit == Temperature_Resolution_11_bit)
	{
		config_reg_value |= (1<<10); //11 bit
	}

	switch(Humidity_Resolution_x_bit)
	{
	case Humidity_Resolution_11_bit:
		config_reg_value|= (1<<8);
		break;
	case Humidity_Resolution_8_bit:
		config_reg_value|= (1<<9);
		break;
	}

	data_send[0]= (config_reg_value>>8);
	data_send[1]= (config_reg_value&0x00ff);

	Wire.begin();

	Wire.beginTransmission(HDC_1080_ADD);
	Wire.write(CONFIGURATION);
	Wire.write(data_send[0]);
	Wire.write(data_send[1]);
	Wire.endTransmission();

}

void hdc1080::Start_measurement(float* temperature, unsigned char* humidity)
{
	unsigned char receive_data[4];
	unsigned int temp=0,humi=0;
	
	Wire.beginTransmission(HDC_1080_ADD);
	Wire.write(TEMPERATURE);
	Wire.endTransmission();
	
	delay(14);

	Wire.requestFrom((int)HDC_1080_ADD,(int)4);

	for(unsigned char i=0;i<4;i++)
	{
		receive_data[i] = Wire.read();
	}

	temp=((receive_data[0]<<8)|receive_data[1]);
	humi=((receive_data[2]<<8)|receive_data[3]);

	*temperature=((temp/65536.0)*165.0)-40.0;
	*humidity=(uint8_t)((humi/65536.0)*100.0);
}
