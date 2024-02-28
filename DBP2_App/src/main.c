#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <stdlib.h>

#include "i2c.h"
#include "linux/errno.h"

#define MPC9844_TEMP_REG 0x05
#define MPC9844_CONFIG_REG 0x1
#define SFP_MSB_TEMP_REG 0x60
#define SFP_LSB_TEMP_REG 0x61
#define INA3221_SHUNT_VOLTAGE_1_REG 0x1
#define INA3221_BUS_VOLTAGE_1_REG 0x2

int iio_event_monitor_up() {

	if(system("/run/media/mmcblk0p1/IIO_MONITOR.elf '/dev/iio:device1' &") ){
		//printf("\nError executing iio_event_monitor.\n");
		return -1;
	}
	else {
		return 0;
	}

}

float mpc9844_read_temperature(struct I2cDevice *dev) {
	int rc;
	uint8_t temp_buf[2] = {0,0};
	uint8_t temp_reg = MPC9844_TEMP_REG;
	float Temperature;

	// Write temperature address in register pointer
	rc = i2c_write(dev,&temp_reg,1);
	if(rc < 0)
		return rc;

	// Read MSB and LSB of ambient temperature
	rc = i2c_read(dev,temp_buf,2);
	if(rc < 0)
			return rc;

	temp_buf[0] = temp_buf[0] & 0x1F;	//Clear Flag bits
	if ((temp_buf[0] & 0x10) == 0x10){//TA 0°C
		temp_buf[0] = temp_buf[0] & 0x0F; //Clear SIGN
		Temperature = (temp_buf[0] * 16 + (float)temp_buf[1] / 16) - 256; //TA 0°C
	}
	else
		Temperature = (temp_buf[0] * 16 + (float)temp_buf[1] / 16); //Temperature = Ambient Temperature (°C)
	return Temperature;
}

float sfp_avago_read_temperature(struct I2cDevice *dev) {
	int rc;
	uint8_t temp_buf[2] = {0,0};
	uint8_t temp_reg = SFP_MSB_TEMP_REG;
	float Temperature;

	// Read MSB of SFP temperature
	rc = i2c_readn_reg(dev,temp_reg,temp_buf,1);
	if(rc < 0)
		return rc;

	// Read LSB of SFP temperature
	temp_reg = SFP_LSB_TEMP_REG;
	rc = i2c_readn_reg(dev,temp_reg,&temp_buf[1],1);
	if(rc < 0)
		return rc;

	Temperature = (float) ((int) (temp_buf[0] << 8)  + temp_buf[1]) / 256;
	return Temperature;
}

int xlnx_ams_read_temp(int *chan, int n, float *res){
	FILE *raw,*offset,*scale;
	for(int i=0;i<n;i++){
		char buffer [sizeof(chan[i])*8+1];
		snprintf(buffer, sizeof(buffer), "%d",chan[i]);
		char raw_str[80];
		char offset_str[80];
		char scale_str[80];

		strcpy(raw_str, "/sys/bus/iio/devices/iio:device1/in_temp");
		strcpy(offset_str, "/sys/bus/iio/devices/iio:device1/in_temp");
		strcpy(scale_str, "/sys/bus/iio/devices/iio:device1/in_temp");

		strcat(raw_str, buffer);
		strcat(offset_str, buffer);
		strcat(scale_str, buffer);

		strcat(raw_str, "_raw");
		strcat(offset_str, "_offset");
		strcat(scale_str, "_scale");

		raw = fopen(raw_str,"r");
		offset = fopen(offset_str,"r");
		scale = fopen(scale_str,"r");

		if((raw==NULL)|(offset==NULL)|(scale==NULL)){
			fclose(raw);
			fclose(offset);
			fclose(scale); 	/*Any of the files could not be opened*/
			return -1;
			}
		else{
			fseek(raw, 0, SEEK_END);
			long fsize = ftell(raw);
			fseek(raw, 0, SEEK_SET);  /* same as rewind(f); */

			char *raw_string = malloc(fsize + 1);
			fread(raw_string, fsize, 1, raw);

			fseek(offset, 0, SEEK_END);
			fsize = ftell(offset);
			fseek(offset, 0, SEEK_SET);  /* same as rewind(f); */

			char *offset_string = malloc(fsize + 1);
			fread(offset_string, fsize, 1, offset);

			fseek(scale, 0, SEEK_END);
			fsize = ftell(scale);
			fseek(scale, 0, SEEK_SET);  /* same as rewind(f); */

			char *scale_string = malloc(fsize + 1);
			fread(scale_string, fsize, 1, scale);

			float Temperature = (atof(scale_string) * (atof(raw_string) + atof(offset_string))) / 1024;
			fclose(raw);
			fclose(offset);
			fclose(scale);
			res[i] = Temperature;
			return 0;
			}
		}

	}


float ina3221_get_som_voltage(struct I2cDevice *dev){
	int rc;
	uint8_t voltage_buf[2] = {0,0};
	uint8_t voltage_reg = INA3221_BUS_VOLTAGE_1_REG;
	float voltage;

	// Write bus voltage channel 1 address in register pointer
	rc = i2c_write(dev,&voltage_reg,1);
	if(rc < 0)
		return rc;

	// Read MSB and LSB of voltage
	rc = i2c_read(dev,voltage_buf,2);
	if(rc < 0)
			return rc;
	int voltage_int = (int)(voltage_buf[0] << 8) + (voltage_buf[1]);
	voltage_int = voltage_int / 8;
	voltage = voltage_int * 8e-3;
	return voltage;
}

float ina3221_get_som_current(struct I2cDevice *dev){
	int rc;
	uint8_t voltage_buf[2] = {0,0};
	uint8_t voltage_reg = INA3221_SHUNT_VOLTAGE_1_REG;
	float voltage;

	// Write shunt voltage channel 1 address in register pointer
	rc = i2c_write(dev,&voltage_reg,1);
	if(rc < 0)
		return rc;

	// Read MSB and LSB of voltage
	rc = i2c_read(dev,voltage_buf,2);
	if(rc < 0)
			return rc;
	int voltage_int = (int)(voltage_buf[0] << 8) + (voltage_buf[1]);
	voltage_int = voltage_int / 8;
	voltage = voltage_int * 40e-6;
	return voltage / 0.05; //Shunt resistor of 0.05 Ohms in the DPB2
}

/*int main(){

		struct I2cDevice dev_pcb_temp, dev_sfp0, dev_sfp02_volt, dev_sfp35_volt, dev_sfp1, dev_sfp2, dev_sfp3, dev_sfp4,dev_sfp5,dev_mux0, dev_mux1,dev_som_volt;
		float ams_temp[64];
		float ams_volt[64];
		int ams_chan[64];
		int ams_nchan;

		dev_pcb_temp.filename = "/dev/i2c-2";
		dev_pcb_temp.addr = 0x18;
		dev_som_volt.filename = "/dev/i2c-2";
		dev_som_volt.addr = 0x40;
		dev_sfp02_volt.filename = "/dev/i2c-3";
		dev_sfp02_volt.addr = 0x40;
		dev_sfp35_volt.filename = "/dev/i2c-3";
		dev_sfp35_volt.addr = 0x41;
		dev_sfp0.filename = "/dev/i2c-6";
		dev_sfp0.addr = 0x51;
		dev_sfp1.filename = "/dev/i2c-10";
		dev_sfp1.addr = 0x51;
		dev_sfp2.filename = "/dev/i2c-8";
		dev_sfp2.addr = 0x51;
		dev_sfp3.filename = "/dev/i2c-12";
		dev_sfp3.addr = 0x51;
		dev_sfp4.filename = "/dev/i2c-9";
		dev_sfp4.addr = 0x51;
		dev_sfp5.filename = "/dev/i2c-13";
		dev_sfp5.addr = 0x51;


	return 0;
}*/
