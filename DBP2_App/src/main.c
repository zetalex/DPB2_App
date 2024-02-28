/*
 * main.c
 *
 * @date
 * @author
 */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <stdlib.h>
#include <string.h>

#include "i2c.h"
#include "constants.h"
#include "linux/errno.h"



/************************** Variable Definitions *****************************/

struct DPB_I2cSensors{

	struct I2cDevice dev_pcb_temp;
	struct I2cDevice dev_sfp0_2_volt;
	struct I2cDevice dev_sfp3_5_volt;
	struct I2cDevice dev_som_volt;
	struct I2cDevice dev_sfp0_A0;
	struct I2cDevice dev_sfp1_A0;
	struct I2cDevice dev_sfp2_A0;
	struct I2cDevice dev_sfp3_A0;
	struct I2cDevice dev_sfp4_A0;
	struct I2cDevice dev_sfp5_A0;
	struct I2cDevice dev_sfp0_A2;
	struct I2cDevice dev_sfp1_A2;
	struct I2cDevice dev_sfp2_A2;
	struct I2cDevice dev_sfp3_A2;
	struct I2cDevice dev_sfp4_A2;
	struct I2cDevice dev_sfp5_A2;
	//struct I2cDevice dev_mux0;
	//struct I2cDevice dev_mux1;
};

/************************** Function Prototypes ******************************/

/************************** I2C Devices Functions ******************************/

int init_I2cSensors(){

	int rc;

	dev_pcb_temp.filename = "/dev/i2c-2";
	dev_pcb_temp.addr = 0x18;

	dev_som_volt.filename = "/dev/i2c-2";
	dev_som_volt.addr = 0x40;
	dev_sfp0_2_volt.filename = "/dev/i2c-3";
	dev_sfp0_2_volt.addr = 0x40;
	dev_sfp3_5_volt.filename = "/dev/i2c-3";
	dev_sfp3_5_volt.addr = 0x41;

	dev_sfp0_A0.filename = "/dev/i2c-6";
	dev_sfp0_A0.addr = 0x50;
	dev_sfp1_A0.filename = "/dev/i2c-10";
	dev_sfp1_A0.addr = 0x50;
	dev_sfp2_A0.filename = "/dev/i2c-8";
	dev_sfp2_A0.addr = 0x50;
	dev_sfp3_A0.filename = "/dev/i2c-12";
	dev_sfp3_A0.addr = 0x50;
	dev_sfp4_A0.filename = "/dev/i2c-9";
	dev_sfp4_A0.addr = 0x50;
	dev_sfp5_A0.filename = "/dev/i2c-13";
	dev_sfp5_A0.addr = 0x50;

	dev_sfp0_A2.filename = "/dev/i2c-6";
	dev_sfp0_A2.addr = 0x51;
	dev_sfp1_A2.filename = "/dev/i2c-10";
	dev_sfp1_A2.addr = 0x51;
	dev_sfp2_A2.filename = "/dev/i2c-8";
	dev_sfp2_A2.addr = 0x51;
	dev_sfp3_A2.filename = "/dev/i2c-12";
	dev_sfp3_A2.addr = 0x51;
	dev_sfp4_A2.filename = "/dev/i2c-9";
	dev_sfp4_A2.addr = 0x51;
	dev_sfp5_A2.filename = "/dev/i2c-13";
	dev_sfp5_A2.addr = 0x51;


	rc = i2c_start(&DPB_I2cSensors.dev_pcb_temp);
	if (rc) {
		printf("failed to start i2c device\r\n");
		return rc;
	}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp0_2_volt);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp3_5_volt);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_som_volt);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp0_A0);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp1_A0);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp2_A0);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp3_A0);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp4_A0);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp5_A0);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp0_A2);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp1_A2);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp2_A2);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp3_A2);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp4_A2);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
	rc = i2c_start(&DPB_I2cSensors.dev_sfp5_A2);
		if (rc) {
			printf("failed to start i2c device\r\n");
			return rc;
		}
}

/************************** IIO_EVENT_MONITOR Functions ******************************/

int iio_event_monitor_up() {

	if(system("/run/media/mmcblk0p1/IIO_MONITOR.elf '/dev/iio:device1' &") ){
		//printf("\nError executing iio_event_monitor.\n");
		return -1;
	}
	else {
		return 0;
	}

}

/************************** AMS Functions ******************************/
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

int xlnx_ams_read_volt(int *chan, int n, float *res){
	FILE *raw,*scale;
	for(int i=0;i<n;i++){

		char buffer [sizeof(chan[i])*8+1];
		snprintf(buffer, sizeof(buffer), "%d",chan[i]);
		char raw_str[80];
		char scale_str[80];

		strcpy(raw_str, "/sys/bus/iio/devices/iio:device1/in_voltage");
		strcpy(scale_str, "/sys/bus/iio/devices/iio:device1/in_voltage");

		strcat(raw_str, buffer);
		strcat(scale_str, buffer);

		strcat(raw_str, "_raw");
		strcat(scale_str, "_scale");

		raw = fopen(raw_str,"r");
		scale = fopen(scale_str,"r");

		if((raw==NULL)|(scale==NULL)){

			fclose(raw);
			fclose(scale); 	/*Any of the files could not be opened*/
			return -1;
			}
		else{

			fseek(raw, 0, SEEK_END);
			long fsize = ftell(raw);
			fseek(raw, 0, SEEK_SET);  /* same as rewind(f); */

			char *raw_string = malloc(fsize + 1);
			fread(raw_string, fsize, 1, raw);

			fseek(scale, 0, SEEK_END);
			fsize = ftell(scale);
			fseek(scale, 0, SEEK_SET);  /* same as rewind(f); */

			char *scale_string = malloc(fsize + 1);
			fread(scale_string, fsize, 1, scale);

			float Voltage = (atof(scale_string) * atof(raw_string)) / 1024;
			fclose(raw);
			fclose(scale);
			res[i] = Voltage;
			return 0;
			}
		}

	}

/************************** Temp.Sensor Functions ******************************/
int mpc9844_read_temperature(float *res) {
	int rc = 0;
	struct I2cDevice dev = DPB_I2cSensors.dev_pcb_temp;
	uint8_t temp_buf[2] = {0,0};
	uint8_t temp_reg = MPC9844_TEMP_REG;


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
		res[0] = (temp_buf[0] * 16 + (float)temp_buf[1] / 16) - 256; //TA 0°C
	}
	else
		res[0] = (temp_buf[0] * 16 + (float)temp_buf[1] / 16); //Temperature = Ambient Temperature (°C)
	return rc;
}

/************************** SFP Functions ******************************/
int sfp_avago_read_temperature(int n, float *res) {
	int rc = 0;
	uint8_t temp_buf[2] = {0,0};
	uint8_t temp_reg = SFP_MSB_TEMP_REG;

	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0:
			dev = DPB_I2cSensors.dev_sfp0_A2;
		break;
		case DEV_SFP1:
			dev = DPB_I2cSensors.dev_sfp1_A2;
		break;
		case DEV_SFP2:
			dev = DPB_I2cSensors.dev_sfp2_A2;
		break;
		case DEV_SFP3:
				dev = DPB_I2cSensors.dev_sfp3_A2;
			break;
		case DEV_SFP4:
				dev = DPB_I2cSensors.dev_sfp4_A2;
			break;
		case DEV_SFP5:
				dev = DPB_I2cSensors.dev_sfp5_A2;
			break;
		default:
			return -EINVAL;
		break;
		}

	// Read MSB of SFP temperature
	rc = i2c_readn_reg(dev,temp_reg,temp_buf,1);
	if(rc < 0)
		return rc;

	// Read LSB of SFP temperature
	temp_reg = SFP_LSB_TEMP_REG;
	rc = i2c_readn_reg(dev,temp_reg,&temp_buf[1],1);
	if(rc < 0)
		return rc;

	res [0] = (float) ((int) (temp_buf[0] << 8)  + temp_buf[1]) / 256;
	return rc;
}

int sfp_avago_read_voltage(int n, float *res) {
	int rc = 0;
	uint8_t voltage_buf[2] = {0,0};
	uint8_t voltage_reg = SFP_MSB_VCC_REG;

	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0:
			dev = DPB_I2cSensors.dev_sfp0_A2;
		break;
		case DEV_SFP1:
			dev = DPB_I2cSensors.dev_sfp1_A2;
		break;
		case DEV_SFP2:
			dev = DPB_I2cSensors.dev_sfp2_A2;
		break;
		case DEV_SFP3:
				dev = DPB_I2cSensors.dev_sfp3_A2;
			break;
		case DEV_SFP4:
				dev = DPB_I2cSensors.dev_sfp4_A2;
			break;
		case DEV_SFP5:
				dev = DPB_I2cSensors.dev_sfp5_A2;
			break;
		default:
			return -EINVAL;
		break;
		}

	// Read MSB of SFP VCC
	rc = i2c_readn_reg(dev,voltage_reg,voltage_buf,1);
	if(rc < 0)
		return rc;

	// Read LSB of SFP VCC
	voltage_reg = SFP_LSB_VCC_REG;
	rc = i2c_readn_reg(dev,voltage_reg,&voltage_buf[1],1);
	if(rc < 0)
		return rc;

	res [0] = (float) ((uint16_t) (voltage_buf[0] << 8)  + voltage_buf[1]) * 1e-4;
	return rc;
}

int sfp_avago_read_lbias_current(int n, float *res) {
	int rc = 0;
	uint8_t current_reg[2] = {0,0};
	uint8_t current_buf = SFP_MSB_TXBIAS_REG;

	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0:
			dev = DPB_I2cSensors.dev_sfp0_A2;
		break;
		case DEV_SFP1:
			dev = DPB_I2cSensors.dev_sfp1_A2;
		break;
		case DEV_SFP2:
			dev = DPB_I2cSensors.dev_sfp2_A2;
		break;
		case DEV_SFP3:
				dev = DPB_I2cSensors.dev_sfp3_A2;
			break;
		case DEV_SFP4:
				dev = DPB_I2cSensors.dev_sfp4_A2;
			break;
		case DEV_SFP5:
				dev = DPB_I2cSensors.dev_sfp5_A2;
			break;
		default:
			return -EINVAL;
		break;
		}

	// Read MSB of SFP Laser Bias Current
	rc = i2c_readn_reg(dev,current_reg,current_buf,1);
	if(rc < 0)
		return rc;

	// Read LSB of SFP Laser Bias Current
	current_reg = SFP_LSB_TXBIAS_REG;
	rc = i2c_readn_reg(dev,current_reg,&current_buf[1],1);
	if(rc < 0)
		return rc;

	res [0] = (float) ((uint16_t) (current_buf[0] << 8)  + current_buf[1]) * 2e-6;
	return rc;
}

int sfp_avago_read_tx_av_optical_pwr(int n, float *res) {
	int rc = 0;
	uint8_t power_reg[2] = {0,0};
	uint8_t power_buf = SFP_MSB_TXPWR_REG;

	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0:
			dev = DPB_I2cSensors.dev_sfp0_A2;
		break;
		case DEV_SFP1:
			dev = DPB_I2cSensors.dev_sfp1_A2;
		break;
		case DEV_SFP2:
			dev = DPB_I2cSensors.dev_sfp2_A2;
		break;
		case DEV_SFP3:
				dev = DPB_I2cSensors.dev_sfp3_A2;
			break;
		case DEV_SFP4:
				dev = DPB_I2cSensors.dev_sfp4_A2;
			break;
		case DEV_SFP5:
				dev = DPB_I2cSensors.dev_sfp5_A2;
			break;
		default:
			return -EINVAL;
		break;
		}

	// Read MSB of SFP Laser Bias Current
	rc = i2c_readn_reg(dev,power_reg,power_buf,1);
	if(rc < 0)
		return rc;

	// Read LSB of SFP Laser Bias Current
	power_reg = SFP_LSB_TXPWR_REG;
	rc = i2c_readn_reg(dev,power_reg,&power_buf[1],1);
	if(rc < 0)
		return rc;

	res [0] = (float) ((uint16_t) (power_buf[0] << 8)  + power_buf[1]) * 1e-7;
	return rc;
}

int sfp_avago_read_rx_av_optical_pwr(int n, float *res) {
	int rc = 0;
	uint8_t power_reg[2] = {0,0};
	uint8_t power_buf = SFP_MSB_RXPWR_REG;

	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0:
			dev = DPB_I2cSensors.dev_sfp0_A2;
		break;
		case DEV_SFP1:
			dev = DPB_I2cSensors.dev_sfp1_A2;
		break;
		case DEV_SFP2:
			dev = DPB_I2cSensors.dev_sfp2_A2;
		break;
		case DEV_SFP3:
				dev = DPB_I2cSensors.dev_sfp3_A2;
			break;
		case DEV_SFP4:
				dev = DPB_I2cSensors.dev_sfp4_A2;
			break;
		case DEV_SFP5:
				dev = DPB_I2cSensors.dev_sfp5_A2;
			break;
		default:
			return -EINVAL;
		break;
		}

	// Read MSB of SFP Laser Bias Current
	rc = i2c_readn_reg(dev,power_reg,power_buf,1);
	if(rc < 0)
		return rc;

	// Read LSB of SFP Laser Bias Current
	power_reg = SFP_LSB_RXPWR_REG;
	rc = i2c_readn_reg(dev,power_reg,&power_buf[1],1);
	if(rc < 0)
		return rc;

	res [0] = (float) ((uint16_t) (power_buf[0] << 8)  + power_buf[1]) * 1e-7;
	return rc;
}

/************************** Volt. and Curr. Sensor Functions ******************************/
int ina3221_get_voltage(int n, float *res){
	int rc = 0;
	uint8_t voltage_buf[2] = {0,0};
	uint8_t voltage_reg = INA3221_BUS_VOLTAGE_1_REG;
	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0_2_VOLT:
			dev = DPB_I2cSensors.dev_sfp0_2_volt;
		break;
		case DEV_SFP3_5_VOLT:
			dev = DPB_I2cSensors.dev_sfp3_5_volt;
		break;
		case DEV_SOM_VOLT:
			dev = DPB_I2cSensors.dev_som_volt;
		break;
		default:
			return -EINVAL;
		break;
	}
	for (int i=0;i<3;i++){
		voltage_reg = voltage_reg + 2*i;
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
		res[i] = voltage_int * 8e-3;
	}
	return rc;
}

int ina3221_get_current(int n, float *res){
	int rc = 0;
	uint8_t voltage_buf[2] = {0,0};
	uint8_t voltage_reg = INA3221_SHUNT_VOLTAGE_1_REG;
	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0_2_VOLT:
				dev = DPB_I2cSensors.dev_sfp0_2_volt;
		break;
		case DEV_SFP3_5_VOLT:
				dev = DPB_I2cSensors.dev_sfp3_5_volt;
		break;
		case DEV_SOM_VOLT:
				dev = DPB_I2cSensors.dev_som_volt;
		break;
		default:
			return -EINVAL;
		break;
		}
	for (int i=0;i<3;i++){
		voltage_reg = voltage_reg + 2*i;
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
		res[i] = (voltage_int * 40e-6) / 0.05 ;
		}
	return rc;
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
