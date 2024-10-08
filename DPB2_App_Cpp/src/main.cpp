/*
 *
 * @date   12-04-2024
 * @author Borja Martínez Sánchez , Alejandro Gómez Gambín
 */

/************************** Libraries includes *****************************/
// COPacket includes
#include <common/protocols/COPacket/COPacket.hpp>
#include <COPacketCmdHkDig.h>

extern "C"
{
#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <dirent.h>
#include <signal.h>
#include <regex.h>
#include "linux/errno.h"
#include <execinfo.h>


#include <dpb2sc.h>

/******************************************************************************
*Local Semaphores.
****************************************************************************/
/** @defgroup semaph Local Semaphores
 *  Semaphores needed to synchronize the application execution and avoid race conditions
 *  @{
 */

/** @brief Semaphore to synchronize thread creation */
sem_t thread_sync;
/** @} */
/******************************************************************************
*Child process and threads.
****************************************************************************/
/** @defgroup pth Child process and threads
 *  Threads and subprocesses declaration
 *  @{
 */
/** @brief IIO Event Monitor Process ID */
pid_t child_pid;
/** @brief AMS alarm Thread */
pthread_t t_1;
/** @brief I2C alarm Thread */
pthread_t t_2;
/** @brief Monitoring Thread */
pthread_t t_3;
/** @brief Command Handling Thread */
pthread_t t_4;
/** @brief periods for each of the threads in order (1 = AMS alarms 2= Other alarms 3= Monitoring 4 = Command handling) */
int periods[4];

/** @} */

int break_flag = 0;
struct DPB_I2cSensors data;
extern _COPacketCmdList HkDigCmdList;
/******************************************************************************
*Threads timers (ms).
****************************************************************************/
#define MONIT_THREAD_PERIOD_DEFAULT 5000000
#define ALARMS_THREAD_PERIOD_DEFAULT 100000
#define AMS_ALARMS_THREAD_PERIOD_DEFAULT 100000
#define COMMAND_THREAD_PERIOD_DEFAULT 50000

/************************** Function Prototypes ******************************/


int iio_event_monitor_up();
void sighandler(int );
void segmentation_handler(int );
static void *monitoring_thread(void *);
static void *i2c_alarms_thread(void *);
static void *ams_alarms_thread(void *);
static void *command_thread(void *);

/************************** IIO_EVENT_MONITOR Functions ******************************/
/** @defgroup add Additional functions for the application besides libdpb2sc
 *  Additional functions declaration
 *  @{
 */
/**
 * Start IIO EVENT MONITOR to enable Xilinx-AMS events
 *
 *
 * @return Negative integer if start fails.If not, returns 0 and enables Xilinx-AMS events.
 */
int iio_event_monitor_up() {

	int rc = 0;
	char path[64];
	FILE *temp_file;
	char str[64];
	regex_t r1;
	int data = regcomp(&r1, "[:IIO_MONITOR:]", 0);

	char cmd[64];
	strcpy(cmd,"which IIO_MONITOR >> /home/petalinux/path_temp.txt");

	rc = system(cmd);
	temp_file = fopen("/home/petalinux/path_temp.txt","r");
	if((rc == -1) | (temp_file == NULL)){
		return -EINVAL;
	}
	fread(path, 64, 1, temp_file);
	fclose(temp_file);

	strcpy(str,strtok(path,"\n"));
	strcat(str,"");
	data = regexec(&r1, str, 0, NULL, 0);
	if(data){
		remove("/home/petalinux/path_temp.txt");
		regfree(&r1);
		return -EINVAL;
	}
	else{
		remove("/home/petalinux/path_temp.txt");
		regfree(&r1);
	}

    child_pid = fork(); // Create a child process

    if (child_pid == 0) {
        // Child process
        // Path of the .elf file and arguments
        char *args[] = {str, "-a", "/dev/iio:device0", NULL};

        // Execute the .elf file
        if (execvp(args[0], args) == -1) {
            perror("Error executing the .elf file");
            return -1;
        }
    } else if (child_pid > 0) {
        // Parent process
    } else {
        // Error creating the child process
        perror("Error creating the child process");
        return -1;
    }
    return 0;
}
/** @} */

/************************** Signal Handling function declaration ******************************/
/** @defgroup  sighandler Signal Handlers implementation
 *  All the dpb slow control application signal handlers for different termination and error signals
 *  @{
 */
/**
 * Handles termination signals, kills every subprocess
 *
 * @param signum Signal ID
 *
 * @return void
 */
void sighandler(int signum) {
   kill(child_pid,SIGKILL);
   //End threads
   pthread_cancel(t_1);
   pthread_cancel(t_2);
   pthread_cancel(t_4);
   pthread_cancel(t_3);

   pthread_join(t_1,NULL);
   pthread_join(t_2,NULL);
   pthread_join(t_3,NULL);
   pthread_join(t_4,NULL);

   dpbsc_lib_close(&data);
   break_flag = 1;
   return ;
}

/**
 * Handles segmentation fault signals, prints backtrace
 *
 * @param sig Signal ID, should be -11 (SEGV)
 *
 * @return this function should not return,it quits the program executing exit
 */
void segmentation_handler(int sig) {
	void *array[10];
	  size_t size;

	  // get void*'s for all entries on the stack
	  size = backtrace(array, 10);
	  // print out all the frames to stderr
	  fprintf(stderr, "Error: signal %d:\n", sig);
	  backtrace_symbols_fd(array, size, STDERR_FILENO);
	  exit(1);
}
/** @} */
/************************** Threads declaration ******************************/
/** @defgroup threads Threads implementation
 *  All the dpb slow control application threads implementation
 *  @{
 */
/**
 * Periodic thread that every x seconds reads every magnitude of every sensor available and stores it.
 *
 * @param arg must contain a struct with every I2C device that wants to be monitored
 *
 * @return NULL (if exits is because of an error).
 */
static void *monitoring_thread(void *arg)
{
	struct periodic_info info;
	int rc ;
	struct DPB_I2cSensors *data = static_cast<DPB_I2cSensors *>(arg);

	int eth_status[2];
	int aurora_status[4];

	char curr[32] = "12Vcurrent";
	char volt[32] = "12Vvoltage";
	char pwr[32] = "12Vpwr";
	int rc2;

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

	float temp[SFP_NUM];
	float sfp_temp[SFP_NUM];
	float sfp_txpwr[SFP_NUM];
	float sfp_rxpwr[SFP_NUM];
	float sfp_vcc[SFP_NUM];
	float sfp_txbias[SFP_NUM];
	uint8_t sfp_status[2][SFP_NUM];

	printf("Monitoring thread period: %3.4fs\n",((float)periods[2])/1000000);
	rc = make_periodic(periods[2], &info);
	if (rc) {
		printf("Error\r\n");
		return NULL;
	}
	sem_post(&thread_sync);
	while (1) {
		// DPB Slow Control Monitoring
		sem_wait(&i2c_sync); //Semaphore to sync I2C usage
		rc = mcp9844_read_temperature(data,temp);
		if (rc) {
			printf("Reading Error\r\n");
		}
		// SFP monitoring
		for(int i = 0; i < SFP_NUM;i++){
				if(sfp_connected[i]){
				rc = sfp_avago_read_temperature(data,i,&sfp_temp[i]);
				if (rc) {
					printf("Reading Error\r\n");
				}
				rc = sfp_avago_read_voltage(data,i,&sfp_vcc[i]);
				if (rc) {
					printf("Reading Error\r\n");
				}
				rc = sfp_avago_read_lbias_current(data,i,&sfp_txbias[i]);
				if (rc) {
					printf("Reading Error\r\n");
				}
				rc = sfp_avago_read_tx_av_optical_pwr(data,i,&sfp_txpwr[i]);
				if (rc) {
					printf("Reading Error\r\n");
				}
				rc = sfp_avago_read_rx_av_optical_pwr(data,i,&sfp_rxpwr[i]);
				if (rc) {
					printf("Reading Error\r\n");
				}
				rc = sfp_avago_read_status(data,i,sfp_status[i]);
				if (rc) {
					printf("Reading Error\r\n");
				
				}
			}
		}
		rc = ina3221_get_voltage(data,0,volt_sfp0_2);
		if (rc) {
			printf("Reading Error\r\n");
		}
		rc = ina3221_get_voltage(data,1,volt_sfp3_5);
		if (rc) {
			printf("Reading Error\r\n");
		}
		rc = ina3221_get_voltage(data,2,volt_som);
		if (rc) {
			printf("Reading Error\r\n");
		}
		rc = ina3221_get_current(data,0,curr_sfp0_2);
		if (rc) {
			printf("Reading Error\r\n");
		}
		rc = ina3221_get_current(data,1,curr_sfp3_5);
		if (rc) {
			printf("Reading Error\r\n");
		}
		rc = ina3221_get_current(data,2,curr_som);
		if (rc) {
			printf("Reading Error\r\n");
		}
		sem_post(&i2c_sync);//Free semaphore to sync I2C usage

		rc = xlnx_ams_read_temp(temp_chan,AMS_TEMP_NUM_CHAN,ams_temp);
		if (rc) {
			printf("Reading Error\r\n");
		}
		rc = xlnx_ams_read_volt(volt_chan,AMS_VOLT_NUM_CHAN,ams_volt);
		if (rc) {
			printf("Reading Error\r\n");
		}
		rc = eth_link_status("eth0",&eth_status[0]);
		if (rc) {
			printf("Reading Error\r\n");
		}
		rc = eth_link_status("eth1",&eth_status[1]);
		if (rc) {
			printf("Reading Error\r\n");
		}
		rc = read_GPIO(DIG0_MAIN_AURORA_LINK,&aurora_status[0]);
		if (rc) {
			printf("Reading Error\r\n");
		}
		rc = read_GPIO(DIG0_BACKUP_AURORA_LINK,&aurora_status[1]);
		if (rc) {
			printf("Reading Error\r\n");
		}
		rc = read_GPIO(DIG1_MAIN_AURORA_LINK,&aurora_status[2]);
		if (rc) {
			printf("Reading Error\r\n");
		}
		rc = read_GPIO(DIG1_BACKUP_AURORA_LINK,&aurora_status[3]);
		if (rc) {
			printf("Reading Error\r\n");
		}
		//json_object * jobj = json_object_new_object();
		json_object *jdata = json_object_new_object();
		json_object *jlv = json_object_new_object();
		json_object *jhv = json_object_new_object();
		json_object *jdig0 = json_object_new_object();
		json_object *jdig1 = json_object_new_object();
		json_object *jdpb = json_object_new_object();

		json_object *jsfps = json_object_new_array();
		parsing_mon_environment_status_into_object(jdpb, "ethmain", eth_status[0]);
		parsing_mon_environment_status_into_object(jdpb, "ethbackup", eth_status[1]);

		parsing_mon_environment_status_into_object(jdig0, "auroramain", aurora_status[0]);
		parsing_mon_environment_status_into_object(jdig0, "aurorabackup", aurora_status[1]);

		parsing_mon_environment_status_into_object(jdig1, "auroramain", aurora_status[2]);
		parsing_mon_environment_status_into_object(jdig1, "aurorabackup", aurora_status[3]);

		parsing_mon_environment_data_into_object(jdpb,"boardtemp", temp[0]);

		//Include SFP data in JSON object if they are connected
		for(int i = 0; i < SFP_NUM; i++){
			if(sfp_connected[i]){
			parsing_mon_channel_data_into_object(jsfps,i,"temperature",sfp_temp[i]);
			parsing_mon_channel_data_into_object(jsfps,i,"biascurr",sfp_txbias[i]);
			parsing_mon_channel_data_into_object(jsfps,i,"txpwr",sfp_txpwr[i]);
			parsing_mon_channel_data_into_object(jsfps,i,"rxpwr",sfp_rxpwr[i]);

			parsing_mon_channel_status_into_object(jsfps,0,"rxlos",sfp_status[0][i]);
			parsing_mon_channel_status_into_object(jsfps,0,"txfault",sfp_status[1][i]);
			}
		}
		parsing_mon_environment_data_into_object(jdpb,"lpdcputemp", ams_temp[0]);
		parsing_mon_environment_data_into_object(jdpb,"fpdcputemp", ams_temp[1]);
		parsing_mon_environment_data_into_object(jdpb,"fpgatemp", ams_temp[2]);

		/*for(int n = 0; n<AMS_VOLT_NUM_CHAN;n++){
			if(n != 11){
				parsing_mon_environment_data_into_object(jdpb,ams_channels[n+2],ams_volt[n]);	}
		}*/
		for(int j=0;j<INA3221_NUM_CHAN;j++){
			pwr_array[j] = volt_sfp0_2[j]*curr_sfp0_2[j];
			parsing_mon_channel_data_into_object(jsfps,j,"voltage",volt_sfp0_2[j]);
			parsing_mon_channel_data_into_object(jsfps,j,"current",curr_sfp0_2[j]);
			parsing_mon_channel_data_into_object(jsfps,j,"pwr",pwr_array[j]);
		}
		for(int k=0;k<INA3221_NUM_CHAN;k++){
			pwr_array[k] = volt_sfp3_5[k]*curr_sfp3_5[k];
			parsing_mon_channel_data_into_object(jsfps,k + 3,"voltage",volt_sfp3_5[k]);
			parsing_mon_channel_data_into_object(jsfps,k + 3,"current",curr_sfp3_5[k]);
			parsing_mon_channel_data_into_object(jsfps,k + 3,"pwr",pwr_array[k]);
		}

		for(int l=0;l<INA3221_NUM_CHAN;l++){
			switch(l){
			case 0:
				strcpy(volt , "12Vvoltage");
				strcpy(curr , "12Vcurrent");
				strcpy(pwr , "12Vpwr");
				break;
			case 1:
				strcpy(volt , "3V3voltage");
				strcpy(curr , "3V3current");
				strcpy(pwr , "3V3pwr");
				break;
			case 2:
				strcpy(volt , "1V8voltage");
				strcpy(curr , "1V8current");
				strcpy(pwr , "1V8pwr");
				break;
			default:
				strcpy(volt , "12Vvoltage");
				strcpy(curr , "12Vcurrent");
				strcpy(pwr , "12Vpwr");
			break;
			}
			parsing_mon_environment_data_into_object(jdpb,volt, volt_som[l]);
			parsing_mon_environment_data_into_object(jdpb,curr, curr_som[l]);
			parsing_mon_environment_data_into_object(jdpb,pwr, curr_som[l]*volt_som[l]);
		}

		//Digitizer 0 Slow Control Monitoring
		CCOPacket pkt(COPKT_DEFAULT_START, COPKT_DEFAULT_STOP, COPKT_DEFAULT_SEP);
		COPacketResponse_type	pktError=COPACKET_NOERR;
		char digcmd[32];
		char dig_response[32];
		char *dig_mag_str;
		float dig_value;
		if(dig0_connected){
			// Board parameters
			for(int i = 0; i < DIG_MON_BOARD_CODES_SIZE; i++){
				pkt.CreatePacket(digcmd, HkDigCmdList.CmdList[dig_monitor_mag_board_codes[i]].CmdString);
				dig_command_handling(DIGITIZER_0,digcmd,dig_response);
				pktError = pkt.LoadString(dig_response);
				int16_t cmdIdx = pkt.GetNextFiedlAsCOMMAND(HkDigCmdList);
				switch(cmdIdx){
					//String
					case HKDIG_GET_GW_VER:
    				case HKDIG_GET_SW_VER:
    				case HKDIG_GET_BOARD_STATUS:
    				case HKDIG_GET_BOARD_CNTRL:
					case HKDIG_GET_UPTIME:
					case HKDIG_GET_RMON_T:
					case HKDIG_GET_TLNK_LOCK:
					case HKDIG_GET_EEPROM_OUI:			// Returns EEPROM OUI code
					case HKDIG_GET_EEPROM_EID:
					// BME280 commands
					case HKDIG_GET_BME_TCAL:
					case HKDIG_GET_BME_HCAL:				
					case HKDIG_GET_BME_PCAL:
						dig_mag_str = pkt.GetNextField();
						parsing_mon_environment_string_into_object(jdig0, dig_monitor_mag_board_names[i],dig_mag_str);
						break;
					//Float
					case HKDIG_GET_BOARD_3V3A:
					case HKDIG_GET_BOARD_12VA:
					case HKDIG_GET_BOARD_I12V:
					case HKDIG_GET_BOARD_5V0A:
					case HKDIG_GET_BOARD_5V0F:
					case HKDIG_GET_BOARD_C12V:
					case HKDIG_GET_BOARD_I5VF:
					case HKDIG_GET_BOARD_I3V3A:
					case HKDIG_GET_BOARD_I12VA:
					case HKDIG_GET_BOARD_TU40:
					case HKDIG_GET_BOARD_TU41:
					case HKDIG_GET_BOARD_TU45:
						pkt.GetNextFieldAsFLOAT(dig_value);
						parsing_mon_environment_data_into_object(jdig0, dig_monitor_mag_board_names[i],dig_value);
						break;				
					//Clock
					case HKDIG_GET_CLOCK:
						if(!strcmp(dig_mag_str,"0")){
							strcpy(dig_mag_str,"Local");
						}
						else{
							strcpy(dig_mag_str,"DPB");
						}
						parsing_mon_environment_string_into_object(jdig0, dig_monitor_mag_board_names[i],dig_mag_str);
						break;
					//Error
					case HKDIG_ERRO:
					default:
						strcpy(dig_mag_str, "ERROR");
						parsing_mon_environment_string_into_object(jdig0, dig_monitor_mag_board_names[i],dig_mag_str);
						break;
				}
			}

			// Channel parameters
			json_object *jdig0channels = json_object_new_array();
			for(int i = 0; i < DIG_MON_CHAN_CODES_SIZE; i++){
					for(int j = 0; j < 12; j++){
					pkt.CreatePacket(digcmd, HkDigCmdList.CmdList[dig_monitor_mag_chan_codes[i]].CmdString,(uint32_t) j);
					dig_command_handling(DIGITIZER_0,digcmd,dig_response);
					pktError = pkt.LoadString(dig_response);
					int16_t cmdIdx = pkt.GetNextFiedlAsCOMMAND(HkDigCmdList);

					switch(cmdIdx){

						//Float
						case HKDIG_GET_THR_NUM:
						case HKDIG_GET_IT_NUM:
						case HKDIG_GET_DT_NUM:
							pkt.GetNextFieldAsFLOAT(dig_value);
							pkt.GetNextFieldAsFLOAT(dig_value);
							parsing_mon_channel_data_into_object(jdig0channels,j, dig_monitor_mag_chan_names[i],dig_value);
							break;

						//String 
						case HKDIG_GET_CHN_STATUS:
						case HKDIG_GET_CHN_CNTRL:
						case HKDIG_GET_PED_TYPE:
							dig_mag_str = pkt.GetNextField();
							dig_mag_str = pkt.GetNextField();
							parsing_mon_channel_string_into_object(jdig0channels,j, dig_monitor_mag_chan_names[i],dig_mag_str);
							break;
						//Error
						case HKDIG_ERRO:
						default:
							strcpy(dig_mag_str, "ERROR");
							parsing_mon_channel_string_into_object(jdig0channels,j, dig_monitor_mag_chan_names[i],dig_mag_str);
							break;
					}
				}
			}
			json_object_object_add(jdig0,"channels",jdig0channels);
		}
		//Digitizer 1 Slow Control Monitoring
		if(dig1_connected){
			// Board parameters
			for(int i = 0; i < DIG_MON_BOARD_CODES_SIZE; i++){
				pkt.CreatePacket(digcmd, HkDigCmdList.CmdList[dig_monitor_mag_board_codes[i]].CmdString);
				dig_command_handling(DIGITIZER_1,digcmd,dig_response);
				pktError = pkt.LoadString(dig_response);
				int16_t cmdIdx = pkt.GetNextFiedlAsCOMMAND(HkDigCmdList);
				switch(cmdIdx){
					//String
					case HKDIG_GET_GW_VER:
    				case HKDIG_GET_SW_VER:
    				case HKDIG_GET_BOARD_STATUS:
    				case HKDIG_GET_BOARD_CNTRL:
					case HKDIG_GET_UPTIME:
					case HKDIG_GET_RMON_T:
					case HKDIG_GET_TLNK_LOCK:
					// BME280 commands
					case HKDIG_GET_BME_TCAL:
					case HKDIG_GET_BME_HCAL:				
					case HKDIG_GET_BME_PCAL:
						dig_mag_str = pkt.GetNextField();
						parsing_mon_environment_string_into_object(jdig1, dig_monitor_mag_board_names[i],dig_mag_str);
						break;
					//Float
					case HKDIG_GET_BOARD_3V3A:
					case HKDIG_GET_BOARD_12VA:
					case HKDIG_GET_BOARD_I12V:
					case HKDIG_GET_BOARD_5V0A:
					case HKDIG_GET_BOARD_5V0F:
					case HKDIG_GET_BOARD_C12V:
					case HKDIG_GET_BOARD_I5VF:
					case HKDIG_GET_BOARD_I3V3A:
					case HKDIG_GET_BOARD_I12VA:
					case HKDIG_GET_BOARD_TU40:
					case HKDIG_GET_BOARD_TU41:
					case HKDIG_GET_BOARD_TU45:
						pkt.GetNextFieldAsFLOAT(dig_value);
						parsing_mon_environment_data_into_object(jdig1, dig_monitor_mag_board_names[i],dig_value);
						break;				
					//Clock
					case HKDIG_GET_CLOCK:
						if(!strcmp(dig_mag_str,"0")){
							strcpy(dig_mag_str,"Local");
						}
						else{
							strcpy(dig_mag_str,"DPB");
						}
						parsing_mon_environment_string_into_object(jdig1, dig_monitor_mag_board_names[i],dig_mag_str);
						break;
					//Error
					case HKDIG_ERRO:
					default:
						strcpy(dig_mag_str, "ERROR");
						parsing_mon_environment_string_into_object(jdig1, dig_monitor_mag_board_names[i],dig_mag_str);
						break;
				}
			}

			// Channel parameters
			json_object *jdig1channels = json_object_new_array();
			for(int i = 0; i < DIG_MON_CHAN_CODES_SIZE; i++){
					for(int j = 0; j < 12; j++){
					pkt.CreatePacket(digcmd, HkDigCmdList.CmdList[dig_monitor_mag_chan_codes[i]].CmdString,(uint32_t) j);
					dig_command_handling(DIGITIZER_1,digcmd,dig_response);
					pktError = pkt.LoadString(dig_response);
					int16_t cmdIdx = pkt.GetNextFiedlAsCOMMAND(HkDigCmdList);

					switch(cmdIdx){

						//Float
						case HKDIG_GET_THR_NUM:
						case HKDIG_GET_IT_NUM:
						case HKDIG_GET_DT_NUM:
							pkt.GetNextFieldAsFLOAT(dig_value);
							pkt.GetNextFieldAsFLOAT(dig_value);
							parsing_mon_channel_data_into_object(jdig1channels,j, dig_monitor_mag_chan_names[i],dig_value);
							break;

						//String 
						case HKDIG_GET_CHN_STATUS:
						case HKDIG_GET_CHN_CNTRL:
						case HKDIG_GET_PED_TYPE:
							dig_mag_str = pkt.GetNextField();
							dig_mag_str = pkt.GetNextField();
							parsing_mon_channel_string_into_object(jdig1channels,j, dig_monitor_mag_chan_names[i],dig_mag_str);
							break;
						//Error
						case HKDIG_ERRO:
						default:
							strcpy(dig_mag_str, "ERROR");
							parsing_mon_channel_string_into_object(jdig1channels,j, dig_monitor_mag_chan_names[i],dig_mag_str);
							break;
					}
				}
			}
			json_object_object_add(jdig1,"channels",jdig1channels);
		}

		//LV Slow Control Monitoring
		char lv_mon_root[80];
		char lv_mon_cmd[80];
		char response[80];
		char channel_str[4];
		char board_dev[32];
		char mag_str[32];
		float mag_value;

		if(lv_connected){
			json_object *jlvchannels = json_object_new_array();
			strcpy(lv_mon_root,"$BD:0,$CMD:MON,PAR:");
			strcpy(board_dev,"/dev/ttyUL4");

			//Send Serial number
			parsing_mon_environment_string_into_object(jlv, lv_mag_names[0],LV_SN);

			//Read Environment Parameters
			for(int i = 1 ; i < 6; i++){
				strcpy(lv_mon_cmd,lv_mon_root);
				strcat(lv_mon_cmd,lv_board_words[i]);
				strcat(lv_mon_cmd,"\r\n");
				hv_lv_command_handling(board_dev,lv_mon_cmd,response);
				// Strip the returned value from response string
				char *target = NULL;
				char *start, *end;
				if ( (start = strstr( response, "#CMD:OK,VAL:" )) ){
					start += strlen( "#CMD:OK,VAL:" );
					if ( (end = strstr( start, "\r\n" )) )
					{
						target = ( char * )malloc( end - start + 1 );
						if(target){
							memcpy( target, start, end - start );
							target[end - start] = '\0';
							strcpy(mag_str,target);
						}
						else{
							strcpy(mag_str,"ERROR");
						}
						free(target);
					}
					else {
						strcpy(mag_str,"ERROR");
					}
				}
				else {
					strcpy(mag_str,"ERROR");
				}
				switch(i){
					case 1: // Temperature
					case 2: // BCM Temperature
					case 3: // Relative Humidity
					case 4: // Pressure
						mag_value=(float) atoi(mag_str);
						parsing_mon_environment_data_into_object(jlv,lv_mag_names[i], mag_value);
						break;
					case 5: // Water Leak
						if(!strcmp(mag_str,"YES"))
							mag_value = 1;
						else
							mag_value = 0;
						parsing_mon_environment_status_into_object(jlv,lv_mag_names[i], mag_value);
						break;
					default:
						break;
				}
			}

			//Read Channel Parameters
			strcpy(lv_mon_root,"$BD:0,$CMD:MON,CH:");
			for(int i = 0; i <= 7; i++){
			//Status Voltage and Current
				for(int j = 6; j < (LV_CMD_TABLE_SIZE-1); j++){  // We can't read CPU status
					strcpy(lv_mon_cmd,lv_mon_root);
					sprintf(channel_str,"%d",i);
					strcat(lv_mon_cmd,channel_str);
					strcat(lv_mon_cmd,",PAR:");
					// Exception, enable for channels 0 and 1 are bus converters
					if(i <= 1 && j == 5){
						strcat(lv_mon_cmd,"BCEN");
					} // Enable for channels 2 to 7 are stepdowns
					else if(i > 1 && j == 5){
						strcat(lv_mon_cmd,"SDEN");
					}
					else{
						strcat(lv_mon_cmd,lv_board_words[j]);
					}
					strcat(lv_mon_cmd,"\r\n");
					hv_lv_command_handling(board_dev,lv_mon_cmd,response);
					// Strip the returned value from response string
					char *target = NULL;
					char *start, *end;
					if ( (start = strstr( response, "#CMD:OK,VAL:" )) ){
						start += strlen( "#CMD:OK,VAL:" );
						if (( end = strstr( start, "\r\n" )) )
						{
							target = ( char * )malloc( end - start + 1 );
							memcpy( target, start, end - start );
							target[end - start] = '\0';
							if(target)
								strcpy(mag_str,target);
							free(target);
						}
						else {
							strcpy(mag_str,"ERROR");
						}
					}
					else {
						strcpy(mag_str,"ERROR");
					}
					switch (j-6){
						case 0: //Output Status
						if(!strcmp(mag_str,"ON"))
							mag_value = 1;
						else
							mag_value= 0;
						parsing_mon_channel_status_into_object(jlvchannels,i,lv_mag_names[j],mag_value);
						break;
						case 1: //Voltage Monitor
						case 2: //Current Monitor
						mag_value=atof(mag_str);
						parsing_mon_channel_data_into_object(jlvchannels,i,lv_mag_names[j],mag_value);
						default:
							break;
					}
				}
			}
			json_object_object_add(jlv,"channels",jlvchannels);
		}

		// HV Slow Control Monitoring
		char hv_mon_root[80];
		char hv_mon_cmd[80];
		int mag_status;
		if(hv_connected){

			json_object *jhvchannels = json_object_new_array();
			strcpy(board_dev,"/dev/ttyUL3");

			//Read Serial Number
			parsing_mon_environment_string_into_object(jhv,hv_mag_names[0], HV_SN);

			//Read Board Temperature
			strcpy(hv_mon_cmd,"$BD:1,$CMD:MON,PAR:BDTEMP\r\n");
			hv_lv_command_handling(board_dev,hv_mon_cmd,response);

			// Strip the returned value from response string
			char *target = NULL;
			char *start, *end;
			if ( (start = strstr( response, "#CMD:OK,VAL:+" )) ){
				start += strlen( "#CMD:OK,VAL:" );
				if ( (end = strstr( start, "\r\n" )) )
				{
					target = ( char * )malloc( end - start + 1 );
					if(target){
						memcpy( target, start, end - start );
						target[end - start] = '\0';
						strcpy(mag_str,target);
					}
					else{
						strcpy(mag_str,"ERROR");
					}
					free(target);
				}
				else {
					strcpy(mag_str,"ERROR");
				}
			}
			else {
				strcpy(mag_str,"ERROR");
			}
			mag_value = atof(mag_str);
			parsing_mon_environment_data_into_object(jhv,hv_mag_names[1], mag_value);

			//Read Channel Parameters
			strcpy(hv_mon_root,"$BD:1,$CMD:MON,CH:");
			for(int i = 0; i < 24; i++){
				for(int j = 2; j < HV_CMD_TABLE_SIZE; j++){
					strcpy(hv_mon_cmd,hv_mon_root);
					sprintf(channel_str,"%d",i);
					strcat(hv_mon_cmd,channel_str);
					strcat(hv_mon_cmd,",PAR:");
					strcat(hv_mon_cmd,hv_board_words[j]);
					strcat(hv_mon_cmd,"\r\n");
					hv_lv_command_handling(board_dev,hv_mon_cmd,response);

					// Strip the returned value from response string
					char *target = NULL;
					char *start, *end;
					if ( (start = strstr( response, "#CMD:OK,VAL:" ) )){
						start += strlen( "#CMD:OK,VAL:" );
						if ( (end = strstr( start, "\r\n" )) )
						{
							target = ( char * )malloc( end - start + 1 );
							if(target){
								memcpy( target, start, end - start );
								target[end - start] = '\0';
								strcpy(mag_str,target);
							}
							else{
								strcpy(mag_str,"ERROR");
							}
							free(target);

						}
						else {
							strcpy(mag_str,"ERROR");
						}
					}
					else {
						strcpy(mag_str,"ERROR");
					}

					switch(j-2) {
						case 0:
						// If it is status, we strip the least significant bit from the string
						mag_status = atoi(mag_str) & 0x1;
						parsing_mon_channel_status_into_object(jhvchannels,i,hv_mag_names[j],mag_status);
						break;
						case 1:  //Voltage Monitor
						case 2:	 //Current Monitor
						case 3:  // Temperature
						case 4:  //Rampup Speed
						case 5:  // Rampdown Speed
						case 6: // Trip Time
						mag_value = atof(mag_str);
						parsing_mon_channel_data_into_object(jhvchannels,i,hv_mag_names[j],mag_value);
						break;
						case 7:
						// If it is the channel error, we strip the most significant bit
						mag_status = (atoi(mag_str) & (0x1 << 13)) >> 13;
						parsing_mon_channel_status_into_object(jhvchannels,i,hv_mag_names[j],mag_status);
						break;
						case 8:
						case 9:
						if(inList(i,hv_sd_channels,8)){
							mag_value = atof(mag_str);
							parsing_mon_channel_data_into_object(jhvchannels,i,hv_mag_names[j],mag_value);
						}
						break;
						default:
						break;
					}
				}
			}
			json_object_object_add(jhv,"channels",jhvchannels);
		}

		json_object_object_add(jdpb,"SFPs",jsfps);

		json_object_object_add(jdata,"LV", jlv);
		json_object_object_add(jdata,"HV", jhv);
		json_object_object_add(jdata,"Dig0", jdig0);
		json_object_object_add(jdata,"Dig1", jdig1);
		json_object_object_add(jdata,"DPB", jdpb);

		const char *serialized_json = json_object_to_json_string(jdata);

		//rc = json_schema_validate("JSONSchemaMonitoring.json",serialized_json, "mon_temp.json");
		//if (rc) {
		//	printf("Error validating JSON Schema\r\n");
		//}
		//else{ //FIXME DAQ Function HERE. Use the send monitoring data function of DAQ library
			rc2 = zmq_send(mon_publisher, serialized_json, strlen(serialized_json), 0);
		//	if (rc2 < 0) {
		//		printf("Error sending JSON\r\n");
		//	}
		//}
		json_object_put(jdata);
		wait_period(&info);
	}
	//stop_I2cSensors(&data);//
	return NULL;
}
/**
 * Periodic thread that every x seconds reads every alarm of every I2C sensor available and handles the interruption.
 * It also deals with Ethernet and GPIO Alarms.
 *
 * @param arg must contain a struct with every I2C device that wants to be monitored
 *
 * @return  NULL (if exits is because of an error).
 */
static void *i2c_alarms_thread(void *arg){
	struct periodic_info info;
	int rc ;
	struct DPB_I2cSensors *data = static_cast<DPB_I2cSensors *>(arg);

	printf("Alarms thread period: %3.4fms\n",((float)periods[1])/1000);
	rc = make_periodic(periods[1], &info);
	if (rc) {
		printf("Error\r\n");
		return NULL;
	}
	int hv_alarms_period = 700000/periods[1]; //in us
	int hv_count = 0;
	sem_post(&thread_sync);
	while(1){
		rc = eth_down_alarm("eth0",&eth0_flag);
		if (rc) {
			printf("Error reading alarm\r\n");
		}
		rc = eth_down_alarm("eth1",&eth1_flag);
		if (rc) {
			printf("Error reading alarm\r\n");
		}
		rc = aurora_down_alarm(0,&dig0_main_flag);
		if (rc) {
			printf("Error reading alarm\r\n");
		}
		rc = aurora_down_alarm(1,&dig0_backup_flag);
		if (rc) {
			printf("Error reading alarm\r\n");
		}
		rc = aurora_down_alarm(2,&dig1_main_flag);
		if (rc) {
			printf("Error reading alarm\r\n");
		}
		rc = aurora_down_alarm(3,&dig1_backup_flag);
		if (rc) {
			printf("Error reading alarm\r\n");
		}
		sem_wait(&i2c_sync); //Semaphore to sync I2C usage

		rc = mcp9844_read_alarms(data);
		if (rc) {
			printf("Error reading alarm\r\n");
		}
		rc = ina3221_read_alarms(data,0);
		if (rc) {
			printf("Error reading alarm\r\n");
		}
		rc = ina3221_read_alarms(data,1);
		if (rc) {
			printf("Error reading alarm\r\n");
		}
		rc = ina3221_read_alarms(data,2);
		if (rc) {
			printf("Error reading alarm\r\n");
		}
		for (int i = 0; i < SFP_NUM; i++){
			if(sfp_connected[i]){
			rc = sfp_avago_read_alarms(data,i);
				if (rc) {
					printf("Error reading alarm\r\n");
				}
			}
		}
		sem_post(&i2c_sync); //Free semaphore to sync I2C usage

		//HV alarm parsing only each certain period multiple of alarm thread period
		hv_count++;
		if(hv_connected && hv_count == hv_alarms_period ){
			hv_count = 0;
			hv_read_alarms();
		}

		wait_period(&info);
	}

	return NULL;
}
/**
 * Periodic thread that is waiting for an alarm from any Xilinx AMS channel, the alarm is presented as an event,
 * events are reported by IIO EVENT MONITOR through shared memory.
 *
 * @param arg must be NULL
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

	printf("AMS Alarms thread period: %3.4fms\n",((float)periods[0])/1000);
	rc = make_periodic(periods[0], &info);
	if (rc) {
		printf("Error\r\n");
		return NULL;
	}
	sem_post(&thread_sync);
	while(1){
        sem_wait(&memory->full);  //Semaphore to wait until any event happens

        res[0] = 0;
        rc = 0;

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
    			printf("AMS Voltage file could not be opened!!! \n");/*Any of the files could not be opened*/
    			}
    		else if(chan >= 7){
    			fseek(raw, 0, SEEK_END);
    			long fsize = ftell(raw);
    			fseek(raw, 0, SEEK_SET);  /* same as rewind(f); */

    			char *raw_string = static_cast<char *>(malloc(fsize + 1));
    			fread(raw_string, fsize, 1, raw);

    			fseek(rising, 0, SEEK_END);
    			fsize = ftell(rising);
    			fseek(rising, 0, SEEK_SET);  /* same as rewind(f); */

    			char *ris_string = static_cast<char *>(malloc(fsize + 1));
    			fread(ris_string, fsize, 1, rising);

    			if(atof(ris_string)>=atof(raw_string))
    				strcpy(ev_type,"falling");
    			else
    				strcpy(ev_type,"rising");
    			free(ris_string);
    			free(raw_string);


    			rc = alarm_json("DPB",ams_channels[chan-7],ev_type, 99, res[0],timestamp,"warning");
    			//printf("Chip: AMS. Event type: %s. Timestamp: %lld. Channel type: %s. Channel: %d. Value: %f V\n",ev_type,timestamp,ch_type,chan,res[0]);
    			fclose(raw);
    			fclose(rising);
    		}

        }
        else if(!strcmp(ch_type,"temp") && chan >= 7){
        	xlnx_ams_read_temp(&chan,1,res);
			// FIXME: DAQ Function here. Replace alarm_json by DAQ function
        	rc = alarm_json("DPB",ams_channels[chan-7],ev_type, 99, res[0],timestamp,"warning");
            //printf("Chip: AMS. Event type: %s. Timestamp: %lld. Channel type: %s. Channel: %d. Value: %f ºC\n",ev_type,timestamp,ch_type,chan,res[0]);
        }
		if (rc) {
			printf("Error\r\n");
		}
		wait_period(&info);
	}
	return NULL;
}

/**
 * Periodic thread that is waiting for a command from the DAQ and handling it
 *
 * @param arg must be NULL
 *
 * @return  NULL (if exits is because of an error).
 */
static void *command_thread(void *arg){

	struct periodic_info info;
	int rc ;
	struct DPB_I2cSensors *data = static_cast<DPB_I2cSensors *>(arg);

	printf("Command thread period: %3.4fms\n",((float)periods[3])/1000);
	rc = make_periodic(periods[3], &info);
	if (rc) {
		printf("Error\r\n");
		return NULL;
	}
	while(1){
		json_object * jid;
		json_object * jcmd;
		json_object *jobj;
		int size;
		char *cmd[6];
		char aux_buff[256];
		char buffer[256];
		int msg_id;
		char reply[256];

		const char *serialized_json;
		const char *serialized_json_msg;
		int words_n;

		size = zmq_recv(cmd_router, aux_buff, 255, 0);
		if (size == -1)
		  return NULL;
		if (size > 255)
		  size = 255;
		aux_buff[size] = '\0';
		strcpy(buffer,aux_buff);
		json_object * jmsg = json_tokener_parse(buffer);
		if(jmsg == NULL){
			rc = command_status_response_json (0,-EINCMD,reply);
			goto waitmsg;
		}
		serialized_json_msg = json_object_to_json_string(jmsg);
		rc = json_schema_validate("JSONSchemaSlowControl.json",serialized_json_msg, "cmd_temp.json");
		if(rc){
			rc = command_status_response_json (0,-EINCMD,reply);
			goto waitmsg;
		}
		json_object_object_get_ex(jmsg, "msg_id", &jid);
		json_object_object_get_ex(jmsg, "msg_value", &jcmd);

		strcpy(buffer,json_object_get_string(jcmd));
		msg_id = json_object_get_int(jid);

		cmd[0] = strtok(buffer," ");
		words_n = 0;
		while( cmd[words_n] != NULL ) {
		   words_n++;
		   cmd[words_n] = strtok(NULL, " ");
		}
		jobj = json_object_new_object();
		char buff[512];

		switch(words_n){
		case 5:
			json_object *jstr5;
			if(!strcmp(cmd[4],"ON") | !strcmp(cmd[4],"OFF")){
				jstr5 = json_object_new_string(cmd[4]);
			}
			else{
				float val = atof(cmd[4]);
				sprintf(buff, "%f", val);
				jstr5 = json_object_new_double_s((double) val,buff);
			}
		case 4:
			json_object *jstr4;
			jstr4 = json_object_new_string(cmd[3]);
		case 3: {
				json_object *jstr3 = json_object_new_string(cmd[2]);
				json_object *jstr2 = json_object_new_string(cmd[1]);
				json_object *jstr1 = json_object_new_string(cmd[0]);

				json_object_object_add(jobj,"operation", jstr1);
				json_object_object_add(jobj,"board", jstr2);
				json_object_object_add(jobj,"magnitude", jstr3);

				if(words_n>=4){
					json_object_object_add(jobj,"channel", jstr4);
				}
				else{
					json_object *jempty = json_object_new_string("");
					json_object_object_add(jobj,"channel", jempty);
				}
				if(words_n == 5){
					json_object_object_add(jobj,"write_value", jstr5);
				}
				else{
					json_object *jempty = json_object_new_string("");
					json_object_object_add(jobj,"write_value", jempty);
				}
			}
			break;
		default:
			break;
		}
		//Check JSON schema valid
		serialized_json = json_object_to_json_string(jobj);
		// TODO: Add digitizer commands to the JSON schema before removing this comment
		//rc = json_schema_validate("JSONSchemaCommandRequest.json",serialized_json, "cmd_temp.json");
		rc= 0;
		if(rc){
			rc = command_status_response_json (msg_id,-EINCMD,reply);
		}
		else{
			char board_response[64];
			if(!strcmp(cmd[1],"LV")){
				// Implement CPU toggling of LV (Only SET command)
				if(!strcmp(cmd[2],"CPU")){
					int gpio_cpu_addr;
					int gpio_cpu_val;
					if(!strcmp(cmd[3],"MAIN")){
						gpio_cpu_addr = LV_MAIN_CPU_GPIO_OFFSET;
					}
					else{
						gpio_cpu_addr = LV_BACKUP_CPU_GPIO_OFFSET;
					}
					if(!strcmp(cmd[4],"ON")){
						gpio_cpu_val = 1;
					}
					else{
						gpio_cpu_val = 0;
					}
					write_GPIO(gpio_cpu_addr,gpio_cpu_val);
					command_status_response_json (msg_id,99,reply);
				}
				else if(lv_connected){	
					char board_dev[64] = "/dev/ttyUL4";
					//Command conversion
					char hvlvcmd[40] =  "$BD:0,$CMD:";
					rc = hv_lv_command_translation(hvlvcmd, cmd, words_n);
					//RS485 communication
					rc = hv_lv_command_handling(board_dev,hvlvcmd, board_response);
					// Generate the JSON message depending on reading or setting
					rc = hv_lv_command_response(board_response,reply,msg_id,cmd);
				}
				else{
					strcpy(board_response,"ERROR: READ operation not successful");
					rc = hv_lv_command_response(board_response,reply,msg_id,cmd);
				}
			}
			else if(!strcmp(cmd[1],"HV")){
				// Implement CPU toggling of HV (only SET command)
				if(!strcmp(cmd[2],"CPU")){
					int gpio_cpu_addr;
					int gpio_cpu_val;
					if(!strcmp(cmd[3],"MAIN")){
						gpio_cpu_addr = HV_MAIN_CPU_GPIO_OFFSET;
					}
					else{
						gpio_cpu_addr = HV_BACKUP_CPU_GPIO_OFFSET;
					}
					if(!strcmp(cmd[4],"ON")){
						gpio_cpu_val = 1;
					}
					else{
						gpio_cpu_val = 0;
					}
					write_GPIO(gpio_cpu_addr,gpio_cpu_val);
					command_status_response_json (msg_id,99,reply);
				}
				else if(hv_connected){
					char board_dev[64] = "/dev/ttyUL3";
					//Command conversion
					char hvlvcmd[40] =  "$BD:1,$CMD:";
					rc = hv_lv_command_translation(hvlvcmd, cmd, words_n);
					if(rc){
						printf("HV/LV Command not valid \n");
						strcpy(board_response,"ERROR: READ operation not successful");
						command_response_string_json(msg_id,board_response,reply);
					}
					else{
					//RS485 communication
					rc = hv_lv_command_handling(board_dev,hvlvcmd, board_response);
					// Generate the JSON message depending on reading or setting
					rc = hv_lv_command_response(board_response,reply,msg_id,cmd);
					}
				}
				else{
					strcpy(board_response,"ERROR: READ operation not successful");
					command_response_string_json(msg_id,board_response,reply);
				}
			}
			else if(!strcmp(cmd[1],"DIG0")){ //Digitizer 0
				char digcmd[32];
				char board_response[32];
				//Command conversion
				rc = dig_command_translation(digcmd, cmd, words_n);
				if(rc){
					printf("DIG0 Command not valid \n");
					strcpy(board_response,"ERROR: READ operation not successful");
					command_response_string_json(msg_id,board_response,reply);
				}
				else{
					//Serial Port Communication
					rc = dig_command_handling(0, digcmd, board_response);
					// Generate the JSON message depending on reading or setting
					rc = dig_command_response(board_response,reply,msg_id,cmd);
				}

			}
			else if(!strcmp(cmd[1],"DIG1")){ //Digitizer 1
				char digcmd[32];
				char board_response[32];
				//Command conversion
				rc = dig_command_translation(digcmd, cmd, words_n);
				if(rc){
					printf("DIG1 Command not valid \n");
					strcpy(board_response,"ERROR: READ operation not successful");
					command_response_string_json(msg_id,board_response,reply);
				}
				else{
					//Serial Port Communication
					rc = dig_command_handling(1, digcmd, board_response);
					// Generate the JSON message depending on reading or setting
					rc = dig_command_response(board_response,reply,msg_id,cmd);
				}
			}
			else{ //DPB
				rc = dpb_command_handling(data,cmd,msg_id,reply);
			}
		}
		// Free JSON objects after using them
		json_object_put(jmsg);
		json_object_put(jobj);
waitmsg:
	const char* msg_sent = (const char*) reply;
	//FIXME: DAQ Function HERE. Use whole command_thread function as callback function for DAQ library and parse string into DPB command format
	zmq_send(cmd_router,msg_sent, strlen(msg_sent), 0);
	wait_period(&info);
	}

	return NULL;
}
/** @} */
/************************** Main function ******************************/

int main(int argc, char *argv[]){

	setbuf(stdout, NULL);
	sigset_t alarm_sig;
	int i;
	int rc;
	int serial_port_fd;
	int	n;
	CCOPacket pkt(COPKT_DEFAULT_START, COPKT_DEFAULT_STOP, COPKT_DEFAULT_SEP);

	for(int i = 1 ; i < 5; i++) {
		if(argc <= i)
			switch(i){
				case 1:
				periods[i-1] = AMS_ALARMS_THREAD_PERIOD_DEFAULT;
				break;
				case 2:
				periods[i-1] = ALARMS_THREAD_PERIOD_DEFAULT;
				break;
				case 3:
				periods[i-1] = MONIT_THREAD_PERIOD_DEFAULT;
				break;
				case 4:
				periods[i-1] = COMMAND_THREAD_PERIOD_DEFAULT;
				break;
			}
		else
			periods[i-1] = atoi(argv[i]);
	}

	rc = dpbsc_lib_init(&data);
	if(rc){
		goto end;
	}

	rc = iio_event_monitor_up(); //Initialize iio event monitor
	if (rc) {
		printf("Error\r\n");
		return rc;
	}
	// Enable HV LV driver
	write_GPIO(HVLV_DRV_ENABLE_GPIO_OFFSET,1);
	//Enable Main CPUs of both HV and LV
	write_GPIO(LV_MAIN_CPU_GPIO_OFFSET,1);
	write_GPIO(HV_MAIN_CPU_GPIO_OFFSET,1);

	// Create Signal handlers
	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGSEGV, segmentation_handler);
	// Check if Dig0 and Dig1 are there
	char buffer[40];

	serial_port_fd = open("/dev/ttyUL1",O_RDWR);
	setup_serial_port(serial_port_fd);
	pkt.CreatePacket(buffer, HkDigCmdList.CmdList[HKDIG_GET_GW_VER].CmdString);
	write(serial_port_fd, buffer, strlen(buffer));
	usleep(1000000);
	n = read(serial_port_fd, buffer, sizeof(buffer));
	if(n){
		pkt.LoadString(buffer);
		int16_t cmd_id = pkt.GetNextFiedlAsCOMMAND(HkDigCmdList);
		uint16_t gw_ver;
		pkt.GetNextFieldAsUINT16(gw_ver);
		sprintf(DIG0_SN,"%d",gw_ver);
		printf("Digitizer 0 has been detected: GW Version %s \n",DIG0_SN);
		dig0_connected = 1;
	}
	close(serial_port_fd);
	
	serial_port_fd = open("/dev/ttyUL2",O_RDWR);
	setup_serial_port(serial_port_fd);
	pkt.CreatePacket(buffer, HkDigCmdList.CmdList[HKDIG_GET_GW_VER].CmdString);
	write(serial_port_fd, buffer, strlen(buffer));
	usleep(1000000);
	n = read(serial_port_fd, buffer, sizeof(buffer));
	if(n){
		pkt.LoadString(buffer);
		int16_t cmd_id = pkt.GetNextFiedlAsCOMMAND(HkDigCmdList);
		if(cmd_id == HKDIG_GET_GW_VER){	
			uint16_t gw_ver;
			pkt.GetNextFieldAsUINT16(gw_ver);
			sprintf(DIG1_SN,"%d",gw_ver);
			printf("Digitizer 1 has been detected: S/N %s \n",DIG1_SN);
			dig1_connected = 1;
		}
	}
	close(serial_port_fd);

	// Check if HV and LV are there
	serial_port_fd = open("/dev/ttyUL3",O_RDWR);
	usleep(1000000);
	setup_serial_port(serial_port_fd);
	write(serial_port_fd, "$BD:1,$CMD:MON,PAR:BDSNUM\r\n", strlen("$BD:1,$CMD:MON,PAR:BDSNUM\r\n"));
	usleep(1000000);
	n = read(serial_port_fd, buffer, sizeof(buffer));
	buffer[n] = '\0';
	if(n){
		for(int i = 12; i <=17; i++ ){ // Take just serial number from the response
			HV_SN[i-12] = buffer[i];
		}
		printf("HV has been detected: S/N %s \n",HV_SN);
		hv_connected = 1;
	}
	write(serial_port_fd, "$BD:0,$CMD:MON,PAR:BDSNUM\r\n", strlen("$BD:0,$CMD:MON,PAR:BDSNUM\r\n"));
	usleep(1000000);
	n = read(serial_port_fd, buffer, sizeof(buffer));
	buffer[n] = '\0';
	if(n){
		for(int i = 12; i <=17; i++ ){ // Take just serial number from the response
			LV_SN[i-12] = buffer[i];
		}
		printf("LV has been detected: S/N %s \n", LV_SN);
		lv_connected = 1;
	}
	close(serial_port_fd);

	sem_init(&thread_sync,0,0);

	/* Block all real time signals so they can be used for the timers.
	   Note: this has to be done in main() before any threads are created
	   so they all inherit the same mask. Doing it later is subject to
	   race conditions*/

	sigemptyset(&alarm_sig);
	for (i = SIGRTMIN; i <= SIGRTMAX; i++)
		sigaddset(&alarm_sig, i);
	sigprocmask(SIG_BLOCK, &alarm_sig, NULL);

	pthread_create(&t_1, NULL, ams_alarms_thread,NULL); //Create thread 1 - reads AMS alarms
	sem_wait(&thread_sync);
	pthread_create(&t_2, NULL, i2c_alarms_thread,(void *)&data); //Create thread 2 - reads I2C alarms every x miliseconds
	sem_wait(&thread_sync);
	pthread_create(&t_3, NULL, monitoring_thread,(void *)&data);//Create thread 3 - monitors magnitudes every x seconds
	sem_wait(&thread_sync); //Avoids race conditions
	pthread_create(&t_4, NULL, command_thread,(void *)&data);//Create thread 4 - waits and attends commands

	while(1){
		sleep(100);
		if(break_flag == 1){
			break;
		}
	}
end:
	return 0;
}
}
