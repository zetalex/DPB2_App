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
#include <fcntl.h>

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
/**
 * Initialize MCP9844 Temperature Sensor
 *
 * @param I2cDevice *dev: device to be initialized
 *
 * @return Negative integer if initialization fails.If not, returns 0 and the device initialized
 *
 * @note This also checks via Manufacturer and Device ID that the device is correct
 */
int init_tempSensor (struct I2cDevice *dev) {
	int rc = 0;
	uint8_t manID_buf[2] = {0,0};
	uint8_t manID_reg = MCP9844_MANUF_ID_REG;
	uint8_t devID_buf[2] = {0,0};
	uint8_t devID_reg = MCP9844_DEVICE_ID_REG;

	rc = i2c_start(dev);  //Start I2C device
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
	if(!((manID_buf[0] == 0x00) && (manID_buf[1] == 0x54))){ //Check Manufacturer ID to verify is the right component
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
	if(!((devID_buf[0] == 0x06) && (devID_buf[1] == 0x01))){//Check Device ID to verify is the right component
			rc = -1;
			printf("Device ID does not match the corresponding device: Temperature Sensor\r\n");
			return rc;
	}
	return 0;

}
/**
 * Initialize INA3221 Voltage and Current Sensor
 *
 * @param I2cDevice *dev: device to be initialized
 *
 * @return Negative integer if initialization fails.If not, returns 0 and the device initialized
 *
 * @note This also checks via Manufacturer and Device ID that the device is correct
 */
int init_voltSensor (struct I2cDevice *dev) {
	int rc = 0;
	uint8_t manID_buf[2] = {0,0};
	uint8_t manID_reg = INA3221_MANUF_ID_REG;
	uint8_t devID_buf[2] = {0,0};
	uint8_t devID_reg = INA3221_DIE_ID_REG;

	rc = i2c_start(dev); //Start I2C device
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
	if(!((manID_buf[0] == 0x54) && (manID_buf[1] == 0x49))){ //Check Manufacturer ID to verify is the right component
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
	if(!((devID_buf[0] == 0x32) && (devID_buf[1] == 0x20))){ //Check Device ID to verify is the right component
			rc = -1;
			printf("Device ID does not match the corresponding device: Voltage Sensor\r\n");
			return rc;
	}
	return 0;

}
/**
 *Compares expected SFP checksum to its current value
 *
 * @param I2cDevice *dev: SFP of which the checksum is to be checked
 * @param uint8_t ini_reg: Register where the checksum count starts
 * @param uint8_t checksum_val: Checksum value expected
 * @param int size: number of registers summed for the checksum
 *
 * @return Negative integer if checksum is incorrect, and 0 if it is correct
 */
int checksum_check(struct I2cDevice *dev,uint8_t ini_reg, uint8_t checksum_val, int size){
	int rc = 0;
	int sum = 0;
	uint8_t byte_buf[size] ;

	rc = i2c_readn_reg(dev,ini_reg,byte_buf,1);  //Read every register from ini_reg to ini_reg+size-1
			if(rc < 0)
				return rc;
	for(int n=1;n<size;n++){
	ini_reg ++;
	rc = i2c_readn_reg(dev,ini_reg,&byte_buf[n],1);
		if(rc < 0)
			return rc;
	}

	for(int i=0;i<size;i++){
		sum += byte_buf[i];  //Sum every register read in order to obtain the checksum
	}
	uint8_t calc_checksum = (sum & 0xFF); //Only taking the 8 LSB of the checksum as the checksum register is only 8 bits

	if (checksum_val != calc_checksum){ //Check the obtained checksum equals the device checksum register
		rc = -1;
		printf("Checksum value does not match the expected value \r\n");
		return rc;
	}
	return 0;
}
/**
 * Initialize SFP EEPROM page 1 as an I2C device
 *
 * @param I2cDevice *dev: SFP of which EEPROM is to be initialized
 *
 * @return Negative integer if initialization fails.If not, returns 0 and the EEPROM page initialized as I2C device
 *
 * @note This also checks via Physical device, SFP function  and the checksum registers that the device is correct and the EEPROM is working properly.
 */
int init_SFP_A0(struct I2cDevice *dev) {
	int rc = 0;
	uint8_t SFPphys_reg = SFP_PHYS_DEV;
	uint8_t SFPphys_buf[2] = {0,0};

	rc = i2c_start(dev); //Start I2C device
		if (rc) {
			return rc;
		}
	//Read SFP Physcial device register
	rc = i2c_readn_reg(dev,SFPphys_reg,SFPphys_buf,1);
		if(rc < 0)
			return rc;

	//Read SFP function register
	SFPphys_reg = SFP_FUNCT;
	rc = i2c_readn_reg(dev,SFPphys_reg,&SFPphys_buf[1],1);
	if(rc < 0)
			return rc;
	if(!((SFPphys_buf[0] == 0x03) && (SFPphys_buf[1] == 0x04))){ //Check Physical device and function to verify is the right component
			rc = -1;
			printf("Device ID does not match the corresponding device: SFP-Avago\r\n");
			return rc;
	}
	rc = checksum_check(dev, SFP_PHYS_DEV,0x7F,63); //Check checksum register to verify is the right component and the EEPROM is working correctly
	if(rc < 0)
				return rc;
	rc = checksum_check(dev, SFP_CHECKSUM2_A0,0xFA,31);//Check checksum register to verify is the right component and the EEPROM is working correctly
	if(rc < 0)
				return rc;
	return 0;

}
/**
 * Initialize SFP EEPROM page 2 as an I2C device
 *
 * @param I2cDevice *dev: SFP of which EEPROM is to be initialized
 *
 * @return Negative integer if initialization fails.If not, returns 0 and the EEPROM page initialized as I2C device
 *
 * @note This also checks via the checksum register that the EEPROM is working properly.
 */
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
/**
 * Initialize every I2C sensor available
 *
 * @param DPB_I2cSensors *data; struct which contains every I2C sensor available
 *
 * @return Negative integer if initialization fails.If not, returns 0 every I2C sensor initialized.
 */
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
/**
 * Stops every I2C Sensors
 *
 * @param DPB_I2cSensors *data: struct which contains every I2C sensor available
 *
 * @returns 0.
 */
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
/**
 * Start IIO EVENT MONITOR to enable Xilinx-AMS events
 *
 * @param FILE *proc: file which contains the opened process
 *
 * @return Negative integer if start fails.If not, returns 0 and enables Xilinx-AMS events.
 */
int iio_event_monitor_up(FILE *proc) {
	proc = popen("/run/media/mmcblk0p1/IIO_MONITOR.elf -a iio:device0 &", "r");
	if (proc == NULL){
		printf("\nError executing iio_event_monitor.\n");
		return -1;
	}
	return 0;
}

/************************** AMS Functions ******************************/

/**
 * Reads temperature of n channels (channels specified in *chan) and stores the values in *res
 *
 * @param int *chan: array which contain channels to measure
 * @param int n: number of channels to measure
 * @param float *res: array where results are stored in
 *
 * @return Negative integer if reading fails.If not, returns 0 and the stored values in *res
 *
 * @note The resulting magnitude is obtained by applying the ADC conversion specified by Xilinx
 */
int xlnx_ams_read_temp(int *chan, int n, float *res){ //Read
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
			fclose(scale);
			printf("AMS Temperature file could not be opened!!! \n");/*Any of the files could not be opened*/
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

			float Temperature = (atof(scale_string) * (atof(raw_string) + atof(offset_string))) / 1024; //Apply ADC conversion to Temperature, Xilinx Specs
			fclose(raw);
			fclose(offset);
			fclose(scale);
			res[i] = Temperature;
			//return 0;
			}
		}
		return 0;
	}
/**
 * Reads voltage of n channels (channels specified in *chan) and stores the values in *res
 *
 * @param int *chan: array which contain channels to measure
 * @param int n: number of channels to measure
 * @param float *res: array where results are stored in
 *
 * @return Negative integer if reading fails.If not, returns 0 and the stored values in *res
 *
 * @note The resulting magnitude is obtained by applying the ADC conversion specified by Xilinx
 */
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
			fclose(scale);
			printf("AMS Voltage file could not be opened!!! \n");/*Any of the files could not be opened*/
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

			float Voltage = (atof(scale_string) * atof(raw_string)) / 1024; //Apply ADC conversion to Voltage, Xilinx Specs
			fclose(raw);
			fclose(scale);
			res[i] = Voltage;
			//return 0;
			}
		}
	return 0;
	}

/************************** Temp.Sensor Functions ******************************/

/**
 * Reads ambient temperature and stores the value in *res
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device for the MCP9844 Temperature Sensor
 * @param float *res: where the ambient temperature value is stored
 *
 * @return Negative integer if reading fails.If not, returns 0 and the stored value in *res
 *
 * @note The magnitude conversion depends if the temperature is below 0ºC or above. It also clear flag bits.
 */
int mcp9844_read_temperature(struct DPB_I2cSensors *data,float *res) {
	int rc = 0;
	struct I2cDevice dev = data->dev_pcb_temp;
	uint8_t temp_buf[2] = {0,0};
	uint8_t temp_reg = MCP9844_TEMP_REG;


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
/**
 * Set alarms limits for Temperature
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device for the MCP9844 Temperature Sensor
 * @param int n: which limit is modified
 * @param short temp: value of the limit that is to be set
 *
 * @return Negative integer if writing fails or limit chosen is incorrect.
 * @return 0 if everything is okay and modifies the temperature alarm limit
 */
int mcp9844_set_limits(struct DPB_I2cSensors *data,int n, short temp) {
	int rc = 0;
	struct I2cDevice dev = data->dev_pcb_temp;
	uint8_t temp_buf[2] = {0,0};
	uint8_t temp_reg ;
	switch(n){
		case MCP9844_TEMP_UPPER_LIM: //0
			temp_reg = MCP9844_TEMP_UPPER_LIM_REG;
			break;
		case MCP9844_TEMP_LOWER_LIM: //1
			temp_reg = MCP9844_TEMP_LOWER_LIM_REG;
			break;
		case MCP9844_TEMP_CRIT_LIM: //2
			temp_reg = MCP9844_TEMP_CRIT_LIM_REG;
			break;
		default:
			return -EINVAL;
	}
	if(temp < 0){
		temp = -temp;
		temp = temp << 2 ;
		temp = temp & 0x0FFF;
		temp = temp | 0x1000;
	}
	else{
		temp = temp << 2 ;
		temp = temp & 0x0FFF;
	}
	temp_buf[1] = temp & 0x00FF;
	temp_buf[0] = (temp >> 8) & 0x00FF;
	rc = i2c_write(&dev,&temp_reg,1);
	if(rc < 0)
		return rc;
	rc = i2c_write(&dev,temp_buf,2);
	if(rc < 0)
			return rc;
	return 0;
}
/**
 * Enables or disables configuration register bits of the MCP9844 Temperature Sensor
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device for the MCP9844 Temperature Sensor
 * @param uint8_t *bit_ena: array which should contain the desired bit value (0 o 1)
 * @param uint8_t *bit_num: array which should contain the position of the bit/s that will be modified
 *
 * @return Negative integer if writing fails,array size is mismatching or incorrect value introduced
 * @return 0 if everything is okay and modifies the configuration register
 */
int mcp9844_set_config(struct DPB_I2cSensors *data,uint8_t *bit_ena,uint8_t *bit_num) {
	int rc = 0;
	struct I2cDevice dev = data->dev_pcb_temp;
	uint8_t config_buf[2] = {0,0};
	uint8_t config_reg = MCP9844_CONFIG_REG;
	uint8_t array_size = sizeof(bit_num);
	uint16_t mask;
	uint16_t config;
	if(array_size != sizeof(bit_ena))
		return -EINVAL;
	rc = i2c_write(&dev,&config_reg,1);
	if(rc < 0)
		return rc;
	// Read MSB and LSB of config reg
	rc = i2c_read(&dev,config_buf,2);
	if(rc < 0)
			return rc;
	config = (config_buf[0] << 8) + (config_buf[1]);
	for(int i = 0; i<array_size;i++){
		mask = 1;
		mask = mask << bit_num[i];
		if(bit_ena[i] == 0){
			config = config & (~mask);
		}
		else if(bit_ena[i] == 1){
			config = config | mask;
		}
		else{
			return -EINVAL;
		}
	}
	config_buf[1] = config & 0x00FF;
	config_buf[0] = (config >> 8) & 0x00FF;
	rc = i2c_write(&dev,&config_reg,1);
	if(rc < 0)
		return rc;
	rc = i2c_write(&dev,config_buf,2);
	if(rc < 0)
			return rc;
	return 0;
}

/**
 * Handles MCP9844 Temperature Sensor interruptions
 *
 * @param uint8_t flag_buf: contains alarm flags
 *
 * @return 0 and handles interruption depending on the active flags
 */
int mcp9844_interruptions(uint8_t flag_buf){
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
/**
 * Reads MCP9844 Temperature Sensor alarms flags
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device for the MCP9844 Temperature Sensor
 *
 * @return  0 and if there is any flag active calls the corresponding function to handle the interruption
 *
 * @note It also clear flag bits.
 */
int mcp9844_read_alarms(struct DPB_I2cSensors *data) {
	int rc = 0;
	struct I2cDevice dev = data->dev_pcb_temp;
	uint8_t alarm_buf[1] = {0};
	uint8_t alarm_reg = MCP9844_TEMP_REG;


	// Write temperature address in register pointer
	rc = i2c_write(&dev,&alarm_reg,1);
	if(rc < 0)
		return rc;

	// Read MSB and LSB of ambient temperature
	rc = i2c_read(&dev,alarm_buf,1);
	if(rc < 0)
			return rc;
	if((alarm_buf[0] & 0xE0) != 0){
		mcp9844_interruptions(alarm_buf[0]);
	}
	alarm_buf[0] = alarm_buf[0] & 0x1F;	//Clear Flag bits
	return 0;
}

/************************** SFP Functions ******************************/
/**
 * Reads SFP temperature and stores the value in *res
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device SFP EEPROM page 2
 *
 * @param int n: indicate from which of the 6 SFP is going to be read,float *res where the magnitude value is stored
 *
 * @return Negative integer if reading fails.If not, returns 0 and the stored value in *res
 *
 * @note The magnitude conversion is based on the datasheet.
 */
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
/**
 * Reads SFP voltage supply and stores the value in *res
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device SFP EEPROM page 2
 *
 * @param int n: indicate from which of the 6 SFP is going to be read,float *res where the magnitude value is stored
 *
 * @return Negative integer if reading fails.If not, returns 0 and the stored value in *res
 *
 * @note The magnitude conversion is based on the datasheet.
 */
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
/**
 * Reads SFP laser bias current and stores the value in *res
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device SFP EEPROM page 2
 *
 * @param int n: indicate from which of the 6 SFP is going to be read,float *res where the magnitude value is stored
 *
 * @return Negative integer if reading fails.If not, returns 0 and the stored value in *res
 *
 * @note The magnitude conversion is based on the datasheet.
 */
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
/**
 * Reads SFP average transmitted optical power and stores the value in *res
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device SFP EEPROM page 2
 *
 * @param int n: indicate from which of the 6 SFP is going to be read,float *res where the magnitude value is stored
 *
 * @return Negative integer if reading fails.If not, returns 0 and the stored value in *res
 *
 * @note The magnitude conversion is based on the datasheet.
 */
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
/**
 * Reads SFP average received optical power and stores the value in *res
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device SFP EEPROM page 2
 *
 * @param int n: indicate from which of the 6 SFP is going to be read,float *res where the magnitude value is stored
 *
 * @return Negative integer if reading fails.If not, returns 0 and the stored value in *res
 *
 * @note The magnitude conversion is based on the datasheet.
 */
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
/**
 * Handles SFP status interruptions
 *
 * @param uint16_t flags: contains alarms flags
 * @param int n: indicate from which of the 6 SFP is dealing with
 *
 * @return 0 and handles interruption depending on the active status flags
 */
int sfp_avago_status_interruptions(uint8_t status, int n){

	if((status & 0x02) != 0){
		printf("Reception Loss of Signal in SFP:%d",n);
	}
	if((status & 0x04) != 0){
		printf("Transmitter Fault in SFP:%d",n);
	}
	return 0;
}
/**
 * Handles SFP alarm interruptions
 *
 * @param uint16_t flags: contains alarms flags
 * @param int n: indicate from which of the 6 SFP is dealing with
 *
 * @return 0 and handles interruption depending on the active alarms flags
 */
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
/**
 * Reads SFP status and alarms flags
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device for the SFP EEPROM page 2
 * @param int n: indicate from which of the 6 SFP is going to be read
 *
 * @return  0 and if there is any flag active calls the corresponding function to handle the interruption.
 */
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
/**
 * Reads INA3221 Voltage and Current Sensor bus voltage from each of its 3 channels and stores the values in *res
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device INA3221 Voltage and Current Sensor
 * @param int n: indicate from which of the 3 INA3221 is going to be read,float *res where the voltage values are stored
 *
 * @return Negative integer if reading fails.If not, returns 0 and the stored values in *res
 *
 * @note The magnitude conversion is based on the datasheet.
 */
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
/**
 * Reads INA3221 Voltage and Current Sensor shunt voltage from a resistor in each of its 3 channels,
 * obtains the current dividing the voltage by the resistor value and stores the current values in *res
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device INA3221 Voltage and Current Sensor
 * @param int n: indicate from which of the 3 INA3221 is going to be read,float *res where the current values are stored
 *
 * @return Negative integer if reading fails.If not, returns 0 and the stored values in *res
 *
 * @note The magnitude conversion is based on the datasheet and the resistor value is 0.05 Ohm.
 */
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
/**
 * Handles INA3221 Voltage and Current Sensor critical alarm interruptions
 *
 * @param uint16_t mask contains critical alarm flags
 * @param int n indicate from which of the 3 INA3221 is dealing with
 *
 * @return 0 and handles interruption depending on the active alarms flags
 */
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
/**
 * Handles INA3221 Voltage and Current Sensor warning alarm interruptions
 *
 * @param uint16_t mask: contains warning alarm flags
 * @param int n: indicate from which of the 3 INA3221 is dealing with
 *
 * @return 0 and handles interruption depending on the active alarms flags
 */
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
/**
 * Reads INA3221 Voltage and Current Sensor warning and critical alarms flags
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device for the INA3221 Voltage and Current Sensor
 * @param int n: indicate from which of the 3 INA3221 is going to be read
 *
 * @return  0 and if there is any flag active calls the corresponding function to handle the interruption.
 */
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
	else if((mask_int & 0x0038)!= 0){
		ina3221_warning_interruptions(mask_int,n,arr);
	}

	return 0;
}
/**
 * Set current alarms limits for INA3221 (warning or critical)
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device for the MCP9844 Temperature Sensor
 * @param int n: which of the 3 INA3221 is being dealt with
 * @param int ch: which of the 3 INA3221 channels is being dealt with
 * @param int alarm_type: indicates if the limit to be modifies is for a critical alarm or warning alarm
 * @param float curr: current value which will be the new limit
 *
 * @return Negative integer if writing fails or any parameter is incorrect.
 * @return 0 if everything is okay and modifies the current alarm limit (as shunt voltage limit)
 */
int ina3221_set_limits(struct DPB_I2cSensors *data,int n,int ch,int alarm_type ,float curr) {
	int rc = 0;
	uint8_t volt_buf[2] = {0,0};
	uint8_t volt_reg ;
	uint16_t volt_lim;
	struct I2cDevice dev;
	if(curr >= 1.5)
		return EINVAL;
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
	switch(ch){
		case INA3221_CH1:
			volt_reg = (alarm_type)?INA3221_SHUNT_VOLTAGE_WARN1_REG:INA3221_SHUNT_VOLTAGE_CRIT1_REG;
		break;
		case INA3221_CH2:
			volt_reg = (alarm_type)?INA3221_SHUNT_VOLTAGE_WARN2_REG:INA3221_SHUNT_VOLTAGE_CRIT2_REG;
		break;
		case INA3221_CH3:
			volt_reg = (alarm_type)?INA3221_SHUNT_VOLTAGE_WARN3_REG:INA3221_SHUNT_VOLTAGE_CRIT3_REG;
		break;
		default:
			return -EINVAL;
		break;
		}
	if(curr < 0){
		curr = -curr;
		volt_lim = (curr * 0.05 * 1e6)/40; //0.05 = Resistor value
		volt_lim = volt_lim << 3 ;
		volt_lim = (~volt_lim)+1;
		volt_lim = volt_lim | 0x8000;
	}
	else{
		volt_lim = (curr * 0.05 * 1e6)/40; //0.05 = Resistor value
		volt_lim = volt_lim << 3 ;
	}
	volt_buf[1] = volt_lim & 0x00FF;
	volt_buf[0] = (volt_lim >> 8) & 0x00FF;
	rc = i2c_write(&dev,&volt_reg,1);
	if(rc < 0)
		return rc;
	rc = i2c_write(&dev,volt_buf,2);
	if(rc < 0)
			return rc;
	return 0;
}
/**
 * Enables or disables configuration register bits of the INA3221 Voltage Sensor
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device for the INA3221 Voltage Sensor
 * @param uint8_t *bit_ena: array which should contain the desired bit value (0 o 1)
 * @param uint8_t *bit_num: array which should contain the position of the bit/s that will be modified
 * @param int n :which of the 3 INA3221 is being dealt with
 *
 * @return Negative integer if writing fails,array size is mismatching or incorrect value introduced
 * @return 0 if everything is okay and modifies the configuration register
 */
int ina3221_set_config(struct DPB_I2cSensors *data,uint8_t *bit_ena,uint8_t *bit_num, int n) {
	int rc = 0;
	uint8_t config_buf[2] = {0,0};
	uint8_t config_reg = INA3221_CONFIG_REG;
	uint8_t array_size = sizeof(bit_num);
	uint16_t mask;
	uint16_t config;
	struct I2cDevice dev;
	if(array_size != sizeof(bit_ena))
		return -EINVAL;
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
	rc = i2c_write(&dev,&config_reg,1);
	if(rc < 0)
		return rc;
	// Read MSB and LSB of config reg
	rc = i2c_read(&dev,config_buf,2);
	if(rc < 0)
			return rc;
	config = (config_buf[0] << 8) + (config_buf[1]);
	for(int i = 0; i<array_size;i++){
		mask = 1;
		mask = mask << bit_num[i];
		if(bit_ena[i] == 0){
			config = config & (~mask);
		}
		else if(bit_ena[i] == 1){
			config = config | mask;
		}
		else{
			return -EINVAL;
		}
	}
	config_buf[1] = config & 0x00FF;
	config_buf[0] = (config >> 8) & 0x00FF;
	rc = i2c_write(&dev,&config_reg,1);
	if(rc < 0)
		return rc;
	rc = i2c_write(&dev,config_buf,2);
	if(rc < 0)
			return rc;
	return 0;
}
/************************** Threads declaration ******************************/
static int monitoring_thread_count;
/**
 * Periodic thread that every x seconds reads every magnitude of every sensor available and stores it.
 *
 * @param void *arg: must contain a struct with every I2C device that wants to be monitored
 *
 * @return  NULL (if exits is because of an error).
 */
static void *monitoring_thread(void *arg)
{
	struct periodic_info info;
	int rc ;
	struct DPB_I2cSensors *data = arg;

	float ams_temp[3];
	float ams_volt[25];
	int temp_chan[3] = {7,8,20};
	int volt_chan[25] = {9,10,11,12,13,14,15,16,17,18,19,21,22,23,24,25,26,27,28,29,30,31,32,33,34};

	float volt_sfp0_2[3];
	float volt_sfp3_5[3];
	float volt_som[3];

	float curr_sfp0_2[3];
	float curr_sfp3_5[3];
	float curr_som[3];

	float temp[1];
	float sfp_temp_0[1];
	float sfp_txpwr_0[1];
	float sfp_rxpwr_0[1];
	float sfp_vcc_0[1];
	float sfp_txbias_0[1];

	/*float sfp_temp_1[1];
	float sfp_txpwr_1[1];
	float sfp_rxpwr_1[1];
	float sfp_vcc_1[1];
	float sfp_txbias_1[1];

	float sfp_temp_2[1];
	float sfp_txpwr_2[1];
	float sfp_rxpwr_2[1];
	float sfp_vcc_2[1];
	float sfp_txbias_2[1];

	float sfp_temp_3[1];
	float sfp_txpwr_3[1];
	float sfp_rxpwr_3[1];
	float sfp_vcc_3[1];
	float sfp_txbias_3[1];

	float sfp_temp_4[1];
	float sfp_txpwr_4[1];
	float sfp_rxpwr_4[1];
	float sfp_vcc_4[1];
	float sfp_txbias_4[1];

	float sfp_temp_5[1];
	float sfp_txpwr_5[1];
	float sfp_rxpwr_5[1];
	float sfp_vcc_5[1];
	float sfp_txbias_5[1];*/

	//struct DPB_I2cSensors data;


	printf("Monitoring thread period: %ds\n",MONIT_THREAD_PERIOD/1000000);
	rc = make_periodic(MONIT_THREAD_PERIOD, &info);
	if (rc) {
		printf("Error\r\n");
		return NULL;
	}
	while (1) {
		rc = mcp9844_read_temperature(data,temp);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_temperature(data,0,sfp_temp_0);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_voltage(data,0,sfp_vcc_0);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_lbias_current(data,0,sfp_txbias_0);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_tx_av_optical_pwr(data,0,sfp_txpwr_0);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_rx_av_optical_pwr(data,0,sfp_rxpwr_0);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		/*rc = sfp_avago_read_temperature(data,1,sfp_temp_1);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_voltage(data,1,sfp_vcc_1);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_lbias_current(data,1,sfp_txbias_1);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_tx_av_optical_pwr(data,1,sfp_txpwr_1);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_rx_av_optical_pwr(data,1,sfp_rxpwr_1);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_temperature(data,2,sfp_temp_2);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_voltage(data,2,sfp_vcc_2);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_lbias_current(data,2,sfp_txbias_2);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_tx_av_optical_pwr(data,2,sfp_txpwr_2);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_rx_av_optical_pwr(data,2,sfp_rxpwr_2);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_temperature(data,3,sfp_temp_3);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_voltage(data,3,sfp_vcc_3);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_lbias_current(data,3,sfp_txbias_3);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_tx_av_optical_pwr(data,3,sfp_txpwr_3);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_rx_av_optical_pwr(data,3,sfp_rxpwr_3);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_temperature(data,4,sfp_temp_4);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_voltage(data,4,sfp_vcc_4);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_lbias_current(data,4,sfp_txbias_4);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_tx_av_optical_pwr(data,4,sfp_txpwr_4);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_rx_av_optical_pwr(data,4,sfp_rxpwr_4);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_temperature(data,5,sfp_temp_5);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_voltage(data,5,sfp_vcc_5);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_lbias_current(data,5,sfp_txbias_5);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_tx_av_optical_pwr(data,5,sfp_txpwr_5);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_rx_av_optical_pwr(data,5,sfp_rxpwr_5);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}*/
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
		printf("Temperatura SFP: %f ºC\n",sfp_temp_0[0]);
		printf("Tensión SFP: %f V\n",sfp_vcc_0[0]);
		printf("Corriente polarización del láser SFP: %f A\n",sfp_txbias_0[0]);
		printf("Potencia óptica transmitida SFP: %f W\n",sfp_txpwr_0[0]);
		printf("Potencia óptica recibida SFP: %f W\n",sfp_rxpwr_0[0]);
		printf("\n");
		for(int m = 0; m<3;m++){
			printf("Temperatura AMS - Canal %d: %f ºC - Iteración: %d\n",temp_chan[m],ams_temp[m],monitoring_thread_count);
		}
		printf("\n");
		for(int n = 0; n<25;n++){
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
/**
 * Periodic thread that every x seconds reads every alarm of every I2C sensor available and handles the interruption.
 *
 * @param void *arg: must contain a struct with every I2C device that wants to be monitored
 *
 * @return  NULL (if exits is because of an error).
 */
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
/**
 * Periodic thread that is waiting for an alarm from any Xilinx AMS channel, the alarm is presented as an event,
 * events are reported by IIO EVENT MONITOR through shared memory
 *
 * @param void *arg: NULL
 *
 * @return  NULL (if exits is because of an error).
 */
static void *ams_alarms_thread(void *arg){
	struct periodic_info info;
	int rc ;
	char ev_type[8];
	char ch_type[16];
	int chan;
	__s64 timestamp;
	char ev_str[80];
    int fd;
    char ena = '1';
    char disab = '0';
	char buffer [64];

	strcpy(ev_str, "/sys/bus/iio/devices/iio:device0/events/in_");

	key_t sharedMemoryKey = MEMORY_KEY;  //Using shared memory to communicate with IIO EVENT MONITOR

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
        sem_wait(&memory->full);  //Semaphore to wait until any event happens
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
        fd = open(ev_str, O_WRONLY);
        write (fd, &disab, 1);

        usleep(10);

        write (fd, &ena, 1);  //Restarting enablement of the event again so it can be asserted again later
        close(fd);
        sem_post(&memory->cmutex);//Free the semaphore so the IIO EVENT MONITOR can report another event
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
	   race conditions*/

	sigemptyset(&alarm_sig);
	for (i = SIGRTMIN; i <= SIGRTMAX; i++)
		sigaddset(&alarm_sig, i);
	sigprocmask(SIG_BLOCK, &alarm_sig, NULL);

	pthread_create(&t_1, NULL, monitoring_thread,(void *)&data); //Create thread 1 - monitors magnitudes every x seconds
	//pthread_create(&t_2, NULL, alarms_thread,(void *)&data); //Create thread 2 - read alarms every x miliseconds
	pthread_create(&t_3, NULL, ams_alarms_thread,NULL);//Create thread 2 - read AMS alarms

	while(1){
		sleep(1000000);
		}
	//stop_I2cSensors(&data);
	return 0;
}
