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
#include <zmq.h>
#include "json-c/json.h"
#include <math.h>
 #include <dirent.h>
#include <regex.h>

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
/******************************************************************************
*ZMQ Socket Initializer
****************************************************************************/
void *mon_context ;
void *mon_publisher;
void *log_context ;
void *log_publisher ;
void *cmd_context;
void *cmd_router;
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
int mcp9844_interruptions(struct DPB_I2cSensors *, uint8_t );
int mcp9844_read_alarms(struct DPB_I2cSensors *);
int sfp_avago_read_temperature(struct DPB_I2cSensors *,int , float *);
int sfp_avago_read_voltage(struct DPB_I2cSensors *,int , float *);
int sfp_avago_read_lbias_current(struct DPB_I2cSensors *,int, float *);
int sfp_avago_read_tx_av_optical_pwr(struct DPB_I2cSensors *,int, float *);
int sfp_avago_read_rx_av_optical_pwr(struct DPB_I2cSensors *,int, float *);
int sfp_avago_read_status(struct DPB_I2cSensors *,int ,uint8_t *);
int sfp_avago_status_interruptions(uint8_t, int);
int sfp_avago_alarms_interruptions(struct DPB_I2cSensors *,uint16_t , int );
int sfp_avago_read_alarms(struct DPB_I2cSensors *,int ) ;
int ina3221_get_voltage(struct DPB_I2cSensors *,int , float *);
int ina3221_get_current(struct DPB_I2cSensors *,int , float *);
int ina3221_critical_interruptions(struct DPB_I2cSensors *,uint16_t , int );
int ina3221_warning_interruptions(struct DPB_I2cSensors *,uint16_t , int );
int ina3221_read_alarms(struct DPB_I2cSensors *,int);
int ina3221_set_limits(struct DPB_I2cSensors *,int ,int ,int  ,float );
int ina3221_set_config(struct DPB_I2cSensors *,uint8_t *,uint8_t *, int );
int parsing_mon_sensor_data_into_array (json_object *,float , char *, int );
int parsing_mon_status_data_into_array(json_object *, int , char *,int );
int alarm_json (json_object *,char *,char *, int , float ,int32_t ,char *);
int status_alarm_json (json_object *,char *,char *, int ,int32_t ,char *);
int get_GPIO_base_address(int *);
int write_GPIO(int , int );
int read_GPIO(int ,int *);
int eth_link_status (char *,int *);
int eth_down_alarm(char *,int *);
int aurora_down_alarm(int ,int *);
int zmq_socket_init ();
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
int mcp9844_interruptions(struct DPB_I2cSensors *data, uint8_t flag_buf){
	int32_t timestamp;
	float res [1];
	int rc = 0;
	mcp9844_read_temperature(data,res);
	json_object *jobj = json_object_new_object();

	if((flag_buf & 0x80) == 0x80){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		rc = alarm_json(jobj,"PCB Temperature","critical", 99, res[0],timestamp,"critical");
	}
	if((flag_buf & 0x40) == 0x40){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		rc = alarm_json(jobj,"PCB Temperature","rising", 99, res[0],timestamp,"warning");
	}
	if((flag_buf & 0x20) == 0x20){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		rc = alarm_json(jobj,"PCB Temperature","falling", 99, res[0],timestamp,"warning");
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
		mcp9844_interruptions(data,alarm_buf[0]);
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
 * @param struct DPB_I2cSensors *data: I2C devices
 * @param int n: indicate from which of the 6 SFP is going to be read
 * @param float *res where the magnitude value is stored
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
 * @param struct DPB_I2cSensors *data: I2C devices
 * @param int n: indicate from which of the 6 SFP is going to be read
 * @param float *res: where the magnitude value is stored
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
 * @param struct DPB_I2cSensors *data: I2C devices
 * @param int n: indicate from which of the 6 SFP is going to be read
 * @param float *res: where the magnitude value is stored
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
 * @param struct DPB_I2cSensors *data: I2C devices
 * @param int n: indicate from which of the 6 SFP is going to be read
 * @param float *res: where the magnitude value is stored
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
 * @param struct DPB_I2cSensors *data: I2C devices
 * @param int n: indicate from which of the 6 SFP is going to be read,
 * @param float *res: where the magnitude value is stored
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
 * HReads SFP current RX_LOS and TX_FAULT status
 *
 * @param struct DPB_I2cSensors *data: I2C devices
 * @param int n: indicate from which of the 6 SFP is dealing with
 * @param uint8_t * res : stores the current RX_LOS and TX_FAULT status
 *
 * @return 0 if reads properly and stores 0 or 1 depending on the current states (1 if status asserted, 0 if not)
 */
int sfp_avago_read_status(struct DPB_I2cSensors *data,int n,uint8_t *res) {
	int rc = 0;
	uint8_t status_buf[1] = {0};
	uint8_t status_reg = SFP_STAT_REG;

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
	//Set RX_LOS Status Flag
	if(status_buf[0] & 0x02)
		res[0] = 1;
	else
		res[0] = 0;
	//Set TX_FAULT Status flag
	if(status_buf[0] & 0x04)
		res[1] = 1;
	else
		res[1] = 0;
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
	int32_t timestamp;
	json_object *jobj = json_object_new_object();
	int rc = 0;

	if((status & 0x02) != 0){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		rc = status_alarm_json(jobj,"DPB","SFP RX_LOS Status",n,timestamp,"critical");
	}
	if((status & 0x04) != 0){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		rc = status_alarm_json(jobj,"DPB","SFP TX_FAULT Status", n,timestamp,"critical");
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
int sfp_avago_alarms_interruptions(struct DPB_I2cSensors *data,uint16_t flags, int n){
	int32_t timestamp;
	float res [1];
	json_object *jobj = json_object_new_object();
	int rc = 0;

	if((flags & 0x0080) == 0x0080){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		sfp_avago_read_rx_av_optical_pwr(data,n,res);
		rc = alarm_json(jobj,"SFP RX Power","rising", n, res[0],timestamp,"warning");
	}
	if((flags & 0x0040) == 0x0040){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		sfp_avago_read_rx_av_optical_pwr(data,n,res);
		rc = alarm_json(jobj,"SFP RX Power","falling", n, res[0],timestamp,"warning");
	}
	if((flags & 0x0200) == 0x0200){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		sfp_avago_read_tx_av_optical_pwr(data,n,res);
		rc = alarm_json(jobj,"SFP TX Power","rising", n, res[0],timestamp,"warning");
	}
	if((flags & 0x0100) == 0x0100){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		sfp_avago_read_tx_av_optical_pwr(data,n,res);
		rc = alarm_json(jobj,"SFP TX Power","falling", n, res[0],timestamp,"warning");
	}
	if((flags & 0x0800) == 0x0800){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		sfp_avago_read_lbias_current(data,n,res);
		rc = alarm_json(jobj,"SFP Laser Bias Current","rising", n, res[0],timestamp,"warning");
	}
	if((flags & 0x0400) == 0x0400){
		timestamp = time(NULL);
		sfp_avago_read_lbias_current(data,n,res);
		rc = alarm_json(jobj,"SFP Laser Bias Current","falling", n, res[0],timestamp,"warning");
	}
	if((flags & 0x2000) == 0x2000){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		sfp_avago_read_voltage(data,n,res);
		rc = alarm_json(jobj,"SFP Voltage Monitor","rising", n, res[0],timestamp,"warning");
	}
	if((flags & 0x1000) == 0x1000){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		sfp_avago_read_voltage(data,n,res);
		rc = alarm_json(jobj,"SFP Voltage Monitor","falling", n, res[0],timestamp,"warning");
	}
	if((flags & 0x8000) == 0x8000){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		sfp_avago_read_temperature(data,n,res);
		rc = alarm_json(jobj,"SFP Temperature","rising", n, res[0],timestamp,"warning");
	}
	if((flags & 0x4000) == 0x4000){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		sfp_avago_read_temperature(data,n,res);
		rc = alarm_json(jobj,"SFP Temperature","falling", n, res[0],timestamp,"warning");
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
		sfp_avago_alarms_interruptions(data,flags,n);
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
 * @param float *res: storage of collected data
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
 * @param float *res: storage of collected data
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
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device INA3221 Voltage and Current Sensor
 * @param uint16_t mask contains critical alarm flags
 * @param int n indicate from which of the 3 INA3221 is dealing with
 *
 * @return 0 and handles interruption depending on the active alarms flags
 */
int ina3221_critical_interruptions(struct DPB_I2cSensors *data,uint16_t mask, int n){
	int32_t timestamp;
	float res[3];
	ina3221_get_current(data,n,res);
	int k = 0;
	int rc = 0;
	json_object *jobj = json_object_new_object();
	if (n == 1)
		k = 3;

	if((mask & 0x0080) == 0x0080){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		if(n == 2)
			rc = alarm_json(jobj,"Current Monitor (+12V)","rising", 99, res[2],timestamp,"critical");
		else
			rc = alarm_json(jobj,"SFP Current Monitor","rising", k+2, res[2],timestamp,"critical");		}
	if((mask & 0x0100) == 0x0100){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		if(n == 2)
			rc = alarm_json(jobj,"Current Monitor (+3.3V)","rising", 99, res[2],timestamp,"critical");
		else
			rc = alarm_json(jobj,"SFP Current Monitor","rising", k+2, res[2],timestamp,"critical");	}
	if((mask & 0x0200) == 0x0200){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		if(n == 2)
			rc = alarm_json(jobj,"Current Monitor (+1.8V)","rising", 99, res[2],timestamp,"critical");
		else
			rc = alarm_json(jobj,"SFP Current Monitor","rising", k+2, res[2],timestamp,"critical");	}
	return 0;
}
/**
 * Handles INA3221 Voltage and Current Sensor warning alarm interruptions
 *
 * @param struct DPB_I2cSensors *data: being the corresponding I2C device INA3221 Voltage and Current Sensor
 * @param uint16_t mask: contains warning alarm flags
 * @param int n: indicate from which of the 3 INA3221 is dealing with
 *
 * @return 0 and handles interruption depending on the active alarms flags
 */
int ina3221_warning_interruptions(struct DPB_I2cSensors *data,uint16_t mask, int n){

	int32_t timestamp;
	float res[3];
	ina3221_get_current(data,n,res);
	json_object *jobj = json_object_new_object();
	int k = 0;
	int rc = 0;
	if (n == 1) //Number of INA3221
		k = 3;

	if((mask & 0x0008) == 0x0008){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		if(n == 2)
			rc = alarm_json(jobj,"Current Monitor (+12V)","rising", 99, res[0],timestamp,"warning");
		else
			rc = alarm_json(jobj,"SFP Current Monitor","rising", k, res[0],timestamp,"warning");	}
	if((mask & 0x0010) == 0x0010){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		if(n == 2)
			rc = alarm_json(jobj,"Current Monitor (+3.3V)","rising", 99, res[1],timestamp,"warning");
		else
			rc = alarm_json(jobj,"SFP Current Monitor","rising", k+1, res[1],timestamp,"warning");	}
	if((mask & 0x0020) == 0x0020){
		jobj = json_object_new_object();
		timestamp = time(NULL);
		if(n == 2)
			rc = alarm_json(jobj,"Current Monitor (+1.8V)","rising", 99, res[2],timestamp,"warning");
		else
			rc = alarm_json(jobj,"SFP Current Monitor","rising", k+2, res[2],timestamp,"warning");	}
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
		ina3221_critical_interruptions(data,mask_int,n);
	}
	else if((mask_int & 0x0038)!= 0){
		ina3221_warning_interruptions(data,mask_int,n);
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
 * Parses monitoring data into a JSON array so as to include it in a JSON object
 *
 * @param json_object *jarray: JSON array in which the data will be stored
 * @param int chan: Number of measured channel, if chan is 99 means channel will not be parsed
 * @param float val: Measured magnitude value
 * @param char *magnitude: Name of the measured magnitude
 *
 * @return: 0
 */
int parsing_mon_sensor_data_into_array (json_object *jarray,float val, char *magnitude, int chan)
{
	json_object * jobj = json_object_new_object();
	char buffer[8];

	sprintf(buffer, "%3.4f", val);
	json_object *jdouble = json_object_new_double_s((double) val,buffer);
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
 * Parses monitoring status data into a JSON array so as to include it in a JSON object
 *
 * @param json_object *jarray: JSON array in which the data will be stored
 * @param int status: Value of the status
 * @param char *magnitude: Name of the measured magnitude/interface
 * @param int chan: Number of measured channel, if chan is 99 means channel will not be parsed
 *
 * @return: 0
 */
int parsing_mon_status_data_into_array(json_object *jarray, int status, char *magnitude, int chan)
{
	json_object * jobj = json_object_new_object();
	json_object *jstatus ;

	json_object *jstring = json_object_new_string(magnitude);
	json_object *jint = json_object_new_int(chan);
	if(status == 1)
		jstatus = json_object_new_string("ON");
	else if (status == 0)
		jstatus = json_object_new_string("OFF");

	json_object_object_add(jobj,"magnitudename", jstring);
	if (chan != 99)
		json_object_object_add(jobj,"channel", jint);
	json_object_object_add(jobj,"value", jstatus);

	json_object_array_add(jarray,jobj);

	return 0;
}
/**
 * Parses alarms data into a JSON object
 *
 * @param json_object *jobj: JSON object where data will be parsed
 * @param int chan: Number of measured channel, if chan is 99 means channel will not be parsed
 * @param float val: Measured magnitude value
 * @param char *chip: Name of the chip that triggered the alarm
 * @param char *ev_type: Type of event that has occurred
 * @param int32_t timestamp: Time when the event occurred
 * @param char *info_type: Determines the reported event type (inof,warning or critical)
 *
 *
 * @return: 0
 */
int alarm_json (json_object *jobj,char *chip,char *ev_type, int chan, float val,int32_t timestamp,char *info_type)
{
	json_object *jalarm_data = json_object_new_object();
	char buffer[8];

	int32_t timestamp_msg = time(NULL);

	sprintf(buffer, "%3.4f", val);

	json_object *jdevice = json_object_new_string("ID DPB");
	json_object *jboard = json_object_new_string("DPB");
	json_object *jinfo_type = json_object_new_string(info_type);
	json_object *jtimestamp_msg = json_object_new_int(timestamp_msg);

	json_object_object_add(jobj,"timestamp", jtimestamp_msg);
	json_object_object_add(jobj,"device", jdevice);
	json_object_object_add(jobj,"board", jboard);
	json_object_object_add(jobj,"info", jinfo_type);

	json_object *jdouble = json_object_new_double_s((double) val,buffer);
	json_object *jchip = json_object_new_string(chip);
	json_object *jev_type = json_object_new_string(ev_type);
	json_object *jchan = json_object_new_int(chan);
	json_object *jtimestamp = json_object_new_int(timestamp);

	json_object_object_add(jalarm_data,"magnitudename", jchip);
	json_object_object_add(jalarm_data,"eventtype", jev_type);
	json_object_object_add(jalarm_data,"eventtimestamp", jtimestamp);
	if (chan != 99)
		json_object_object_add(jalarm_data,"channel", jchan);

	json_object_object_add(jalarm_data,"value", jdouble);

	json_object_object_add(jobj,"data", jalarm_data);

	const char *serialized_json = json_object_to_json_string(jobj);
	zmq_send (log_publisher, strdup(serialized_json), strlen(serialized_json), 0);

	return 0;
}

/**
 * Parses alarms data into a JSON object
 *
 * @param json_object *jobj: JSON object where data will be parsed
 * @param int chan: Number of measured channel, if chan is 99 means channel will not be parsed (also indicates it is not SFP related)
 * @param char *chip: Name of the chip that triggered the alarm
 * @param char *board: Name of the board where the alarm is asserted
 * @param int32_t timestamp: Time when the event occurred
 * @param char *info_type: Determines the reported event type (inof,warning or critical)
 *
 *
 * @return: 0
 */
int status_alarm_json (json_object *jobj,char *board,char *chip, int chan,int32_t timestamp,char *info_type)
{
	json_object *jalarm_data = json_object_new_object();

	int32_t timestamp_msg = time(NULL);

	json_object *jdevice = json_object_new_string("ID DPB");
	json_object *jboard = json_object_new_string(board);
	json_object *jinfo_type = json_object_new_string(info_type);
	json_object *jtimestamp_msg = json_object_new_int(timestamp_msg);

	json_object_object_add(jobj,"timestamp", jtimestamp_msg);
	json_object_object_add(jobj,"device", jdevice);
	json_object_object_add(jobj,"board", jboard);
	json_object_object_add(jobj,"info", jinfo_type);


	json_object *jchip = json_object_new_string(chip);
	json_object *jchan = json_object_new_int(chan);
	json_object *jtimestamp = json_object_new_int(timestamp);
	json_object *jstatus ;

	json_object_object_add(jalarm_data,"magnitudename", jchip);
	json_object_object_add(jalarm_data,"eventtimestamp", jtimestamp);
	if (chan != 99){
		json_object_object_add(jalarm_data,"channel", jchan);
		jstatus = json_object_new_string("ON");
	}
	else{
		jstatus = json_object_new_string("OFF");
	}

	json_object_object_add(jalarm_data,"value", jstatus);

	json_object_object_add(jobj,"data", jalarm_data);

	const char *serialized_json = json_object_to_json_string(jobj);
	zmq_send(log_publisher, strdup(serialized_json), strlen(serialized_json), 0);

	return 0;
}
/************************** GPIO functions ******************************/
/**
 * Gets GPIO base address
 *
 * @param int *address: pointer where the read GPIO base address plus corresponding offset will be stored
 *
 * @return: 0
 */
int get_GPIO_base_address(int *address){

	char GPIO_dir[64] = "/sys/class/gpio/";
	regex_t r1;
	DIR *dp;
	FILE *GPIO;

	int data = 0;
	int i = 0;
	char *arr[8];
	long bytes = 0;
	char label_str[64];
	struct dirent *entry;
	dp = opendir (GPIO_dir);

	data = regcomp(&r1, "gpiochip.*", 0);


	while ((entry = readdir (dp)) != NULL){
		data = regexec(&r1, entry->d_name, 0, NULL, 0);
		if(data == 0){
			arr[i] = entry->d_name;
			i++;
		}
	}
	for(int j=0; j<i; j++){

		strcat(GPIO_dir,arr[j]);
		strcat(GPIO_dir,"/label");
		GPIO = fopen(GPIO_dir,"r");

		strcpy(GPIO_dir,"/sys/class/gpio/");
		fread(label_str, sizeof(label_str), 1, GPIO);
		fwrite(label_str, bytes, 1, stdout);
		if(!(strcmp(label_str,"zynqmp_gpio\n"))){
		    	fclose(GPIO);
		    	strcat(GPIO_dir,arr[j]);
		    	strcat(GPIO_dir,"/base");
		    	GPIO = fopen(GPIO_dir,"r");
		    	fseek(GPIO, 0, SEEK_END);
		    	long fsize = ftell(GPIO);
		    	fseek(GPIO, 0, SEEK_SET);  /* same as rewind(f); */

		    	char *add_string = malloc(fsize + 1);
		    	fread(add_string, fsize, 1, GPIO);
		    	address[0] = (int) atof(add_string) + 78 ;
				fclose(GPIO);
				break;
			}
		fclose(GPIO);
	}
	return 0;
}

/**
 * Writes into a given GPIO address
 *
 * @param int address: GPIO address where the value is going to be written
 * @param int value: value which will be written (0 o 1)
 *
 * @return: 0 if worked correctly, if not returns a negative integer.
 */
int write_GPIO(int address, int value){

	char cmd1[64];
	char cmd2[64];
	char dir_add[64];
	char val_add[64];
    FILE *fd1;
    FILE *fd2;
    char val[1];
    char *dir = "out";

    if((value != 0) && (value != 1) )
    	return -EINVAL;

    val[0] = value + '0';
	int add = address + GPIO_BASE_ADDRESS;

    // Building first command
    snprintf(cmd1, sizeof(cmd1), "echo %d > /sys/class/gpio/export", add);

    // Building GPIO sysfs file
    if (system(cmd1) == -1) {
        return -EINVAL;
    }
    snprintf(dir_add, sizeof(dir_add), "/sys/class/gpio/gpio%d/direction", add);
    snprintf(val_add, sizeof(val_add), "/sys/class/gpio/gpio%d/value", add);

    fd1 = fopen(dir_add,"w");
    fwrite(dir, sizeof(dir), 1,fd1);
    fclose(fd1);

    fd2 = fopen(val_add,"w");
    fwrite(val,sizeof(val), 1,fd2);
    fclose(fd2);

    // Building second command
    snprintf(cmd2, sizeof(cmd2), "echo %d > /sys/class/gpio/unexport", add);

    //Removing GPIO sysfs file
    if (system(cmd2) == -1) {
        return -EINVAL;
    }
	return 0;
}

/**
 * Gets GPIO base address
 *
 * @param int address: GPIO address where the desired value is stored
 * @param int *value: pointer where the read value will be stored
 *
 * @return: 0 if worked correctly, if not returns a negative integer.
 */
int read_GPIO(int address,int *value){

	char cmd1[64];
	char cmd2[64];
	char dir_add[64];
	char val_add[64];
    FILE *fd1;
    char *dir = "in";
    FILE *GPIO_val;

	int add = address + GPIO_BASE_ADDRESS;
    // Building first command
    snprintf(cmd1, sizeof(cmd1), "echo %d > /sys/class/gpio/export", add);

    // Building GPIO sysfs file
    if (system(cmd1) == -1) {
        return -EINVAL;
    }
    snprintf(dir_add, sizeof(dir_add), "/sys/class/gpio/gpio%d/direction", add);
    snprintf(val_add, sizeof(val_add), "/sys/class/gpio/gpio%d/value", add);


    fd1 = fopen(dir_add,"w");
    fwrite(dir, sizeof(dir), 1,fd1);
    fclose(fd1);

    GPIO_val = fopen(val_add,"r");
	fseek(GPIO_val, 0, SEEK_END);
	long fsize = ftell(GPIO_val);
	fseek(GPIO_val, 0, SEEK_SET);  /* same as rewind(f); */

	char *value_string = malloc(fsize + 1);
	fread(value_string, fsize, 1, GPIO_val);
	value[0] = (int) atof(value_string);
	fclose(GPIO_val);

    // Building second command
    snprintf(cmd2, sizeof(cmd2), "echo %d > /sys/class/gpio/unexport", add);

    //Removing GPIO sysfs file
    if (system(cmd2) == -1) {
        return -EINVAL;
    }

	return 0;
}

/************************** External monitoring (via GPIO) functions ******************************/
/**
 * Checks from GPIO if Ethernet Links status and reports it
 *
 * @param char *eth_interface: Name of the Ethernet interface
 * @param int status: value of the Ethernet interface status
 *
 * @return  0 if parameters are OK, if not negative integer
 */
int eth_link_status (char *eth_interface, int *status)
{
	char eth_link[64];
	FILE *link_file;
	char *str;

	char cmd[64] = "ethtool ";

	strcat(cmd,eth_interface);
	strcat(cmd," | grep 'Link detected'");

	link_file = popen(cmd, "r");
	fread(eth_link, sizeof(eth_link), 1, link_file);
	pclose(link_file);

	strtok(eth_link," ");
	strtok(NULL," ");
	str = strtok(NULL,"\n");
	strcat(str,"");
	if((strcmp(str,"yes")) == 0)
		status[0] = 1;
	else if((strcmp(str,"no")) == 0)
		status[0] = 0;
	else
		return -EINVAL;
	return 0;

}

/************************** External monitoring (via GPIO) functions ******************************/
/**
 * Updates Ethernet interface status to ON/OFF
 *
 * @param char *eth_interface: Name of the Ethernet interface
 * @param int val: value of the Ethernet interface status
 *
 * @return  0 if parameters are OK, if not negative integer
 */
int eth_link_status_config (char *eth_interface, int val)
{
	char eth_link[32];
	char cmd[64];
	if(strcmp(eth_interface,"ETH0") == 0){
		strcpy(eth_link,"eth0");
	}
	else{
		strcpy(eth_link,"eth1");
	}
	strcpy(cmd,"ifconfig ");
	strcat(cmd,eth_link);
	if(val == 1){
		strcat(cmd," up");
	}
	else if (val == 0){
		strcat(cmd," down");
	}
	else{
		return -EINVAL;
	}
	system(cmd);

	return 0;

}
/************************** External alarms (via GPIO) functions ******************************/
/*
* Checks from GPIO if Ethernet Links status has changed from up to down and reports it if necessary
*
* @param char *eth_interface: Name of the Ethernet interface
* @param int status: value of the Ethernet interface flag, determines if the link was previously up
*
* @return  0 if parameters OK and reports the event, if not returns negative integer.
*/
int eth_down_alarm(char *str,int *flag){

	int eth_status[1];
	int rc = 0;
	int32_t timestamp ;
	json_object *jobj = json_object_new_object();

    if((flag[0] != 0) && (flag[0] != 1)){
    	return -EINVAL;
    }
	if((strcmp(str,"eth0")) && (strcmp(str,"eth1"))){
		return -EINVAL;}

	rc = eth_link_status(str,&eth_status[0]);
	if (rc) {
		printf("Error\r\n");
		return rc;
	}
	if((flag[0] == 0) & (eth_status[0] == 1)){
		flag[0] = eth_status[0];}
	if((flag[0] == 1) & (eth_status[0] == 0)){
		flag[0] = eth_status[0];
		if(!(strcmp(str,"eth0"))){
			jobj = json_object_new_object();
			timestamp = time(NULL);
			rc = status_alarm_json(jobj,"DPB","Main Ethernet Link Status",99,timestamp,"critical");
			return rc;
		}
		else if(!(strcmp(str,"eth1"))){
			jobj = json_object_new_object();
			timestamp = time(NULL);
			rc = status_alarm_json(jobj,"DPB","Backup Ethernet Link Status",99,timestamp,"critical");
			return rc;
		}
	}
	return 0;
}
/**
* Checks from GPIO if Ethernet Links status has changed from up to down and reports it if necessary
 *
 * @param int aurora_link: Choose main or backup link of Dig0 or Dig1 (O: Dig0 Main, 1:Dig0 Backup, 2:Dig1 Main, 3:Dig1 Backup)
 * @param int flags: indicates current status of the link
 *
 * @return  0 if parameters are OK, if not negative integer
 */
int aurora_down_alarm(int aurora_link,int *flag){

	int aurora_status[1];
	int rc = 0;
	int address = 0;
	int32_t timestamp ;
	char *link_id;
	json_object *jobj = json_object_new_object();

    if((flag[0] != 0) && (flag[0] != 1)){
    	return -EINVAL;
    }
	if((aurora_link>3) | (aurora_link<0)){
		return -EINVAL;}
	switch(aurora_link){
	case 0:
		address = DIG0_MAIN_AURORA_LINK;
		link_id = "Aurora Main Link Status";
		break;
	case 1:
		address = DIG0_BACKUP_AURORA_LINK;
		link_id = "Aurora Main Link Status";
		break;
	case 2:
		address = DIG1_MAIN_AURORA_LINK;
		link_id = "Aurora Main Link Status";
		break;
	case 3:
		address = DIG1_BACKUP_AURORA_LINK;
		link_id = "Aurora Main Link Status";
		break;
	default:
		return -EINVAL;
	}

	rc = read_GPIO(address,&aurora_status[0]);
	if (rc) {
		printf("Error\r\n");
		return rc;
	}
	if((flag[0] == 0) & (aurora_status[0] == 1)){
		flag[0] = aurora_status[0];}
	if((flag[0] == 1) & (aurora_status[0] == 0)){
		flag[0] = aurora_status[0];
		if(aurora_link<2){
			jobj = json_object_new_object();
			timestamp = time(NULL);
			rc = status_alarm_json(jobj,"Dig0",link_id,99,timestamp,"critical");
			return rc;
		}
		else{
			jobj = json_object_new_object();
			timestamp = time(NULL);
			rc = status_alarm_json(jobj,"Dig1",link_id,99,timestamp,"critical");
			return rc;
		}
	}
	return 0;
}

/************************** ZMQ Functions******************************/
/*
* Initializes ZMQ monitoring, command and logging sockets
*
* @return  0 if parameters OK and reports the event, if not returns negative integer.
*/
int zmq_socket_init (){

	int rc = 0;
    mon_context = zmq_ctx_new();
    mon_publisher = zmq_socket(mon_context, ZMQ_PUB);
    rc = zmq_bind(mon_publisher, "tcp://127.0.0.1:5555");
	if (rc) {
		return rc;
	}
    log_context = zmq_ctx_new();
    log_publisher = zmq_socket(log_context, ZMQ_PUB);
    rc = zmq_bind(log_publisher, "tcp://127.0.0.1:5556");
	if (rc) {
		return rc;
	}
    cmd_context = zmq_ctx_new();
    cmd_router = zmq_socket(cmd_context, ZMQ_ROUTER);
    rc = zmq_bind(cmd_router, "tcp://127.0.0.1:5557");
	if (rc) {
		return rc;
	}
	return 0;
}
/************************** Command handling Functions******************************/
/*
* Handles received DPB command
*
* @return  0 if parameters OK and reports the event, if not returns negative integer.
*/
void dpb_command_handling(struct DPB_I2cSensors *data, char **cmd){

	regex_t r1;
	int reg_exp;
	int rc;
	int bool_set;
	int bool_read[1];
	int ams_chan[1];
	int chan ;
	float val_read[1];
	float ina3221_read[3];


	reg_exp = regcomp(&r1, "SFP.*", 0);
	reg_exp = regexec(&r1, cmd[3], 0, NULL, 0);
	if(reg_exp == 0){  //SFP
		char *sfp_num_str = strtok(cmd[3],"SFP");
		int sfp_num = atoi(sfp_num_str);

		if(strcmp(cmd[2],"STATUS") == 0){
			if(strcmp(cmd[0],"READ") == 0){
				rc = read_GPIO(SFP0_TX_ENA+(sfp_num*4),bool_read);
			}
			else{
				bool_set=((strcmp(cmd[4],"ON") == 0)?(1):(0));
				rc = write_GPIO(SFP0_TX_ENA+sfp_num,bool_set);
			}
		}
		if(strcmp(cmd[2],"VOLT") == 0){
			if(strcmp(cmd[0],"READ") == 0){
				rc = sfp_avago_read_voltage(data,sfp_num,val_read);
			}
		}
		if(strcmp(cmd[2],"CURR") == 0){
			if(strcmp(cmd[0],"READ") == 0){
				rc = ina3221_get_current(data,(sfp_num/3),ina3221_read);
				val_read[0] = ina3221_read[sfp_num%3];
			}
			else{
				rc = ina3221_set_limits(data,(sfp_num/3),sfp_num%3,1,atof(cmd[4]));
			}
		}
		if(strcmp(cmd[2],"TEMP") == 0){
			if(strcmp(cmd[0],"READ") == 0){
				rc = sfp_avago_read_temperature(data,sfp_num,val_read);
			}
		}
		if(strcmp(cmd[2],"RXPWR") == 0){
			if(strcmp(cmd[0],"READ") == 0){
				rc = sfp_avago_read_rx_av_optical_pwr(data,sfp_num,val_read);
			}
		}
		if(strcmp(cmd[2],"TXPWR") == 0){
			if(strcmp(cmd[0],"READ") == 0){
				rc = sfp_avago_read_tx_av_optical_pwr(data,sfp_num,val_read);
			}
		}
	}
	else{
		if(strcmp(cmd[2],"STATUS") == 0){
			if(strcmp(cmd[0],"READ") == 0){
				if(strcmp(cmd[3],"ETH0") == 0){
					rc = eth_link_status("eth0",bool_read);
				}
				else{
					rc = eth_link_status("eth1",bool_read);
				}
			}
			else{
				bool_set=((strcmp(cmd[4],"ON") == 0)?(1):(0));
				rc = eth_link_status_config(cmd[3], bool_set);
			}
		}
		if(strcmp(cmd[2],"VOLT") == 0){
			if(strcmp(cmd[0],"READ") == 0){
				if(strcmp(cmd[0],"FPDCPU") == 0){
					ams_chan[0] = 10;
					rc = xlnx_ams_read_volt(ams_chan,1,val_read);
				}
				else if(strcmp(cmd[0],"LPDCPU") == 0){
					ams_chan[0] = 9;
					rc = xlnx_ams_read_volt(ams_chan,1,val_read);
				}
				else{
					chan = ((strcmp(cmd[3],"12V") == 0)) ? 0 : ((strcmp(cmd[3],"3V3") == 0)) ? 1 : 2;
					rc = ina3221_get_voltage(data,2,ina3221_read);
					val_read[0] = ((strcmp(cmd[3],"12V") == 0))? ina3221_read[0] :((strcmp(cmd[3],"3V3") == 0))? ina3221_read[1] :ina3221_read[2];
				}
			}
			else{
				if(strcmp(cmd[0],"FPDCPU") == 0){
					rc = xlnx_ams_set_limits(8,"voltage","rising",atof(cmd[4]));
				}
				else if(strcmp(cmd[0],"LPDCPU") == 0){
					rc = xlnx_ams_set_limits(7,"voltage","rising",atof(cmd[4]));
				}
			}
		}
		if(strcmp(cmd[2],"CURR") == 0){
			if(strcmp(cmd[0],"READ") == 0){
				rc = ina3221_get_current(data,2,ina3221_read);
				val_read[0] = ((strcmp(cmd[3],"12V") == 0))? ina3221_read[0] :((strcmp(cmd[3],"3V3") == 0))? ina3221_read[1] :ina3221_read[2];
			}
			else{
				chan = ((strcmp(cmd[3],"12V") == 0)) ? 0 : ((strcmp(cmd[3],"3V3") == 0)) ? 1 : 2;
				rc = ina3221_set_limits(data,2,chan,1,atof(cmd[4]));
			}
		}
		if(strcmp(cmd[2],"TEMP") == 0){
			if(strcmp(cmd[0],"READ") == 0){
				if(strcmp(cmd[0],"FPDCPU") == 0){
					ams_chan[0] = 8;
					rc = xlnx_ams_read_temp(ams_chan,1,val_read);
				}
				else if(strcmp(cmd[0],"LPDCPU") == 0){
					ams_chan[0] = 7;
					rc = xlnx_ams_read_temp(ams_chan,1,val_read);
				}
				else if(strcmp(cmd[0],"FPGA") == 0){
					ams_chan[0] = 20;
					rc = xlnx_ams_read_temp(ams_chan,1,val_read);
				}
				else{
					rc = mcp9844_read_temperature(data,val_read);
				}
			}
			else{
				if(strcmp(cmd[0],"FPDCPU") == 0){
					rc = xlnx_ams_set_limits(8,"temp","rising",atof(cmd[4]));
				}
				else if(strcmp(cmd[0],"LPDCPU") == 0){
					rc = xlnx_ams_set_limits(7,"temp","rising",atof(cmd[4]));
				}
				else if(strcmp(cmd[0],"FPGA") == 0){
					rc = xlnx_ams_set_limits(20,"temp","rising",atof(cmd[4]));
				}
				else{
					rc = mcp9844_set_limits(data,0,atof(cmd[4]));
				}
			}
		}
	}

	return;
}
/************************** Threads declaration ******************************/
/*
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

	int eth_status[2];
	int aurora_status[4];

	char *curr;
	char *volt;
	char *pwr;

	float ams_temp[AMS_TEMP_NUM_CHAN];
	float ams_volt[AMS_VOLT_NUM_CHAN];
	int temp_chan[AMS_TEMP_NUM_CHAN] = {7,8,20};
	int volt_chan[AMS_VOLT_NUM_CHAN] = {9,10,11,12,13,14,15,16,17,18,19,21,22,23,24,25,26,27,28,29};

	float volt_sfp0_2[INA3221_NUM_CHAN];
	float volt_sfp3_5[INA3221_NUM_CHAN];
	float volt_som[INA3221_NUM_CHAN];

	float curr_sfp0_2[INA3221_NUM_CHAN];
	float curr_sfp3_5[INA3221_NUM_CHAN];
	float curr_som[INA3221_NUM_CHAN];
	float pwr_array[INA3221_NUM_CHAN];

	float temp[1];
	float sfp_temp_0[1];
	float sfp_txpwr_0[1];
	float sfp_rxpwr_0[1];
	float sfp_vcc_0[1];
	float sfp_txbias_0[1];
	uint8_t sfp_status_0[2];

	/*float sfp_temp_1[1];
	float sfp_txpwr_1[1];
	float sfp_rxpwr_1[1];
	float sfp_vcc_1[1];
	float sfp_txbias_1[1];
	uint8_t sfp_status_1[2];

	float sfp_temp_2[1];
	float sfp_txpwr_2[1];
	float sfp_rxpwr_2[1];
	float sfp_vcc_2[1];
	float sfp_txbias_2[1];
	uint8_t sfp_status_2[2];

	float sfp_temp_3[1];
	float sfp_txpwr_3[1];
	float sfp_rxpwr_3[1];
	float sfp_vcc_3[1];
	float sfp_txbias_3[1];
	uint8_t sfp_status_3[2];

	float sfp_temp_4[1];
	float sfp_txpwr_4[1];
	float sfp_rxpwr_4[1];
	float sfp_vcc_4[1];
	float sfp_txbias_4[1];
	uint8_t sfp_status_4[2];

	float sfp_temp_5[1];
	float sfp_txpwr_5[1];
	float sfp_rxpwr_5[1];
	float sfp_vcc_5[1];
	float sfp_txbias_5[1];
	uint8_t sfp_status_5[2];*/

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
		rc = sfp_avago_read_status(data,0,sfp_status_0);
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
		rc = sfp_avago_read_status(data,,sfp_status_1);
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
		rc = sfp_avago_read_status(data,2,sfp_status_2);
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
		rc = sfp_avago_read_status(data,3,sfp_status_3);
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
		rc = sfp_avago_read_status(data,4,sfp_status_4);
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
		}
		rc = sfp_avago_read_status(data,5,sfp_status_5);
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

		rc = xlnx_ams_read_temp(temp_chan,AMS_TEMP_NUM_CHAN,ams_temp);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = xlnx_ams_read_volt(volt_chan,AMS_VOLT_NUM_CHAN,ams_volt);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = eth_link_status("eth0",&eth_status[0]);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = eth_link_status("eth1",&eth_status[1]);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = read_GPIO(DIG0_MAIN_AURORA_LINK,&aurora_status[0]);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = read_GPIO(DIG0_BACKUP_AURORA_LINK,&aurora_status[1]);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = read_GPIO(DIG1_MAIN_AURORA_LINK,&aurora_status[2]);
		if (rc) {
			printf("Error2\r\n");
			return NULL;
		}
		rc = read_GPIO(DIG1_BACKUP_AURORA_LINK,&aurora_status[3]);
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

		parsing_mon_status_data_into_array(jdpb,eth_status[0],"Main Ethernet Link Status",99);
		parsing_mon_status_data_into_array(jdpb,eth_status[1],"Backup Ethernet Link Status",99);

		parsing_mon_status_data_into_array(jdig0,aurora_status[0],"Aurora Main Link Status",99);
		parsing_mon_status_data_into_array(jdig0,aurora_status[1],"Aurora Backup Link Status",99);

		parsing_mon_status_data_into_array(jdig1,aurora_status[2],"Aurora Main Link Status",99);
		parsing_mon_status_data_into_array(jdig1,aurora_status[3],"Aurora Backup Link Status",99);

		parsing_mon_sensor_data_into_array(jdpb,temp[0],"PCB Temperature",99);

		parsing_mon_sensor_data_into_array(jdpb,sfp_temp_0[0],"SFP Temperature",0);
		parsing_mon_sensor_data_into_array(jdpb,sfp_txbias_0[0],"SFP Laser Bias Current",0);
		parsing_mon_sensor_data_into_array(jdpb,sfp_txpwr_0[0],"SFP TX Power",0);
		parsing_mon_sensor_data_into_array(jdpb,sfp_rxpwr_0[0],"SFP RX Power",0);
		parsing_mon_status_data_into_array(jdpb,sfp_status_0[0],"SFP RX_LOS Status",0);
		parsing_mon_status_data_into_array(jdpb,sfp_status_0[1],"SFP TX_FAULT Status",0);

		/*parsing_mon_sensor_data_into_array(jdpb,sfp_temp_1[0],"SFP Temperature",1);
		parsing_mon_sensor_data_into_array(jdpb,sfp_txbias_1[0],"SFP Laser Bias Current",1);
		parsing_mon_sensor_data_into_array(jdpb,sfp_txpwr_1[0],"SFP TX Power",1);
		parsing_mon_sensor_data_into_array(jdpb,sfp_rxpwr_1[0],"SFP RX Power",1);
		parsing_mon_status_data_into_array(jdpb,sfp_status_1[0],"SFP RX_LOS Status",1);
		parsing_mon_status_data_into_array(jdpb,sfp_status_1[1],"SFP TX_FAULT Status",1);

		parsing_mon_sensor_data_into_array(jdpb,sfp_temp_2[0],"SFP Temperature",2);
		parsing_mon_sensor_data_into_array(jdpb,sfp_txbias_2[0],"SFP Laser Bias Current",2);
		parsing_mon_sensor_data_into_array(jdpb,sfp_txpwr_2[0],"SFP TX Power",2);
		parsing_mon_sensor_data_into_array(jdpb,sfp_rxpwr_2[0],"SFP RX Power",2);
		parsing_mon_status_data_into_array(jdpb,sfp_status_2[0],"SFP RX_LOS Status",2);
		parsing_mon_status_data_into_array(jdpb,sfp_status_2[1],"SFP TX_FAULT Status",2);

		parsing_mon_sensor_data_into_array(jdpb,sfp_temp_3[0],"SFP Temperature",3);
		parsing_mon_sensor_data_into_array(jdpb,sfp_txbias_3[0],"SFP Laser Bias Current",3);
		parsing_mon_sensor_data_into_array(jdpb,sfp_txpwr_3[0],"SFP TX Power",3);
		parsing_mon_sensor_data_into_array(jdpb,sfp_rxpwr_3[0],"SFP RX Power",3);
		parsing_mon_status_data_into_array(jdpb,sfp_status_3[0],"SFP RX_LOS Status",3);
		parsing_mon_status_data_into_array(jdpb,sfp_status_3[1],"SFP TX_FAULT Status",3);

		parsing_mon_sensor_data_into_array(jdpb,sfp_temp_4[0],"SFP Temperature",4);
		parsing_mon_sensor_data_into_array(jdpb,sfp_txbias_4[0],"SFP Laser Bias Current",4);
		parsing_mon_sensor_data_into_array(jdpb,sfp_txpwr_4[0],"SFP TX Power",4);
		parsing_mon_sensor_data_into_array(jdpb,sfp_rxpwr_4[0],"SFP RX Power",4);
		parsing_mon_status_data_into_array(jdpb,sfp_status_4[0],"SFP RX_LOS Status",4);
		parsing_mon_status_data_into_array(jdpb,sfp_status_4[1],"SFP TX_FAULT Status",4);

		parsing_mon_sensor_data_into_array(jdpb,sfp_temp_5[0],"SFP Temperature",5);
		parsing_mon_sensor_data_into_array(jdpb,sfp_txbias_5[0],"SFP Laser Bias Current",5);
		parsing_mon_sensor_data_into_array(jdpb,sfp_txpwr_5[0],"SFP TX Power",5);
		parsing_mon_sensor_data_into_array(jdpb,sfp_rxpwr_5[0],"SFP RX Power",5);
		parsing_mon_status_data_into_array(jdpb,sfp_status_5[0],"SFP RX_LOS Status",5);
		parsing_mon_status_data_into_array(jdpb,sfp_status_5[1],"SFP TX_FAULT Status",5);*/

		parsing_mon_sensor_data_into_array(jdpb,ams_temp[0],ams_channels[0],99);
		parsing_mon_sensor_data_into_array(jdpb,ams_temp[1],ams_channels[1],99);
		parsing_mon_sensor_data_into_array(jdpb,ams_temp[2],ams_channels[13],99);

		for(int n = 0; n<AMS_VOLT_NUM_CHAN;n++){
			if(n != 11){
				parsing_mon_sensor_data_into_array(jdpb,ams_volt[n],ams_channels[n+2],99);	}
		}

		for(int j=0;j<INA3221_NUM_CHAN;j++){
			pwr_array[j] = volt_sfp0_2[j]*curr_sfp0_2[j];
			parsing_mon_sensor_data_into_array(jdpb,volt_sfp0_2[j],"SFP Voltage Monitor",j);
			parsing_mon_sensor_data_into_array(jdpb,curr_sfp0_2[j],"SFP Current Monitor",j);
			parsing_mon_sensor_data_into_array(jdpb,pwr_array[j],"SFP Power Monitor",j);
		}
		for(int k=0;k<INA3221_NUM_CHAN;k++){
			pwr_array[k] = volt_sfp3_5[k]*curr_sfp3_5[k];
			parsing_mon_sensor_data_into_array(jdpb,volt_sfp3_5[k],"SFP Voltage Monitor",k+3);
			parsing_mon_sensor_data_into_array(jdpb,curr_sfp3_5[k],"SFP Current Monitor",k+3);
			parsing_mon_sensor_data_into_array(jdpb,pwr_array[k],"SFP Power Monitor",k+3);
		}
		for(int l=0;l<INA3221_NUM_CHAN;l++){
			switch(l){
			case 0:
				volt = "Voltage Monitor (+12V)";
				curr = "Current Monitor (+12V)";
				pwr = "Power Monitor (+12V)";
				break;
			case 1:
				volt = "Voltage Monitor (+3.3V)";
				curr = "Current Monitor (+3.3V)";
				pwr = "Power Monitor (+3.3V)";
				break;
			case 2:
				volt = "Voltage Monitor (+1.8V)";
				curr = "Current Monitor (+1.8V)";
				pwr = "Power Monitor (+1.8V)";
				break;
			default:
				volt = "Voltage Monitor (+12V)";
				curr = "Current Monitor (+12V)";
				pwr = "Power Monitor (+12V)";
			break;
			}
			parsing_mon_sensor_data_into_array(jdpb,volt_som[l],volt,99);
			parsing_mon_sensor_data_into_array(jdpb,curr_som[l],curr,99);
			parsing_mon_sensor_data_into_array(jdpb,curr_som[l]*volt_som[l],pwr,99);
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

		const char *serialized_json = json_object_to_json_string(jobj);
		zmq_send (mon_publisher, strdup(serialized_json), strlen(serialized_json), 0);
		//zstr_send_compress(mon_publisher,serialized_json);

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
 * It also deals with Ethernet Alarms.
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
		rc = eth_down_alarm("eth0",&eth0_flag);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = eth_down_alarm("eth1",&eth1_flag);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = aurora_down_alarm(0,&dig0_main_flag);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = aurora_down_alarm(1,&dig0_backup_flag);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = aurora_down_alarm(2,&dig1_main_flag);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = aurora_down_alarm(3,&dig1_backup_flag);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}

		sem_wait(&i2c_sync); //Semaphore to sync I2C usage

		rc = mcp9844_read_alarms(data);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_read_alarms(data,0);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_read_alarms(data,1);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = ina3221_read_alarms(data,2);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}

		/*rc = sfp_avago_read_alarms(data,0);
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}

		rc = sfp_avago_read_alarms(data,1)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(data,2)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(data,3)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(data,4)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		rc = sfp_avago_read_alarms(data,5)
		if (rc) {
			printf("Error\r\n");
			return NULL;
		}
		*/

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

        json_object *jobj = json_object_new_object();
        res[0] = 0;
        int rc = 0;

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
    		else if(chan >= 7){
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


    			rc = alarm_json(jobj,ams_channels[chan-7],ev_type, 99, res[0],timestamp,"warning");
    			//printf("Chip: AMS. Event type: %s. Timestamp: %lld. Channel type: %s. Channel: %d. Value: %f V\n",ev_type,timestamp,ch_type,chan,res[0]);
    			fclose(raw);
    			fclose(rising);
    		}

        }
        else if(!strcmp(ch_type,"temp") && chan >= 7){
        	xlnx_ams_read_temp(&chan,1,res);
        	rc = alarm_json(jobj,ams_channels[chan-7],ev_type, 99, res[0],timestamp,"warning");
            //printf("Chip: AMS. Event type: %s. Timestamp: %lld. Channel type: %s. Channel: %d. Value: %f ºC\n",ev_type,timestamp,ch_type,chan,res[0]);
        }
		wait_period(&info);
	}
	return NULL;
}

/**
 * Periodic thread that is waiting for a command from the DAQ and handling it
 *
 * @param void *arg: NULL
 *
 * @return  NULL (if exits is because of an error).
 */
static void *command_thread(void *arg){

	struct periodic_info info;
	int rc ;
	struct DPB_I2cSensors *data = arg;

	printf("Command thread period: %dms\n",COMMAND_THREAD_PERIOD);
	rc = make_periodic(COMMAND_THREAD_PERIOD, &info);
	if (rc) {
		printf("Error\r\n");
		return NULL;
	}
	while(1){
		char *cmd[6];
		char buffer [256];


		int size = zmq_recv(cmd_router, buffer, 255, 0);
		if (size == -1)
		  return NULL;
		if (size > 255)
		  size = 255;
		buffer [size] = '\0';
		strdup(buffer);

		cmd[0] = strtok(buffer," ");
		int i = 0;
		while( cmd[i] != NULL ) {
		   i++;
		   cmd[i] = strtok(NULL, " ");
		}

		json_object *jobj = json_object_new_object();
		json_object *jstr4;
		json_object *jstr5;
		char buff[8];

		switch(i){
		case 5:
			if(!strcmp(cmd[4],"ON") | !strcmp(cmd[4],"OFF")){
				jstr5 = json_object_new_string(cmd[4]);
			}
			else{
				float val = atof(cmd[4]);
				sprintf(buff, "%3.4f", val);
				jstr5 = json_object_new_double_s((double) val,buff);
			}
		case 4:
			jstr4 = json_object_new_string(cmd[3]);
		default:
			json_object *jstr3 = json_object_new_string(cmd[2]);
			json_object *jstr2 = json_object_new_string(cmd[1]);
			json_object *jstr1 = json_object_new_string(cmd[0]);
			json_object *jempty = json_object_new_string("");
			json_object_object_add(jobj,"opcode", jstr1);
			json_object_object_add(jobj,"board", jstr2);
			json_object_object_add(jobj,"magnitude", jstr3);
			if(i>=4){
				json_object_object_add(jobj,"optparam", jstr4);
			}
			else{
				json_object_object_add(jobj,"optparam", jempty);
			}
			if(i == 5){
				json_object_object_add(jobj,"value", jstr5);
			}
			else{
				json_object_object_add(jobj,"value", jempty);
			}
			break;
		}
		//Check JSON schema valid
		if(!strcmp(cmd[1],"HV")){
			//Command conversion
			//RS485 communication
		}
		else if(!strcmp(cmd[1],"LV")){
			//Command conversion
			//RS485 communication
		}
		else if(!strcmp(cmd[1],"Dig0")){
			//Command conversion
			//Serial Port Communication
		}
		else if(!strcmp(cmd[1],"Dig1")){
			//Command conversion
			//Serial Port Communication
		}
		else{ //DPB

		}

		wait_period(&info);
	}

	return NULL;
}

/************************** Main function ******************************/
int main(){

	//Threads elements
	pthread_t t_1, t_2, t_3, t_4;
	sigset_t alarm_sig;
	int i;

	int rc;
	struct DPB_I2cSensors data;

	get_GPIO_base_address(&GPIO_BASE_ADDRESS);

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
	rc = zmq_socket_init(); //Initialize ZMQ Sockets
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

	pthread_create(&t_1, NULL, ams_alarms_thread,NULL); //Create thread 1 - reads AMS alarms
	pthread_create(&t_2, NULL, i2c_alarms_thread,(void *)&data); //Create thread 2 - reads I2C alarms every x miliseconds
	pthread_create(&t_3, NULL, monitoring_thread,(void *)&data);//Create thread 3 - monitors magnitudes every x seconds
	//pthread_create(&t_4, NULL, command_thread,(void *)&data);//Create thread 4 - waits and attends commands


	while(1){
		sleep(1000000);
		}
	//stop_I2cSensors(&data);
	return 0;
}
