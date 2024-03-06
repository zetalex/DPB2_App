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
#include "timer.h"
#include <semaphore.h>
#include <sys/shm.h>

#include "i2c.h"
#include "constants.h"
#include "linux/errno.h"



/************************** Main Struct Definition *****************************/

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

int memoryID;
struct wrapper *memory;
/************************** Function Prototypes ******************************/

/************************** I2C Devices Functions ******************************/

int init_tempSensor (struct I2cDevice *dev) {
	int rc = 0;
	uint8_t manID_buf[2] = {0,0};
	uint8_t manID_reg = MPC9844_MANUF_ID_REG;
	uint8_t devID_buf[2] = {0,0};
	uint8_t devID_reg = MPC9844_DEVICE_ID_REG;

	rc = i2c_start(dev);
		if (rc) {
			return rc;
		}
	// Write Manufacturer ID address in register pointer
	rc = i2c_write(dev,&manID_reg,1);
	if(rc < 0)
		return rc;

	// Read MSB and LSB of Manufacturer ID
	rc = i2c_read(dev,manID_buf,2);
	if(rc < 0)
			return rc;
	if(!((manID_buf[0] == 0x00) && (manID_buf[1] == 0x54))){
		rc = -1;
		printf("Manufacturer ID does not match the corresponding device: Temperature Sensor\r\n");
		return rc;
	}

	// Write Device ID address in register pointer
	rc = i2c_write(dev,&devID_reg,1);
	if(rc < 0)
		return rc;

	// Read MSB and LSB of Device ID
	rc = i2c_read(dev,devID_buf,2);
	if(rc < 0)
			return rc;
	if(!((devID_buf[0] == 0x06) && (devID_buf[1] == 0x01))){
			rc = -1;
			printf("Device ID does not match the corresponding device: Temperature Sensor\r\n");
			return rc;
	}
	return 0;

}

int init_voltSensor (struct I2cDevice *dev) {
	int rc = 0;
	uint8_t manID_buf[2] = {0,0};
	uint8_t manID_reg = INA3221_MANUF_ID_REG;
	uint8_t devID_buf[2] = {0,0};
	uint8_t devID_reg = INA3221_DIE_ID_REG;

	rc = i2c_start(dev);
		if (rc) {
			return rc;
		}
	// Write Manufacturer ID address in register pointer
	rc = i2c_write(dev,&manID_reg,1);
	if(rc < 0)
		return rc;

	// Read MSB and LSB of Manufacturer ID
	rc = i2c_read(dev,manID_buf,2);
	if(rc < 0)
			return rc;
	if(!((manID_buf[0] == 0x54) && (manID_buf[1] == 0x49))){
		rc = -1;
		printf("Manufacturer ID does not match the corresponding device: Voltage Sensor\r\n");
		return rc;
	}

	// Write Device ID address in register pointer
	rc = i2c_write(dev,&devID_reg,1);
	if(rc < 0)
		return rc;

	// Read MSB and LSB of Device ID
	rc = i2c_read(dev,devID_buf,2);
	if(rc < 0)
			return rc;
	if(!((devID_buf[0] == 0x32) && (devID_buf[1] == 0x20))){
			rc = -1;
			printf("Device ID does not match the corresponding device: Voltage Sensor\r\n");
			return rc;
	}
	return 0;

}

int checksum_check(struct I2cDevice *dev,uint8_t ini_reg, uint8_t checksum_val, int size){
	int rc = 0;
	int sum = 0;
	uint8_t byte_buf[size] ;

	rc = i2c_readn_reg(dev,ini_reg,byte_buf,1);
			if(rc < 0)
				return rc;
	for(int n=1;n<size;n++){
	ini_reg ++;
	rc = i2c_readn_reg(dev,ini_reg,&byte_buf[n],1);
		if(rc < 0)
			return rc;
	}

	for(int i=0;i<size;i++){
		sum += byte_buf[i];
	}
	uint8_t calc_checksum = (sum & 0xFF);

	if (checksum_val != calc_checksum){
		rc = -1;
		printf("Checksum value does not match the expected value \r\n");
		return rc;
	}
	return 0;
}

int init_SFP_A0(struct I2cDevice *dev) {
	int rc = 0;
	uint8_t SFPphys_reg = SFP_PHYS_DEV;
	uint8_t SFPphys_buf[2] = {0,0};

	rc = i2c_start(dev);
		if (rc) {
			return rc;
		}
	rc = i2c_readn_reg(dev,SFPphys_reg,SFPphys_buf,1);
		if(rc < 0)
			return rc;


	SFPphys_reg = SFP_FUNCT;
	rc = i2c_readn_reg(dev,SFPphys_reg,&SFPphys_buf[1],1);
	if(rc < 0)
			return rc;
	if(!((SFPphys_buf[0] == 0x03) && (SFPphys_buf[1] == 0x04))){
			rc = -1;
			printf("Device ID does not match the corresponding device: SFP-Avago\r\n");
			return rc;
	}
	rc = checksum_check(dev, SFP_PHYS_DEV,0x7F,63);
	if(rc < 0)
				return rc;
	rc = checksum_check(dev, SFP_CHECKSUM2_A0,0xFA,31);
	if(rc < 0)
				return rc;
	return 0;

}
int init_SFP_A2(struct I2cDevice *dev) {
	int rc = 0;

	rc = i2c_start(dev);
		if (rc) {
			return rc;
		}
	rc = checksum_check(dev,SFP_MSB_HTEMP_ALARM_REG,0x61,95);
	if(rc < 0)
				return rc;
	return 0;

}

int init_I2cSensors(struct DPB_I2cSensors *data){

	int rc;
	data->dev_pcb_temp.filename = "/dev/i2c-2";
	data->dev_pcb_temp.addr = 0x18;

	data->dev_som_volt.filename = "/dev/i2c-2";
	data->dev_som_volt.addr = 0x40;
	data->dev_sfp0_2_volt.filename = "/dev/i2c-3";
	data->dev_sfp0_2_volt.addr = 0x40;
	data->dev_sfp3_5_volt.filename = "/dev/i2c-3";
	data->dev_sfp3_5_volt.addr = 0x41;

	data->dev_sfp0_A0.filename = "/dev/i2c-6";
	data->dev_sfp0_A0.addr = 0x50;
	data->dev_sfp1_A0.filename = "/dev/i2c-10";
	data->dev_sfp1_A0.addr = 0x50;
	data->dev_sfp2_A0.filename = "/dev/i2c-8";
	data->dev_sfp2_A0.addr = 0x50;
	data->dev_sfp3_A0.filename = "/dev/i2c-12";
	data->dev_sfp3_A0.addr = 0x50;
	data->dev_sfp4_A0.filename = "/dev/i2c-9";
	data->dev_sfp4_A0.addr = 0x50;
	data->dev_sfp5_A0.filename = "/dev/i2c-13";
	data->dev_sfp5_A0.addr = 0x50;

	data->dev_sfp0_A2.filename = "/dev/i2c-6";
	data->dev_sfp0_A2.addr = 0x51;
	data->dev_sfp1_A2.filename = "/dev/i2c-10";
	data->dev_sfp1_A2.addr = 0x51;
	data->dev_sfp2_A2.filename = "/dev/i2c-8";
	data->dev_sfp2_A2.addr = 0x51;
	data->dev_sfp3_A2.filename = "/dev/i2c-12";
	data->dev_sfp3_A2.addr = 0x51;
	data->dev_sfp4_A2.filename = "/dev/i2c-9";
	data->dev_sfp4_A2.addr = 0x51;
	data->dev_sfp5_A2.filename = "/dev/i2c-13";
	data->dev_sfp5_A2.addr = 0x51;


	rc = init_tempSensor(&data->dev_pcb_temp);
	if (rc) {
		printf("Failed to start i2c device: Temp. Sensor\r\n");
		return rc;
	}

	rc = init_voltSensor(&data->dev_sfp0_2_volt);
	if (rc) {
		printf("Failed to start i2c device: SFP 0-2 Voltage Sensor\r\n");
		return rc;
	}

	rc = init_voltSensor(&data->dev_sfp3_5_volt);
	if (rc) {
		printf("Failed to start i2c device: SFP 3-5 Voltage Sensor\r\n");
		return rc;
	}

	rc = init_voltSensor(&data->dev_som_volt);
	if (rc) {
		printf("Failed to start i2c device: SoM Voltage Sensor\r\n");
		return rc;
	}

	rc = init_SFP_A0(&data->dev_sfp0_A0);
		if (rc) {
			printf("Failed to start i2c device: SFP 0 - EEPROM page A0h\r\n");
			return rc;
		}
	rc = init_SFP_A2(&data->dev_sfp0_A2);
		if (rc) {
			printf("Failed to start i2c device: SFP 0 - EEPROM page A2h\r\n");
			return rc;
		}
	/*rc = init_SFP_A0(&data->dev_sfp1_A0);
		if (rc) {
			printf("Failed to start i2c device: SFP 1 - EEPROM page A0h\r\n");
			return rc;
		}
	rc = init_SFP_A2(&data->dev_sfp1_A2);
		if (rc) {
			printf("Failed to start i2c device: SFP 1 - EEPROM page A2h\r\n");
			return rc;
		}
	rc = init_SFP_A0(&data->dev_sfp0_A2);
		if (rc) {
			printf("Failed to start i2c device: SFP 2 - EEPROM page A0h\r\n");
			return rc;
		}
	rc = init_SFP_A2(&data->dev_sfp2_A2);
		if (rc) {
			printf("Failed to start i2c device: SFP 2 - EEPROM page A2h\r\n");
			return rc;
		}
	rc = init_SFP_A0(&data->dev_sfp3_A0);
		if (rc) {
			printf("Failed to start i2c device: SFP 3 - EEPROM page A0h\r\n");
			return rc;
		}
	rc = init_SFP_A2(&data->dev_sfp3_A2);
		if (rc) {
			printf("Failed to start i2c device: SFP 3 - EEPROM page A2h\r\n");
			return rc;
		}
	rc = init_SFP_A0(&data->dev_sfp4_A0);
		if (rc) {
			printf("Failed to start i2c device: SFP 4 - EEPROM page A0h\r\n");
			return rc;
		}
	rc = init_SFP_A2(&data->dev_sfp4_A2);
		if (rc) {
			printf("Failed to start i2c device: SFP 4 - EEPROM page A2h\r\n");
			return rc;
		}
	rc = init_SFP_A0(&data->dev_sfp5_A0);
		if (rc) {
			printf("Failed to start i2c device: SFP 5 - EEPROM page A0h\r\n");
			return rc;
		}
	rc = init_SFP_A2(&data->dev_sfp5_A2);
		if (rc) {
			printf("Failed to start i2c device: SFP 5 - EEPROM page A2h\r\n");
			return rc;
		}*/
	return 0;
}

int stop_I2cSensors(struct DPB_I2cSensors *data){

	i2c_stop(&data->dev_pcb_temp);

	i2c_stop(&data->dev_sfp0_2_volt);
	i2c_stop(&data->dev_sfp3_5_volt);
	i2c_stop(&data->dev_som_volt);

	i2c_stop(&data->dev_sfp0_A0);
	i2c_stop(&data->dev_sfp1_A0);
	i2c_stop(&data->dev_sfp2_A0);
	i2c_stop(&data->dev_sfp3_A0);
	i2c_stop(&data->dev_sfp4_A0);
	i2c_stop(&data->dev_sfp5_A0);

	i2c_stop(&data->dev_sfp0_A2);
	i2c_stop(&data->dev_sfp1_A2);
	i2c_stop(&data->dev_sfp2_A2);
	i2c_stop(&data->dev_sfp3_A2);
	i2c_stop(&data->dev_sfp4_A2);
	i2c_stop(&data->dev_sfp5_A2);

	return 0;
}
/************************** IIO_EVENT_MONITOR Functions ******************************/

int iio_event_monitor_up(FILE *proc) {

	proc = popen("/run/media/mmcblk0p1/IIO_MONITOR.elf -a iio:device0 &", "r");
	if (proc == NULL){
		printf("\nError executing iio_event_monitor.\n");
		return -1;
	}
	return 0;
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

		strcpy(raw_str, "/sys/bus/iio/devices/iio:device0/in_temp");
		strcpy(offset_str, "/sys/bus/iio/devices/iio:device0/in_temp");
		strcpy(scale_str, "/sys/bus/iio/devices/iio:device0/in_temp");

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
			//return 0;
			}
		}
		return 0;
	}

int xlnx_ams_read_volt(int *chan, int n, float *res){
	FILE *raw,*scale;
	for(int i=0;i<n;i++){

		char buffer [sizeof(chan[i])*8+1];
		snprintf(buffer, sizeof(buffer), "%d",chan[i]);
		char raw_str[80];
		char scale_str[80];

		strcpy(raw_str, "/sys/bus/iio/devices/iio:device0/in_voltage");
		strcpy(scale_str, "/sys/bus/iio/devices/iio:device0/in_voltage");

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
			//return 0;
			}
		}
	return 0;
	}

/************************** Temp.Sensor Functions ******************************/
int mpc9844_read_temperature(struct DPB_I2cSensors *data,float *res) {
	int rc = 0;
	struct I2cDevice dev = data->dev_pcb_temp;
	uint8_t temp_buf[2] = {0,0};
	uint8_t temp_reg = MPC9844_TEMP_REG;


	// Write temperature address in register pointer
	rc = i2c_write(&dev,&temp_reg,1);
	if(rc < 0)
		return rc;

	// Read MSB and LSB of ambient temperature
	rc = i2c_read(&dev,temp_buf,2);
	if(rc < 0)
			return rc;

	temp_buf[0] = temp_buf[0] & 0x1F;	//Clear Flag bits
	if ((temp_buf[0] & 0x10) == 0x10){//TA 0°C
		temp_buf[0] = temp_buf[0] & 0x0F; //Clear SIGN
		res[0] = (temp_buf[0] * 16 + (float)temp_buf[1] / 16) - 256; //TA 0°C
	}
	else
		res[0] = (temp_buf[0] * 16 + (float)temp_buf[1] / 16); //Temperature = Ambient Temperature (°C)
	return 0;
}

int mpc9844_interruptions(uint8_t flag_buf){
	if((flag_buf & 0x80) == 0x80){
		printf("CRITICAL!!! The ambient temperature has exceeded the established critical limit.");
	}
	if((flag_buf & 0x40) == 0x40){
		printf("WARNING!!! The ambient temperature has exceeded the established high limit.");
	}
	if((flag_buf & 0x20) == 0x20){
		printf("WARNING!!! The ambient temperature has exceeded the established low limit.");
	}
	return 0;
}

int mpc9844_read_alarms(struct DPB_I2cSensors *data) {
	int rc = 0;
	struct I2cDevice dev = data->dev_pcb_temp;
	uint8_t alarm_buf[1] = {0};
	uint8_t alarm_reg = MPC9844_TEMP_REG;


	// Write temperature address in register pointer
	rc = i2c_write(&dev,&alarm_reg,1);
	if(rc < 0)
		return rc;

	// Read MSB and LSB of ambient temperature
	rc = i2c_read(&dev,alarm_buf,1);
	if(rc < 0)
			return rc;
	if((alarm_buf[0] & 0xE0) != 0){
		mpc9844_interruptions(alarm_buf[0]);
	}
	alarm_buf[0] = alarm_buf[0] & 0x1F;	//Clear Flag bits
	return 0;
}

/************************** SFP Functions ******************************/
int sfp_avago_read_temperature(struct DPB_I2cSensors *data,int n, float *res) {
	int rc = 0;
	uint8_t temp_buf[2] = {0,0};
	uint8_t temp_reg = SFP_MSB_TEMP_REG;

	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0:
			dev = data->dev_sfp0_A2;
		break;
		case DEV_SFP1:
			dev = data->dev_sfp1_A2;
		break;
		case DEV_SFP2:
			dev = data->dev_sfp2_A2;
		break;
		case DEV_SFP3:
				dev = data->dev_sfp3_A2;
			break;
		case DEV_SFP4:
				dev = data->dev_sfp4_A2;
			break;
		case DEV_SFP5:
				dev = data->dev_sfp5_A2;
			break;
		default:
			return -EINVAL;
		break;
		}

	// Read MSB of SFP temperature
	rc = i2c_readn_reg(&dev,temp_reg,temp_buf,1);
	if(rc < 0)
		return rc;

	// Read LSB of SFP temperature
	temp_reg = SFP_LSB_TEMP_REG;
	rc = i2c_readn_reg(&dev,temp_reg,&temp_buf[1],1);
	if(rc < 0)
		return rc;

	res [0] = (float) ((int) (temp_buf[0] << 8)  + temp_buf[1]) / 256;
	return 0;
}

int sfp_avago_read_voltage(struct DPB_I2cSensors *data,int n, float *res) {
	int rc = 0;
	uint8_t voltage_buf[2] = {0,0};
	uint8_t voltage_reg = SFP_MSB_VCC_REG;

	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0:
			dev = data->dev_sfp0_A2;
		break;
		case DEV_SFP1:
			dev = data->dev_sfp1_A2;
		break;
		case DEV_SFP2:
			dev = data->dev_sfp2_A2;
		break;
		case DEV_SFP3:
				dev = data->dev_sfp3_A2;
			break;
		case DEV_SFP4:
				dev = data->dev_sfp4_A2;
			break;
		case DEV_SFP5:
				dev = data->dev_sfp5_A2;
			break;
		default:
			return -EINVAL;
		break;
		}

	// Read MSB of SFP VCC
	rc = i2c_readn_reg(&dev,voltage_reg,voltage_buf,1);
	if(rc < 0)
		return rc;

	// Read LSB of SFP VCC
	voltage_reg = SFP_LSB_VCC_REG;
	rc = i2c_readn_reg(&dev,voltage_reg,&voltage_buf[1],1);
	if(rc < 0)
		return rc;

	res [0] = (float) ((uint16_t) (voltage_buf[0] << 8)  + voltage_buf[1]) * 1e-4;
	return 0;
}

int sfp_avago_read_lbias_current(struct DPB_I2cSensors *data,int n, float *res) {
	int rc = 0;
	uint8_t current_buf[2] = {0,0};
	uint8_t current_reg = SFP_MSB_TXBIAS_REG;

	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0:
			dev = data->dev_sfp0_A2;
		break;
		case DEV_SFP1:
			dev = data->dev_sfp1_A2;
		break;
		case DEV_SFP2:
			dev = data->dev_sfp2_A2;
		break;
		case DEV_SFP3:
				dev = data->dev_sfp3_A2;
			break;
		case DEV_SFP4:
				dev = data->dev_sfp4_A2;
			break;
		case DEV_SFP5:
				dev = data->dev_sfp5_A2;
			break;
		default:
			return -EINVAL;
		break;
		}

	// Read MSB of SFP Laser Bias Current
	rc = i2c_readn_reg(&dev,current_reg,current_buf,1);
	if(rc < 0)
		return rc;

	// Read LSB of SFP Laser Bias Current
	current_reg = SFP_LSB_TXBIAS_REG;
	rc = i2c_readn_reg(&dev,current_reg,&current_buf[1],1);
	if(rc < 0)
		return rc;

	res[0] = (float) ((uint16_t) (current_buf[0] << 8)  + current_buf[1]) * 2e-6;
	return 0;
}

int sfp_avago_read_tx_av_optical_pwr(struct DPB_I2cSensors *data,int n, float *res) {
	int rc = 0;
	uint8_t power_buf[2] = {0,0};
	uint8_t power_reg = SFP_MSB_TXPWR_REG;

	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0:
			dev = data->dev_sfp0_A2;
		break;
		case DEV_SFP1:
			dev = data->dev_sfp1_A2;
		break;
		case DEV_SFP2:
			dev = data->dev_sfp2_A2;
		break;
		case DEV_SFP3:
				dev = data->dev_sfp3_A2;
			break;
		case DEV_SFP4:
				dev = data->dev_sfp4_A2;
			break;
		case DEV_SFP5:
				dev = data->dev_sfp5_A2;
			break;
		default:
			return -EINVAL;
		break;
		}

	// Read MSB of SFP Laser Bias Current
	rc = i2c_readn_reg(&dev,power_reg,power_buf,1);
	if(rc < 0)
		return rc;

	// Read LSB of SFP Laser Bias Current
	power_reg = SFP_LSB_TXPWR_REG;
	rc = i2c_readn_reg(&dev,power_reg,&power_buf[1],1);
	if(rc < 0)
		return rc;

	res[0] = (float) ((uint16_t) (power_buf[0] << 8)  + power_buf[1]) * 1e-7;
	return 0;
}

int sfp_avago_read_rx_av_optical_pwr(struct DPB_I2cSensors *data,int n, float *res) {
	int rc = 0;
	uint8_t power_buf[2] = {0,0};
	uint8_t power_reg = SFP_MSB_RXPWR_REG;

	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0:
			dev = data->dev_sfp0_A2;
		break;
		case DEV_SFP1:
			dev = data->dev_sfp1_A2;
		break;
		case DEV_SFP2:
			dev = data->dev_sfp2_A2;
		break;
		case DEV_SFP3:
				dev = data->dev_sfp3_A2;
			break;
		case DEV_SFP4:
				dev = data->dev_sfp4_A2;
			break;
		case DEV_SFP5:
				dev = data->dev_sfp5_A2;
			break;
		default:
			return -EINVAL;
		break;
		}

	// Read MSB of SFP Laser Bias Current
	rc = i2c_readn_reg(&dev,power_reg,power_buf,1);
	if(rc < 0)
		return rc;

	// Read LSB of SFP Laser Bias Current
	power_reg = SFP_LSB_RXPWR_REG;
	rc = i2c_readn_reg(&dev,power_reg,&power_buf[1],1);
	if(rc < 0)
		return rc;

	res[0] = (float) ((uint16_t) (power_buf[0] << 8)  + power_buf[1]) * 1e-7;
	return 0;
}

int sfp_avago_status_interruptions(uint8_t status, int n){

	if((status & 0x02) != 0){
		printf("Reception Loss of Signal in SFP:%d",n);
	}
	if((status & 0x04) != 0){
		printf("Transmitter Fault in SFP:%d",n);
	}
	return 0;
}

int sfp_avago_alarms_interruptions(uint16_t flags, int n){

	if((flags & 0x0080) == 0x0080){
		printf("WARNING!!! Received average optical power exceeds high alarm threshold in SFP:%d",n);
	}
	if((flags & 0x0040) == 0x0040){
		printf("WARNING!!! Received average optical power exceeds low alarm threshold in SFP:%d",n);
	}
	if((flags & 0x0200) == 0x0200){
		printf("WARNING!!! Transmitted average optical power exceeds high alarm threshold in SFP:%d",n);
	}
	if((flags & 0x0100) == 0x0100){
		printf("WARNING!!! Transmitted average optical power exceeds low alarm threshold in SFP:%d",n);
	}
	if((flags & 0x0800) == 0x0800){
		printf("WARNING!!! Transceiver laser bias current exceeds high alarm threshold in SFP:%d",n);
	}
	if((flags & 0x0400) == 0x0400){
		printf("WARNING!!! Transceiver laser bias current exceeds low alarm threshold in SFP:%d",n);
	}
	if((flags & 0x2000) == 0x2000){
		printf("WARNING!!! Transceiver internal supply voltage exceeds high alarm threshold in SFP:%d",n);
	}
	if((flags & 0x1000) == 0x1000){
		printf("WARNING!!! Transceiver internal supply voltage exceeds low alarm threshold in SFP:%d",n);
	}
	if((flags & 0x8000) == 0x8000){
		printf("WARNING!!! Transceiver internal temperature exceeds high alarm threshold in SFP:%d",n);
	}
	if((flags & 0x4000) == 0x4000){
		printf("WARNING!!! Transceiver internal temperature exceeds low alarm threshold in SFP:%d",n);
	}
	return 0;
}

int sfp_avago_read_alarms(struct DPB_I2cSensors *data,int n) {
	int rc = 0;
	uint8_t flag_buf[2] = {0,0};
	uint8_t status_buf[1] = {0};
	uint8_t status_reg = SFP_STAT_REG;
	uint8_t flag_reg = SFP_FLG1_REG;

	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0:
			dev = data->dev_sfp0_A2;
		break;
		case DEV_SFP1:
			dev = data->dev_sfp1_A2;
		break;
		case DEV_SFP2:
			dev = data->dev_sfp2_A2;
		break;
		case DEV_SFP3:
				dev = data->dev_sfp3_A2;
			break;
		case DEV_SFP4:
				dev = data->dev_sfp4_A2;
			break;
		case DEV_SFP5:
				dev = data->dev_sfp5_A2;
			break;
		default:
			return -EINVAL;
		break;
		}

	// Read status bit register
	rc = i2c_readn_reg(&dev,status_reg,status_buf,1);
	if(rc < 0)
		return rc;


	// Read flag bit register 1
	rc = i2c_readn_reg(&dev,flag_reg,flag_buf,1);
	if(rc < 0)
		return rc;

	// Read flag bit register 2
	flag_reg = SFP_FLG2_REG;
	rc = i2c_readn_reg(&dev,flag_reg,&flag_buf[1],1);
	if(rc < 0)
		return rc;

	uint16_t flags =  ((uint16_t) (flag_buf[0] << 8)  + flag_buf[1]);

	if((status_buf[0] & 0x06) != 0){
		sfp_avago_status_interruptions(status_buf[0],n);
	}
	if((flags & 0xFFC0) != 0){
		sfp_avago_alarms_interruptions(flags,n);
	}
	return 0;
}

/************************** Volt. and Curr. Sensor Functions ******************************/
int ina3221_get_voltage(struct DPB_I2cSensors *data,int n, float *res){
	int rc = 0;
	uint8_t voltage_buf[2] = {0,0};
	uint8_t voltage_reg = INA3221_BUS_VOLTAGE_1_REG;
	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0_2_VOLT:
			dev = data->dev_sfp0_2_volt;
		break;
		case DEV_SFP3_5_VOLT:
			dev = data->dev_sfp3_5_volt;
		break;
		case DEV_SOM_VOLT:
			dev = data->dev_som_volt;
		break;
		default:
			return -EINVAL;
		break;
	}
	for (int i=0;i<3;i++){
		if(i)
			voltage_reg = voltage_reg + 2;
		// Write bus voltage channel 1 address in register pointer
		rc = i2c_write(&dev,&voltage_reg,1);
		if(rc < 0)
			return rc;

	// Read MSB and LSB of voltage
		rc = i2c_read(&dev,voltage_buf,2);
		if(rc < 0)
			return rc;

		int voltage_int = (int)(voltage_buf[0] << 8) + (voltage_buf[1]);
		voltage_int = voltage_int / 8;
		res[i] = voltage_int * 8e-3;
	}
	return 0;
}

int ina3221_get_current(struct DPB_I2cSensors *data,int n, float *res){
	int rc = 0;
	uint8_t voltage_buf[2] = {0,0};
	uint8_t voltage_reg = INA3221_SHUNT_VOLTAGE_1_REG;
	struct I2cDevice dev;

	switch(n){
		case DEV_SFP0_2_VOLT:
				dev = data->dev_sfp0_2_volt;
		break;
		case DEV_SFP3_5_VOLT:
				dev = data->dev_sfp3_5_volt;
		break;
		case DEV_SOM_VOLT:
				dev = data->dev_som_volt;
		break;
		default:
			return -EINVAL;
		break;
		}
	for (int i=0;i<3;i++){
		if(i)
		voltage_reg = voltage_reg + 2;
		// Write shunt voltage channel 1 address in register pointer
		rc = i2c_write(&dev,&voltage_reg,1);
		if(rc < 0)
			return rc;

		// Read MSB and LSB of voltage
		rc = i2c_read(&dev,voltage_buf,2);
		if(rc < 0)
			return rc;

		int voltage_int = (int)(voltage_buf[0] << 8) + (voltage_buf[1]);
		voltage_int = voltage_int / 8;
		res[i] = (voltage_int * 40e-6) / 0.05 ;
		}
	return 0;
}

int ina3221_critical_interruptions(uint16_t mask, int n, char **text){
	char crit_str[80];
	strcpy(crit_str, "CRITICAL!!! Excess current in the channel: ");

	if((mask & 0x0080) == 0x0080){
		strcat(crit_str,text[0]);
		printf(crit_str);
		strcpy(crit_str, "CRITICAL!!! Excess current in the channel: ");
		printf("\n");
	}
	if((mask & 0x0100) == 0x0100){
		strcat(crit_str,text[1]);
		printf(crit_str);
		strcpy(crit_str, "CRITICAL!!! Excess current in the channel: ");
		printf("\n");
	}
	if((mask & 0x0200) == 0x0200){
		strcat(crit_str,text[2]);
		printf(crit_str);
		strcpy(crit_str, "CRITICAL!!! Excess current in the channel: ");
		printf("\n");
	}
	return 0;
}

int ina3221_warning_interruptions(uint16_t mask, int n, char **text){
	char warning_str[80];
	strcpy(warning_str, "Warning!!! Excess current in the channel: ");

	if((mask & 0x0008) == 0x0008){
		strcat(warning_str,text[0]);
		printf(warning_str);
		strcpy(warning_str, "Warning!!! Excess current in the channel: ");
		printf("\n");
	}
	if((mask & 0x0010) == 0x0010){
		strcat(warning_str,text[1]);
		printf(warning_str);
		strcpy(warning_str, "Warning!!! Excess current in the channel: ");
		printf("\n");
	}
	if((mask & 0x0020) == 0x0020){
		strcat(warning_str,text[2]);
		printf(warning_str);
		strcpy(warning_str, "Warning!!! Excess current in the channel: ");
		printf("\n");
	}
	return 0;
}

int ina3221_read_alarms(struct DPB_I2cSensors *data,int n){
	int rc = 0;
	uint8_t mask_buf[2] = {0,0};
	uint8_t mask_reg = INA3221_MASK_ENA_REG;
	struct I2cDevice dev;
	char *arr[] = { "0", "0", "0" };

	switch(n){
		case DEV_SFP0_2_VOLT:
			dev = data->dev_sfp0_2_volt;
			arr[0] =  "SFP-0\n";
			arr[1] =  "SFP-1\n";
			arr[2] =  "SFP-2\n";
		break;
		case DEV_SFP3_5_VOLT:
			dev = data->dev_sfp3_5_volt;
			arr[0] =  "SFP-3\n";
			arr[1] =  "SFP-4\n";
			arr[2] =  "SFP-5\n";
		break;
		case DEV_SOM_VOLT:
			dev = data->dev_som_volt;
			arr[0] =  "12 V\n";
			arr[1] =  "3.3 V\n";
			arr[2] =  "1.8 V\n";
		break;
		default:
			return -EINVAL;
		break;
		}
	// Write alarms address in register pointer
	rc = i2c_write(&dev,&mask_reg,1);
	if(rc < 0)
		return rc;

	// Read MSB and LSB of voltage
	rc = i2c_read(&dev,mask_buf,2);
	if(rc < 0)
		return rc;

	uint16_t mask_int = (uint16_t)(mask_buf[0] << 8) + (mask_buf[1]);
	if((mask_int & 0x0380)!= 0){
		ina3221_critical_interruptions(mask_int,n,arr);
	}
	if((mask_int & 0x0038)!= 0){
		ina3221_warning_interruptions(mask_int,n,arr);
	}

	return 0;
}

/************************** Threads declaration ******************************/
static int monitoring_thread_count;

static void *monitoring_thread(void *arg)
{
	struct periodic_info info;
	int rc ;
	struct DPB_I2cSensors *data = arg;

	float ams_temp[2];
	float ams_volt[18];
	int temp_chan[2] = {7,8};
	int volt_chan[18] = {0,1,2,3,4,5,6,9,10,11,12,13,14,15,16,17,18,19};

	float volt_sfp0_2[3];
	float volt_sfp3_5[3];
	float volt_som[3];

	float curr_sfp0_2[3];
	float curr_sfp3_5[3];
	float curr_som[3];

	float temp[1];
	float sfp_temp[1];
	float sfp_txpwr[1];
	float sfp_rxpwr[1];
	float sfp_vcc[1];
	float sfp_txbias[1];

	//struct DPB_I2cSensors data;


	printf("Monitoring thread period: %ds\n",MONIT_THREAD_PERIOD/1000000);
	rc = make_periodic(MONIT_THREAD_PERIOD, &info);
	if (rc) {
		printf("Error\r\n");
		return NULL;
	}
	while (1) {
		rc = mpc9844_read_temperature(data,temp);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_temperature(data,0,sfp_temp);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_voltage(data,0,sfp_vcc);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_lbias_current(data,0,sfp_txbias);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_tx_av_optical_pwr(data,0,sfp_txpwr);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_rx_av_optical_pwr(data,0,sfp_rxpwr);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}

		rc = ina3221_get_voltage(data,0,volt_sfp0_2);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_get_voltage(data,1,volt_sfp3_5);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_get_voltage(data,2,volt_som);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_get_current(data,0,curr_sfp0_2);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_get_current(data,1,curr_sfp3_5);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_get_current(data,2,curr_som);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = xlnx_ams_read_temp(temp_chan,2,ams_temp);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = xlnx_ams_read_volt(volt_chan,18,ams_volt);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		printf("Temperatura ambiente: %f ºC\n",temp[0]);
		printf("\n");
		printf("Temperatura SFP: %f ºC\n",sfp_temp[0]);
		printf("Tensión SFP: %f V\n",sfp_vcc[0]);
		printf("Corriente polarización del láser SFP: %f A\n",sfp_txbias[0]);
		printf("Potencia óptica transmitida SFP: %f W\n",sfp_txpwr[0]);
		printf("Potencia óptica recibida SFP: %f W\n",sfp_rxpwr[0]);
		printf("\n");
		for(int m = 0; m<2;m++){
			printf("Temperatura AMS - Canal %d: %f ºC - Iteración: %d\n",temp_chan[m],ams_temp[m],monitoring_thread_count);
		}
		printf("\n");
		for(int n = 0; n<18;n++){
			printf("Tensión AMS - Canal %d: %f V - Iteración: %d\n",volt_chan[n],ams_volt[n],monitoring_thread_count);
		}
		printf("\n");
		for(int j=0;j<3;j++){
		printf("Tensión SFP0-2 - Canal %d: %f V - Iteración: %d\n",j+1,volt_sfp0_2[j],monitoring_thread_count);
		printf("Corriente SFP0-2 - Canal %d: %f A - Iteración: %d\n",j+1,curr_sfp0_2[j],monitoring_thread_count);
		printf("Potencia consumida SFP0-2 - Canal %d: %f W - Iteración: %d\n",j+1,curr_sfp0_2[j]*volt_sfp0_2[j],monitoring_thread_count);
		}
		printf("\n");
		for(int k=0;k<3;k++){
		printf("Tensión SFP3-5 - Canal %d: %f V - Iteración: %d\n",k+1,volt_sfp3_5[k],monitoring_thread_count);
		printf("Corriente SFP3-5 - Canal %d: %f A - Iteración: %d\n",k+1,curr_sfp3_5[k],monitoring_thread_count);
		printf("Potencia consumida SFP3-5 - Canal %d: %f W - Iteración: %d\n",k+1,curr_sfp3_5[k]*volt_sfp3_5[k],monitoring_thread_count);
		}
		printf("\n");
		for(int l=0;l<3;l++){
		printf("Tensión SoM - Canal %d: %f V - Iteración: %d\n",l+1,volt_som[l],monitoring_thread_count);
		printf("Corriente SoM - Canal %d: %f A - Iteración: %d\n",l+1,curr_som[l],monitoring_thread_count);
		printf("Potencia consumida SoM - Canal %d: %f W - Iteración: %d\n",l+1,curr_som[l]*volt_som[l],monitoring_thread_count);
		}
		printf("\n");
		monitoring_thread_count++;
		wait_period(&info);
	}
	//stop_I2cSensors(&data);
	return NULL;
}
static void *i2c_alarms_thread(void *arg){
	struct periodic_info info;
	int rc ;
	struct DPB_I2cSensors *data = arg;

	printf("Alarms thread period: %dms\n",ALARMS_THREAD_PERIOD);
	rc = make_periodic(ALARMS_THREAD_PERIOD, &info);
	if (rc) {
		printf("Error\r\n");
		return NULL;
	}
	while(1){
		/*
		rc = mpc9844_read_alarms(data,float *res);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}

		rc = ina3221_read_alarms(data,0,float *res);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_read_alarms(data,1,float *res);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_read_alarms(data,2,float *res);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}

		rc = sfp_avago_read_alarms(data,0,float *res)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(data,1,float *res)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(data,2, float *res)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(data,3, float *res)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(data,4, float *res)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(data,5, float *res)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		*/

		wait_period(&info);
	}

	return NULL;
}



static void *ams_alarms_thread(void *arg){
	struct periodic_info info;
	int rc ;
	char ev_type[8];
	char ch_type[16];
	int chan;
	__s64 timestamp;
	char ev_str[80];

	char buffer [64];

	strcpy(ev_str, "/sys/bus/iio/devices/iio:device0/events/in_");

	key_t sharedMemoryKey = MEMORY_KEY;

    memoryID=shmget(sharedMemoryKey,sizeof(struct wrapper),0);

    if(memoryID==-1)
    {
        perror("shmget(): ");
        exit(1);
    }

    memory = shmat(memoryID,NULL,0);
    if(memory== (void*)-1)
    {
        perror("shmat():");
        exit(1);
    }

	printf("Alarms thread period: %dms\n",AMS_ALARMS_THREAD_PERIOD);
	rc = make_periodic(AMS_ALARMS_THREAD_PERIOD, &info);
	if (rc) {
		printf("Error\r\n");
		return NULL;
	}
	while(1){
        sem_wait(&memory->full);
        sem_wait(&memory->cmutex);

        chan = memory->chn;
        strcpy(ev_type,memory->ev_type);
        strcpy(ch_type,memory->ch_type);
        timestamp = memory->tmpstmp;
        snprintf(buffer, sizeof(buffer), "%d",chan);
        strcat(ev_str, ch_type);
        strcat(ev_str, buffer);
        strcat(ev_str, "_thresh_");
        strcat(ev_str, ev_type);
        strcat(ev_str, "_en");

        usleep(10);

        sem_post(&memory->cmutex);
        sem_post(&memory->empty);

        printf("Event type: %s. Timestamp: %lld. Channel type: %s. Channel: %d.",ev_type,timestamp,ch_type,chan);
        strcpy(ev_str, "/sys/bus/iio/devices/iio:device0/events/in_");
		wait_period(&info);
	}
	return NULL;
}
int main(){
	//IIO_EVENT_MONITOR process file
	FILE *fp = NULL;
	//Threads elements
	pthread_t t_1, t_2, t_3;
	sigset_t alarm_sig;
	int i;

	int rc;
	struct DPB_I2cSensors data;

	rc = init_I2cSensors(&data); //Initialize i2c sensors

	if (rc) {
		printf("Error\r\n");
		return 0;
	}

	rc = iio_event_monitor_up(fp); //Initialize iio event monitor
	if (rc) {
		printf("Error\r\n");
		return rc;
	}
	/* Block all real time signals so they can be used for the timers.
	   Note: this has to be done in main() before any threads are created
	   so they all inherit the same mask. Doing it later is subject to
	   race conditions */

	sigemptyset(&alarm_sig);
	for (i = SIGRTMIN; i <= SIGRTMAX; i++)
		sigaddset(&alarm_sig, i);
	sigprocmask(SIG_BLOCK, &alarm_sig, NULL);

	pthread_create(&t_1, NULL, monitoring_thread,(void *)&data); //Create thread 1 - monitors magnitudes every x seconds
	//pthread_create(&t_2, NULL, alarms_thread,(void *)&data); //Create thread 2 - read alarms every x miliseconds
	pthread_create(&t_3, NULL, ams_alarms_thread,NULL);

	while(1){
		sleep(1000000);
		}
	//stop_I2cSensors(&data);
	return 0;
}
