/*
 * main.c
 *
 * @date
 * @author Borja Martínez Sánchez
 */

/************************** Libraries includes *****************************/
#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include <semaphore.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <time.h>
#include "json-c/json.h"

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
/******************************************************************************
*Local Semaphores.
****************************************************************************/
sem_t i2c_sync;
/************************** Function Prototypes ******************************/

int init_tempSensor (struct I2cDevice *);
int init_voltSensor (struct I2cDevice *);
int checksum_check(struct I2cDevice *,uint8_t,uint8_t,int);
int init_SFP_A0(struct I2cDevice *);
int init_SFP_A2(struct I2cDevice *);
int init_I2cSensors(struct DPB_I2cSensors *);
int stop_I2cSensors(struct DPB_I2cSensors *);
int iio_event_monitor_up();
int xlnx_ams_read_temp(int *, int, float *);
int xlnx_ams_read_volt(int *, int, float *);
int xlnx_ams_set_limits(int, char *, char *, float);
int mcp9844_read_temperature(struct DPB_I2cSensors *,float *);
int mcp9844_set_limits(struct DPB_I2cSensors *,int, float);
int mcp9844_set_config(struct DPB_I2cSensors *,uint8_t *,uint8_t *);
int mcp9844_interruptions(json_object *,struct DPB_I2cSensors *, uint8_t );
int mcp9844_read_alarms(json_object *,struct DPB_I2cSensors *);
int sfp_avago_read_temperature(struct DPB_I2cSensors *,int , float *);
int sfp_avago_read_voltage(struct DPB_I2cSensors *,int , float *);
int sfp_avago_read_lbias_current(struct DPB_I2cSensors *,int, float *);
int sfp_avago_read_tx_av_optical_pwr(struct DPB_I2cSensors *,int, float *);
int sfp_avago_read_rx_av_optical_pwr(struct DPB_I2cSensors *data,int, float *);
int sfp_avago_status_interruptions(json_object *,uint8_t, int);
int sfp_avago_alarms_interruptions(json_object *,struct DPB_I2cSensors *,uint16_t , int );
int sfp_avago_read_alarms(json_object *,struct DPB_I2cSensors *,int ) ;
int ina3221_get_voltage(struct DPB_I2cSensors *,int , float *);
int ina3221_get_current(struct DPB_I2cSensors *,int , float *);
int ina3221_critical_interruptions(json_object *,struct DPB_I2cSensors *,uint16_t , int );
int ina3221_warning_interruptions(json_object *,struct DPB_I2cSensors *,uint16_t , int );
int ina3221_read_alarms(json_object *,struct DPB_I2cSensors *,int);
int ina3221_set_limits(struct DPB_I2cSensors *,int ,int ,int  ,float );
int ina3221_set_config(struct DPB_I2cSensors *,uint8_t *,uint8_t *, int );
int parsing_mon_data_into_array (json_object *,float , char *, int );
int parsing_alm_data_into_array (json_object *,char *,char *, int , float ,int32_t );
static void *monitoring_thread(void *);
static void *i2c_alarms_thread(void *);
static void *ams_alarms_thread(void *);

/************************** IIO_EVENT_MONITOR Functions ******************************/
/**
 * Start IIO EVENT MONITOR to enable Xilinx-AMS events
 *
 * @param FILE *proc: file which contains the opened process
 *
 * @return Negative integer if start fails.If not, returns 0 and enables Xilinx-AMS events.
 */
int iio_event_monitor_up() {


    pid_t pid = fork(); // Create a child process

    if (pid == 0) {
        // Child process
        // Path of the .elf file and arguments
        char *args[] = {"/run/media/mmcblk0p1/IIO_MONITOR.elf", "-a", "/dev/iio:device0", NULL};

        // Execute the .elf file
        if (execvp(args[0], args) == -1) {
            perror("Error executing the .elf file");
            return -1;
        }
    } else if (pid > 0) {
        // Parent process
        // You can perform other tasks here while the child process executes the .elf file
    } else {
        // Error creating the child process
        perror("Error creating the child process");
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
/**
 * Determines the new limit of the alarm of the channel n
 *
 * @param int chan: channel whose alarm limit will be changed
 * @param char *ev_type: string that determines the type of the event
 * @param char *ch_type: string that determines the type of the channel
 * @float val: value of the new limit
 *
 * @return Negative integer if setting fails, any file could not be opened or invalid argument.If not, returns 0 and the modifies the specified limit
 *
 */
int xlnx_ams_set_limits(int chan, char *ev_type, char *ch_type, float val){
	FILE *offset,*scale;

		char buffer [sizeof(chan)*8+1];
		char offset_str[128];
		char thres_str[128];
		char scale_str[128];
		char adc_buff [8];
		long fsize;
		int thres;
		uint16_t adc_code;

		if(val<0) //Cannot be negative
			return -EINVAL;

		snprintf(buffer, sizeof(buffer), "%d",chan);

		strcpy(scale_str, "/sys/bus/iio/devices/iio:device0/in_");
		strcat(scale_str, ch_type);
		strcat(scale_str, buffer);
		strcat(scale_str, "_scale");

		strcpy(thres_str, "/sys/bus/iio/devices/iio:device0/events/in_");
		strcat(thres_str, ch_type);
		strcat(thres_str, buffer);
		strcat(thres_str, "_thresh_");
		strcat(thres_str, ev_type);
		strcat(thres_str, "_value");

		scale = fopen(scale_str,"r");
		thres = open(thres_str, O_WRONLY);

		if((scale==NULL)|(thres < 0)){
			fclose(scale);
			printf("AMS Voltage file could not be opened!!! \n");/*Any of the files could not be opened*/
			return -1;
			}
		else{
			if(!strcmp("temp",ch_type)){
				strcpy(offset_str, "/sys/bus/iio/devices/iio:device0/in_");
				strcat(offset_str, ch_type);
				strcat(offset_str, buffer);
				strcat(offset_str, "_offset");
				offset = fopen(offset_str,"r");
				if(offset==NULL){
					fclose(offset);
					printf("AMS Voltage file could not be opened!!! \n");/*Any of the files could not be opened*/
					return -1;
				}
				if(strcmp("rising",ev_type)){
					return -EINVAL;
				}
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

				fclose(scale);
				fclose(offset);

			    adc_code = (uint16_t) (1024*val)/atof(scale_string) - atof(offset_string);

			}
			else if(!strcmp("voltage",ch_type)){
				if((strcmp("rising",ev_type))&(strcmp("falling",ev_type))){
					return -EINVAL;
				}
				fseek(scale, 0, SEEK_END);
				fsize = ftell(scale);
				fseek(scale, 0, SEEK_SET);  /* same as rewind(f); */

				char *scale_string = malloc(fsize + 1);
				fread(scale_string, fsize, 1, scale);

				adc_code = (uint16_t)(1024*val)/atof(scale_string);

				fclose(scale);

				//return 0;
			}
			else
				return -EINVAL;

			snprintf(adc_buff, sizeof(adc_buff), "%d",adc_code);
			write (thres, &adc_buff, sizeof(adc_buff));
			close(thres);
			}
	return 0;
	}

/************************** I2C Devices Functions ******************************/
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


	rc = mcp9844_set_limits(data,0,75);
	if (rc) {
		printf("Failed to set MCP9844 Upper Limit\r\n");
		return rc;
	}

	rc = mcp9844_set_limits(data,2,90);
	if (rc) {
		printf("Failed to set MCP9844 Critical Limit\r\n");
		return rc;
	}

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

/************************** Temp.Sensor Functions ******************************/
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
		printf("Manufacturer ID does not match the corresponding device: Temperature Sensor\r\n");
		return -EINVAL;
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
			printf("Device ID does not match the corresponding device: Temperature Sensor\r\n");
			return -EINVAL;
	}
	return 0;

}

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
int mcp9844_set_limits(struct DPB_I2cSensors *data,int n, float temp_val) {
	int rc = 0;
	struct I2cDevice dev = data->dev_pcb_temp;
	uint8_t temp_buf[3] = {0,0,0};
	uint8_t temp_reg ;
	uint16_t temp;

	if((temp_val<-40)|(temp_val>125))
		return -EINVAL;

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
	if(temp_val<0){
		temp_val = -temp_val;
		temp = (short) temp_val/ 0.25;
		temp = temp << 2 ;
		temp = temp & 0x0FFF;
		temp = temp | 0x1000;
	}
	else{
		temp = (short) temp_val/ 0.25;
		temp = temp << 2 ;
		temp = temp & 0x0FFF;
	}

	temp_buf[2] = temp & 0x00FF;
	temp_buf[1] = (temp >> 8) & 0x00FF;
	temp_buf[0] = temp_reg;
	rc = i2c_write(&dev,temp_buf,3);
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
	uint8_t conf_buf[3] = {0,0,0};
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
	conf_buf[2] = config & 0x00FF;
	conf_buf[1] = (config >> 8) & 0x00FF;
	conf_buf[0] = config_reg;
	rc = i2c_write(&dev,conf_buf,3);
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
int mcp9844_interruptions(json_object *jarray,struct DPB_I2cSensors *data, uint8_t flag_buf){
	int32_t timestamp;
	float res [1];
	mcp9844_read_temperature(data,res);

	if((flag_buf & 0x80) == 0x80){
		timestamp = time(NULL);
		parsing_alm_data_into_array(jarray,"Board Temperature","critical", 99, res[0],timestamp);	}
	if((flag_buf & 0x40) == 0x40){
		timestamp = time(NULL);
		parsing_alm_data_into_array(jarray,"Board Temperature","rising", 99, res[0],timestamp);	}
	if((flag_buf & 0x20) == 0x20){
		timestamp = time(NULL);
		parsing_alm_data_into_array(jarray,"Board Temperature","falling", 99, res[0],timestamp);	}
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
int mcp9844_read_alarms(json_object *jarray,struct DPB_I2cSensors *data) {
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
		mcp9844_interruptions(jarray,data,alarm_buf[0]);
	}
	alarm_buf[0] = alarm_buf[0] & 0x1F;	//Clear Flag bits
	return 0;
}

/************************** SFP Functions ******************************/
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
			printf("Device ID does not match the corresponding device: SFP-Avago\r\n");
			return -EINVAL;
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
	rc = checksum_check(dev,SFP_MSB_HTEMP_ALARM_REG,0x61,95);//Check checksum register to verify is the right component and the EEPROM is working correctly
	if(rc < 0)
				return rc;
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
		printf("Checksum value does not match the expected value \r\n");
		return -EHWPOISON;
	}
	return 0;
}

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
int sfp_avago_status_interruptions(json_object *jarray,uint8_t status, int n){
	int32_t timestamp;

	if((status & 0x02) != 0){
		timestamp = time(NULL);
		parsing_alm_data_into_array(jarray,"SFP RxLOS","critical", n, 0,timestamp);
	}
	if((status & 0x04) != 0){
		timestamp = time(NULL);
		parsing_alm_data_into_array(jarray,"SFP TxFault","critical", n, 0,timestamp);	}
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
int sfp_avago_alarms_interruptions(json_object *jarray,struct DPB_I2cSensors *data,uint16_t flags, int n){
	int32_t timestamp;
	float res [1];

	if((flags & 0x0080) == 0x0080){
		timestamp = time(NULL);
		sfp_avago_read_rx_av_optical_pwr(data,n,res);
		parsing_alm_data_into_array(jarray,"SFP Rx Optical Power","rising", n, res[0],timestamp);	}
	if((flags & 0x0040) == 0x0040){
		timestamp = time(NULL);
		sfp_avago_read_rx_av_optical_pwr(data,n,res);
		parsing_alm_data_into_array(jarray,"SFP Rx Optical Power","falling", n, res[0],timestamp);	}
	if((flags & 0x0200) == 0x0200){
		timestamp = time(NULL);
		sfp_avago_read_tx_av_optical_pwr(data,n,res);
		parsing_alm_data_into_array(jarray,"SFP Tx Optical Power","rising", n, res[0],timestamp);	}
	if((flags & 0x0100) == 0x0100){
		timestamp = time(NULL);
		sfp_avago_read_tx_av_optical_pwr(data,n,res);
		parsing_alm_data_into_array(jarray,"SFP Tx Optical Power","falling", n, res[0],timestamp);	}
	if((flags & 0x0800) == 0x0800){
		timestamp = time(NULL);
		sfp_avago_read_lbias_current(data,n,res);
		parsing_alm_data_into_array(jarray,"SFP Laser Bias Current","rising", n, res[0],timestamp);	}
	if((flags & 0x0400) == 0x0400){
		timestamp = time(NULL);
		sfp_avago_read_lbias_current(data,n,res);
		parsing_alm_data_into_array(jarray,"SFP Laser Bias Current","falling", n, res[0],timestamp);	}
	if((flags & 0x2000) == 0x2000){
		timestamp = time(NULL);
		sfp_avago_read_voltage(data,n,res);
		parsing_alm_data_into_array(jarray,"SFP Supply Voltage","rising", n, res[0],timestamp);	}
	if((flags & 0x1000) == 0x1000){
		timestamp = time(NULL);
		sfp_avago_read_voltage(data,n,res);
		parsing_alm_data_into_array(jarray,"SFP Supply Voltage","falling", n, res[0],timestamp);	}
	if((flags & 0x8000) == 0x8000){
		timestamp = time(NULL);
		sfp_avago_read_temperature(data,n,res);
		parsing_alm_data_into_array(jarray,"SFP Temperature","rising", n, res[0],timestamp);	}
	if((flags & 0x4000) == 0x4000){
		timestamp = time(NULL);
		sfp_avago_read_temperature(data,n,res);
		parsing_alm_data_into_array(jarray,"SFP Temperature","falling", n, res[0],timestamp);	}
	printf("\n");
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
int sfp_avago_read_alarms(json_object *jarray,struct DPB_I2cSensors *data,int n) {
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
		sfp_avago_status_interruptions(jarray,status_buf[0],n);
	}
	if((flags & 0xFFC0) != 0){
		sfp_avago_alarms_interruptions(jarray,data,flags,n);
	}
	return 0;
}

/************************** Volt. and Curr. Sensor Functions ******************************/
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
		printf("Manufacturer ID does not match the corresponding device: Voltage Sensor\r\n");
		return -EINVAL;
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
			printf("Device ID does not match the corresponding device: Voltage Sensor\r\n");
			return -EINVAL;
	}
	return 0;

}

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
int ina3221_critical_interruptions(json_object *jarray,struct DPB_I2cSensors *data,uint16_t mask, int n){
	int32_t timestamp;
	float res[3];
	ina3221_get_current(data,n,res);
	int k = 0;
	if (n == 1)
		k = 3;

	if((mask & 0x0080) == 0x0080){
		timestamp = time(NULL);
		if(n == 2)
			parsing_alm_data_into_array(jarray,"SoM Current(1.8V)","critical", 99, res[2],timestamp);
		else
			parsing_alm_data_into_array(jarray,"SFP Current","critical", k+2, res[2],timestamp);		}
	if((mask & 0x0100) == 0x0100){
		timestamp = time(NULL);
		if(n == 2)
			parsing_alm_data_into_array(jarray,"SoM Current(1.8V)","critical", 99, res[2],timestamp);
		else
			parsing_alm_data_into_array(jarray,"SFP Current","critical", k+2, res[2],timestamp);	}
	if((mask & 0x0200) == 0x0200){
		timestamp = time(NULL);
		if(n == 2)
			parsing_alm_data_into_array(jarray,"SoM Current(1.8V)","critical", 99, res[2],timestamp);
		else
			parsing_alm_data_into_array(jarray,"SFP Current","critical", k+2, res[2],timestamp);	}
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
int ina3221_warning_interruptions(json_object *jarray,struct DPB_I2cSensors *data,uint16_t mask, int n){

	int32_t timestamp;
	float res[3];
	ina3221_get_current(data,n,res);
	int k = 0;
	if (n == 1)
		k = 3;

	if((mask & 0x0008) == 0x0008){
		timestamp = time(NULL);
		if(n == 2)
			parsing_alm_data_into_array(jarray,"SoM Current(12V)","rising", 99, res[0],timestamp);
		else
			parsing_alm_data_into_array(jarray,"SFP Current","rising", k, res[0],timestamp);	}
	if((mask & 0x0010) == 0x0010){
		timestamp = time(NULL);
		if(n == 2)
			parsing_alm_data_into_array(jarray,"SoM Current(3.3V)","rising", 99, res[1],timestamp);
		else
			parsing_alm_data_into_array(jarray,"SFP Current","rising", k+1, res[1],timestamp);	}
	if((mask & 0x0020) == 0x0020){
		timestamp = time(NULL);
		if(n == 2)
			parsing_alm_data_into_array(jarray,"SoM Current(1.8V)","rising", 99, res[2],timestamp);
		else
			parsing_alm_data_into_array(jarray,"SFP Current","rising", k+2, res[2],timestamp);	}
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
int ina3221_read_alarms(json_object *jarray,struct DPB_I2cSensors *data,int n){
	int rc = 0;
	uint8_t mask_buf[2] = {0,0};
	uint8_t mask_reg = INA3221_MASK_ENA_REG;
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
		ina3221_critical_interruptions(jarray,data,mask_int,n);
	}
	else if((mask_int & 0x0038)!= 0){
		ina3221_warning_interruptions(jarray,data,mask_int,n);
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
	uint8_t volt_buf[3] = {0,0,0};
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
	volt_buf[2] = volt_lim & 0x00FF;
	volt_buf[1] = (volt_lim >> 8) & 0x00FF;
	volt_buf[0] = volt_reg;
	rc = i2c_write(&dev,volt_buf,3);
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
	uint8_t conf_buf[3] = {0,0,0};
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
	conf_buf[2] = config & 0x00FF;
	conf_buf[1] = (config >> 8) & 0x00FF;
	conf_buf[0] = config_reg;
	rc = i2c_write(&dev,conf_buf,3);
	if(rc < 0)
		return rc;
	return 0;
}
/************************** JSON functions ******************************/
/**
 * Parses monitoring data into a JSON array so as to include it in a JSON string
 *
 * @param json_object *jarray: JSON array in which the data will be stored
 * @param int chan: Number of measured channel, if chan is 99 means channel will not be parsed
 * @param float val: Measured magnitude value
 * @param char *magnitude: Name of the measured magnitude
 *
 * @return: 0
 */
int parsing_mon_data_into_array (json_object *jarray,float val, char *magnitude, int chan)
{
	json_object * jobj = json_object_new_object();

	json_object *jdouble = json_object_new_double((double) val);
	json_object *jstring = json_object_new_string(magnitude);
	json_object *jint = json_object_new_int(chan);

	json_object_object_add(jobj,"magnitudename", jstring);
	if (chan != 99)
		json_object_object_add(jobj,"channel", jint);
	json_object_object_add(jobj,"value", jdouble);

	json_object_array_add(jarray,jobj);

	return 0;
}
/**
 * Parses alarms data into a JSON array so as to include it in a JSON string
 *
 * @param json_object *jarray: JSON array in which the data will be stored
 * @param int chan: Number of measured channel, if chan is 99 means channel will not be parsed
 * @param float val: Measured magnitude value
 * @param char *chip: Name of the chip that triggered the alarm
 * @param char *ev_type: Type of event that has occurred
 * @param char *ch_type: Magnitude that has triggered the alarm
 * @param int32_t timestamp: Time when the event occurred
 *
 * @return: 0
 */
int parsing_alm_data_into_array (json_object *jarray,char *chip,char *ev_type, int chan, float val,int32_t timestamp)
{
	json_object * jobj = json_object_new_object();

	json_object *jdouble = json_object_new_double((double) val);
	json_object *jchip = json_object_new_string(chip);
	json_object *jev_type = json_object_new_string(ev_type);
	json_object *jchan = json_object_new_int(chan);
	json_object *jtimestamp = json_object_new_int(timestamp);

	json_object_object_add(jobj,"magnitudename", jchip);
	json_object_object_add(jobj,"event type", jev_type);
	json_object_object_add(jobj,"event timestamp", jtimestamp);
	if (chan != 99)
		json_object_object_add(jobj,"channel", jchan);

	json_object_object_add(jobj,"value", jdouble);

	json_object_array_add(jarray,jobj);

	return 0;
}


/************************** Threads declaration ******************************/

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

	char *curr;
	char *volt;
	char *pwr;

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

	usleep(100);
	printf("Monitoring thread period: %ds\n",MONIT_THREAD_PERIOD/1000000);
	rc = make_periodic(MONIT_THREAD_PERIOD, &info);
	if (rc) {
		printf("Error\r\n");
		return NULL;
	}
	while (1) {
		sem_wait(&i2c_sync); //Semaphore to sync I2C usage
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
		sem_post(&i2c_sync);//Free semaphore to sync I2C usage

		rc = xlnx_ams_read_temp(temp_chan,3,ams_temp);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = xlnx_ams_read_volt(volt_chan,18,ams_volt);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}

		json_object * jobj = json_object_new_object();
		json_object *jdata = json_object_new_object();
		json_object *jlv = json_object_new_array();
		json_object *jhv = json_object_new_array();
		json_object *jdig0 = json_object_new_array();
		json_object *jdig1 = json_object_new_array();
		json_object *jdpb = json_object_new_array();

		parsing_mon_data_into_array(jdpb,temp[0],"Board Temperature",99);

		parsing_mon_data_into_array(jdpb,sfp_temp_0[0],"SFP Temperature",0);
		//parsing_mon_data_into_array(jdpb,sfp_vcc_0[0],"SFP Supply Voltage",0);
		parsing_mon_data_into_array(jdpb,sfp_txbias_0[0],"SFP Laser Bias Current",0);
		parsing_mon_data_into_array(jdpb,sfp_txpwr_0[0],"SFP Tx Optical Power",0);
		parsing_mon_data_into_array(jdpb,sfp_rxpwr_0[0],"SFP Rx Optical Power",0);

		parsing_mon_data_into_array(jdpb,ams_temp[0],"LPD Temperature",temp_chan[0]);
		parsing_mon_data_into_array(jdpb,ams_temp[1],"FPD Temperature",temp_chan[1]);
		parsing_mon_data_into_array(jdpb,ams_temp[2],"PL Temperature",temp_chan[2]);
		/*for(int n = 0; n<25;n++){
			parsing_mon_data_into_array(jdpb,ams_volt[n],"AMS Voltage",volt_chan[n]);		}*/

		for(int j=0;j<3;j++){
			parsing_mon_data_into_array(jdpb,volt_sfp0_2[j],"SFP Voltage",j);
			parsing_mon_data_into_array(jdpb,curr_sfp0_2[j],"SFP Current",j);
			parsing_mon_data_into_array(jdpb,curr_sfp0_2[j]*volt_sfp3_5[j],"SFP Power",j);
		}
		for(int k=0;k<3;k++){
			parsing_mon_data_into_array(jdpb,volt_sfp3_5[k],"SFP Voltage",k+3);
			parsing_mon_data_into_array(jdpb,curr_sfp3_5[k],"SFP Current",k+3);
			parsing_mon_data_into_array(jdpb,curr_sfp3_5[k]*volt_sfp3_5[k],"SFP Power",k+3);
		}
		for(int l=0;l<3;l++){
			switch(l){
			case 0:
				volt = "SoM Voltage (+12V)";
				curr = "SoM Current (+12V)";
				pwr = "SoM Power (+12V)";
				break;
			case 1:
				volt = "SoM Voltage (+3.3V)";
				curr = "SoM Current (+3.3V)";
				pwr = "SoM Power (+3.3V)";
				break;
			case 2:
				volt = "SoM Voltage (+1.8V)";
				curr = "SoM Current (+1.8V)";
				pwr = "SoM Power (+1.8V)";
				break;
			default:
				volt = "SoM Voltage (+12V)";
				curr = "SoM Current (+12V)";
				pwr = "SoM Power (+12V)";
			break;
			}
			parsing_mon_data_into_array(jdpb,volt_som[l],volt,99);
			parsing_mon_data_into_array(jdpb,curr_som[l],curr,99);
			parsing_mon_data_into_array(jdpb,curr_som[l]*volt_som[l],pwr,99);
		}

		json_object_object_add(jdata,"LV", jlv);
		json_object_object_add(jdata,"HV", jhv);
		json_object_object_add(jdata,"Dig0", jdig0);
		json_object_object_add(jdata,"Dig1", jdig1);
		json_object_object_add(jdata,"DPB", jdpb);

		int32_t timestamp = time(NULL);
		json_object *jdevice = json_object_new_string("ID DPB");
		json_object *jtimestamp = json_object_new_int(timestamp);
		json_object_object_add(jobj,"timestamp", jtimestamp);
		json_object_object_add(jobj,"device", jdevice);
		json_object_object_add(jobj,"data",jdata);

		/*FILE* fptr;
		const char *serialized_json = json_object_to_json_string(jobj);
		fptr =  fopen("/run/media/mmcblk0p1/sample.json", "a");
		fwrite(serialized_json,sizeof(char),strlen(serialized_json),fptr);
		fclose(fptr);
		printf("\n");
		printf("The json object created: %sn",json_object_to_json_string(jobj));*/

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
		sem_wait(&i2c_sync); //Semaphore to sync I2C usage

		json_object * jobj = json_object_new_object();
		json_object *jdata = json_object_new_object();
		json_object *jlv = json_object_new_array();
		json_object *jhv = json_object_new_array();
		json_object *jdig0 = json_object_new_array();
		json_object *jdig1 = json_object_new_array();
		json_object *jdpb = json_object_new_array();

		rc = mcp9844_read_alarms(jdpb,data);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_read_alarms(jdpb,data,0);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_read_alarms(jdpb,data,1);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_read_alarms(jdpb,data,2);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}

		/*rc = sfp_avago_read_alarms(jdpb,data,0);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}

		rc = sfp_avago_read_alarms(jdpb,data,1)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(jdpb,data,2)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(jdpb,data,3)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(jdpb,data,4)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(jdpb,data,5)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		*/

		json_object_object_add(jdata,"LV", jlv);
		json_object_object_add(jdata,"HV", jhv);
		json_object_object_add(jdata,"Dig0", jdig0);
		json_object_object_add(jdata,"Dig1", jdig1);
		json_object_object_add(jdata,"DPB", jdpb);

		int32_t timestamp = time(NULL);
		json_object *jdevice = json_object_new_string("ID DPB");
		json_object *jtimestamp = json_object_new_int(timestamp);
		json_object_object_add(jobj,"timestamp", jtimestamp);
		json_object_object_add(jobj,"device", jdevice);
		json_object_object_add(jobj,"alarm data",jdata);
		sem_post(&i2c_sync); //Free semaphore to sync I2C usage
		wait_period(&info);
	}

	return NULL;
}
/**
 * Periodic thread that is waiting for an alarm from any Xilinx AMS channel, the alarm is presented as an event,
 * events are reported by IIO EVENT MONITOR through shared memory.
 *
 * @param void *arg: NULL
 *
 * @return  NULL (if exits is because of an error).
 */
static void *ams_alarms_thread(void *arg){
	FILE *raw,*rising;
	struct periodic_info info;
	int rc ;
	char ev_type[8];
	char ch_type[16];
	int chan;
	__s64 timestamp;
	char ev_str[80];
	char raw_str[80];
	char ris_str[80];
	char buffer [64];
	float res [1];

	strcpy(ev_str, "/sys/bus/iio/devices/iio:device0/events/in_");

	sem_wait(&memory->ams_sync);

	printf("AMS Alarms thread period: %dms\n",AMS_ALARMS_THREAD_PERIOD);
	rc = make_periodic(AMS_ALARMS_THREAD_PERIOD, &info);
	if (rc) {
		printf("Error\r\n");
		return NULL;
	}

	while(1){
        sem_wait(&memory->full);  //Semaphore to wait until any event happens
        res[0] = 0;
        chan = memory->chn;
        strcpy(ev_type,memory->ev_type);
        strcpy(ch_type,memory->ch_type);
        timestamp = (memory->tmpstmp)/1e9; //From ns to s
        snprintf(buffer, sizeof(buffer), "%d",chan);

        if(!strcmp(ch_type,"voltage")){
        	xlnx_ams_read_volt(&chan,1,res);
    		strcpy(raw_str, "/sys/bus/iio/devices/iio:device0/in_voltage");
    		strcpy(ris_str, "/sys/bus/iio/devices/iio:device0/events/in_voltage");

    		strcat(raw_str, buffer);
    		strcat(ris_str, buffer);

    		strcat(raw_str, "_raw");
    		strcat(ris_str, "_thresh_rising_value");

    		raw = fopen(raw_str,"r");
    		rising = fopen(ris_str,"r");

    		if((raw==NULL)|(rising==NULL)){

    			fclose(raw);
    			fclose(rising);
    			printf("AMS Voltage file could not be opened!!! \n");/*Any of the files could not be opened*/
    			return NULL;
    			}
    		else{
    			fseek(raw, 0, SEEK_END);
    			long fsize = ftell(raw);
    			fseek(raw, 0, SEEK_SET);  /* same as rewind(f); */

    			char *raw_string = malloc(fsize + 1);
    			fread(raw_string, fsize, 1, raw);

    			fseek(rising, 0, SEEK_END);
    			fsize = ftell(rising);
    			fseek(rising, 0, SEEK_SET);  /* same as rewind(f); */

    			char *ris_string = malloc(fsize + 1);
    			fread(ris_string, fsize, 1, rising);

    			if(atof(ris_string)>=atof(raw_string))
    				strcpy(ev_type,"falling");
    			else
    				strcpy(ev_type,"rising");

    			printf("Chip: AMS. Event type: %s. Timestamp: %lld. Channel type: %s. Channel: %d. Value: %f V\n",ev_type,timestamp,ch_type,chan,res[0]);
    			fclose(raw);
    			fclose(rising);
    		}

        }
        else if(!strcmp(ch_type,"temp")){
        	xlnx_ams_read_temp(&chan,1,res);
            printf("Chip: AMS. Event type: %s. Timestamp: %lld. Channel type: %s. Channel: %d. Value: %f ºC\n",ev_type,timestamp,ch_type,chan,res[0]);
        }
		wait_period(&info);
	}
	return NULL;
}
int main(){

	//Threads elements
	pthread_t t_1, t_2, t_3;
	sigset_t alarm_sig;
	int i;

	int rc;
	struct DPB_I2cSensors data;

	key_t sharedMemoryKey = MEMORY_KEY;
	memoryID = shmget(sharedMemoryKey, sizeof(struct wrapper), IPC_CREAT | 0600);
	if (memoryID == -1) {
	     perror("shmget():");
	     exit(1);
	 }

	memory = shmat(memoryID, NULL, 0);
	if (memory == (void *) -1) {
	    perror("shmat():");
	    exit(1);
	}

	//printf("Initializtaion !\n");
	strcpy(memory->ev_type,"");
	strcpy(memory->ch_type,"");
	sem_init(&memory->ams_sync, 1, 0);
	sem_init(&memory->empty, 1, 1);
	sem_init(&memory->full, 1, 0);
	memory->chn = 0;
	memory->tmpstmp = 0;

	if (memoryID == -1) {
	    perror("shmget(): ");
	    exit(1);
	 }

	rc = init_I2cSensors(&data); //Initialize i2c sensors

	if (rc) {
		printf("Error\r\n");
		return 0;
	}

	rc = iio_event_monitor_up(); //Initialize iio event monitor
	if (rc) {
		printf("Error\r\n");
		return rc;
	}

	sem_init(&i2c_sync,0,1);
	/* Block all real time signals so they can be used for the timers.
	   Note: this has to be done in main() before any threads are created
	   so they all inherit the same mask. Doing it later is subject to
	   race conditions*/

	sigemptyset(&alarm_sig);
	for (i = SIGRTMIN; i <= SIGRTMAX; i++)
		sigaddset(&alarm_sig, i);
	sigprocmask(SIG_BLOCK, &alarm_sig, NULL);

	pthread_create(&t_1, NULL, ams_alarms_thread,NULL); //Create thread 1 - read AMS alarms
	pthread_create(&t_2, NULL, i2c_alarms_thread,(void *)&data); //Create thread 2 - read alarms every x miliseconds
	pthread_create(&t_3, NULL, monitoring_thread,(void *)&data );//Create thread 3 - monitors magnitudes every x seconds

	while(1){
		sleep(1000000);
		}
	//stop_I2cSensors(&data);
	return 0;
}
