/*
 *
 * @date   12-04-2024
 * @author Borja Martínez Sánchez , Alejandro Gómez Gambín
 */

/************************** Libraries includes *****************************/
// COPacket includes
#include <common/protocols/COPacket/COPacket.hpp>
#include <COPacketCmdHkDig.h>
#include <daq_inter_obj.h>
#ifdef DAQ_MODE
	#include <daqinterface/DAQInterface.h>
#endif

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
/** @brief Configuration Thread */
pthread_t t_5;
/** @brief periods for each of the threads in order (1 = AMS alarms 2= Other alarms 3= Monitoring 4 = Command handling) */
int periods[5];

/** @} */

int break_flag = 0;
extern _COPacketCmdList HkDigCmdList;
/******************************************************************************
*Threads timers (ms).
****************************************************************************/
#define MONIT_THREAD_PERIOD_DEFAULT 5000000
#define ALARMS_THREAD_PERIOD_DEFAULT 100000
#define AMS_ALARMS_THREAD_PERIOD_DEFAULT 100000
#define COMMAND_THREAD_PERIOD_DEFAULT 50000
#define HV_LV_SLEEP_DELAY_DEFAULT 0

/************************** Function Prototypes ******************************/


int iio_event_monitor_up();
void sighandler(int );
void segmentation_handler(int );
static void *monitoring_thread(void *);
static void *i2c_alarms_thread(void *);
static void *ams_alarms_thread(void *);
static void *command_thread(void *);
static void *config_thread(void *);

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
	#ifndef DAQ_MODE
	pthread_cancel(t_4);
	#endif
	pthread_cancel(t_3);

	pthread_join(t_1,NULL);
	pthread_join(t_2,NULL);
	pthread_join(t_3,NULL);
	#ifndef DAQ_MODE
	pthread_join(t_4,NULL);
	#endif

	dpbsc_lib_close(&data);
	break_flag = 1;
	return;
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
	//struct DPB_I2cSensors *data = i2c_data;
	struct DPB_I2cSensors *data = static_cast<DPB_I2cSensors *>(arg);

	int eth_status[2];
	uint8_t tdm_active_link = -1;
	uint8_t tdm_main_mgt_status = -1;
	uint8_t tdm_backup_mgt_status = -1;
	uint32_t dma_packet_size = 0;
	uint32_t rmon_dig0 = 0;
	uint32_t rmon_dig1 = 0;
	uint32_t rmon_dig0_mux = 0;
	uint32_t rmon_dig1_mux = 0;
	uint32_t rmon_dma_source = 0;
	uint32_t rmon_dma = 0;
	uint16_t rmon_timebase = 0;
	char tdm_active_link_str[12];

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
	uint8_t sfp_status[SFP_NUM][2];

	char dma_source_str[16];
	char dpb_multiboot_reg_str[32];

	LOG_PRINTF("Monitoring thread period: %3.4fs\n",((float)periods[2])/1000000);
	rc = make_periodic(periods[2], &info);
	if (rc) {
		LOG_PRINTF("Error creating monitoring thread\r\n");
		return NULL;
	}
	sem_post(&thread_sync);
	while (1) {
		// Do a preliminary checking for Dig0, Dig1, HV and LV
		check_hv_lv_presence();
		check_digs_presence();
		// Do a preliminary checking for SFPs
		check_sfp_presence(data);
		/* DPB Slow Control Monitoring */
		// DPB PCB Temperature sensor
		sem_wait(&i2c_sync); //Semaphore to sync I2C usage
		rc = mcp9844_read_temperature(data,temp);
		if (rc) {
			LOG_PRINTF("Reading Error PCB Temperature\r\n");
		}
		// SFP monitoring
		for(int i = 0; i < SFP_NUM;i++){
				if(sfp_connected[i]){
				rc = sfp_avago_read_temperature(data,i,&sfp_temp[i]);
				if (rc) {
					// Reset the I2C Expander to avoid timeout errors
					write_GPIO(I2C_MUX_RESET,1);
					usleep(100);
					write_GPIO(I2C_MUX_RESET,0);
					LOG_PRINTF("Reading Error SFP temperature\r\n");
				}
				rc = sfp_avago_read_voltage(data,i,&sfp_vcc[i]);
				if (rc) {
					write_GPIO(I2C_MUX_RESET,1);
					usleep(100);
					write_GPIO(I2C_MUX_RESET,0);
					LOG_PRINTF("Reading Error SFP voltage\r\n");
				}
				rc = sfp_avago_read_lbias_current(data,i,&sfp_txbias[i]);
				if (rc) {
					write_GPIO(I2C_MUX_RESET,1);
					usleep(100);
					write_GPIO(I2C_MUX_RESET,0);
					LOG_PRINTF("Reading Error SFP Bias Current\r\n");
				}
				rc = sfp_avago_read_tx_av_optical_pwr(data,i,&sfp_txpwr[i]);
				if (rc) {
					write_GPIO(I2C_MUX_RESET,1);
					usleep(100);
					write_GPIO(I2C_MUX_RESET,0);
					LOG_PRINTF("Reading Error SFP TX power\r\n");
				}
				rc = sfp_avago_read_rx_av_optical_pwr(data,i,&sfp_rxpwr[i]);
				if (rc) {
					write_GPIO(I2C_MUX_RESET,1);
					usleep(100);
					write_GPIO(I2C_MUX_RESET,0);
					LOG_PRINTF("Reading Error RX Power\r\n");
				}
				rc = sfp_avago_read_status(data,i,sfp_status[i]);
				if (rc) {
					write_GPIO(I2C_MUX_RESET,1);
					usleep(100);
					write_GPIO(I2C_MUX_RESET,0);
					LOG_PRINTF("Reading Error SFP Status\r\n");
				
				}
			}
		}
		rc = ina3221_get_voltage(data,0,volt_sfp0_2);
		if (rc) {
			LOG_PRINTF("Reading Error Voltage\r\n");
		}
		rc = ina3221_get_voltage(data,1,volt_sfp3_5);
		if (rc) {
			LOG_PRINTF("Reading Error Voltage\r\n");
		}
		rc = ina3221_get_voltage(data,2,volt_som);
		if (rc) {
			LOG_PRINTF("Reading Error SOM voltage\r\n");
		}
		rc = ina3221_get_current(data,0,curr_sfp0_2);
		if (rc) {
			LOG_PRINTF("Reading Error Current\r\n");
		}
		rc = ina3221_get_current(data,1,curr_sfp3_5);
		if (rc) {
			LOG_PRINTF("Reading Error Current\r\n");
		}
		rc = ina3221_get_current(data,2,curr_som);
		if (rc) {
			LOG_PRINTF("Reading Error Current\r\n");
		}
		sem_post(&i2c_sync);//Free semaphore to sync I2C usage

		rc = xlnx_ams_read_temp(temp_chan,AMS_TEMP_NUM_CHAN,ams_temp);
		if (rc) {
			LOG_PRINTF("Reading Error AMS Temperature\r\n");
		}
		rc = xlnx_ams_read_volt(volt_chan,AMS_VOLT_NUM_CHAN,ams_volt);
		if (rc) {
			LOG_PRINTF("Reading Error AMS Voltage\r\n");
		}
		rc = eth_link_status("eth0",&eth_status[0]);
		if (rc) {
			LOG_PRINTF("Reading Error Ethernet 0 status\r\n");
		}
		rc = eth_link_status("eth1",&eth_status[1]);
		if (rc) {
			LOG_PRINTF("Reading Error Ethernet 1 Status\r\n");
		}
		rc = read_uio(REG_TIMING_LINK_SWITCH,&tdm_active_link);
		if (rc) {
			LOG_PRINTF("Reading Error Timing register\r\n");
		}
		if(tdm_active_link){
			strcpy(tdm_active_link_str,"BACKUP"); // TDM is using backup  link
		}
		else{
			strcpy(tdm_active_link_str,"MAIN"); // TDM using main link
		}

		rc = read_uio(REG_TIMING_MGT_MAIN_SWITCH,&tdm_main_mgt_status);
		if (rc) {
			LOG_PRINTF("Reading Error Timing Register switch\r\n");
		}

		rc = read_uio(REG_TIMING_MGT_BACKUP_SWITCH,&tdm_backup_mgt_status);
		if (rc) {
			LOG_PRINTF("Reading Error Timing Register switch\r\n");
		}

		strcpy(dma_source_str,(dma_source_flag)?"DIG":"COUNTER");

		snprintf(dpb_multiboot_reg_str,32,"0x%08X",dpb_multiboot_reg);

		rc = read_uio(REG_DMA_BUF_SIZE,&dma_packet_size);
		if (rc) {
			LOG_PRINTF("Reading Error DMA Buffer Size\r\n");
		}

		rc = read_uio(REG_RMON_CONFIG_TIMEBASE,&rmon_timebase);
		if (rc) {
			LOG_PRINTF("Reading Error RMON TIMEBASE\r\n");
		}

		rc = read_uio(REG_RMON_DIG0,&rmon_dig0);
		if (rc) {
			LOG_PRINTF("Reading Error RMON DIG0\r\n");
		}
		rmon_dig0 = rmon_dig0 * (100 / rmon_timebase);

		rc = read_uio(REG_RMON_DIG1,&rmon_dig1);
		if (rc) {
			LOG_PRINTF("Reading Error RMON DIG1\r\n");
		}
		rmon_dig1 = rmon_dig1 * (100 / rmon_timebase);

		rc = read_uio(REG_RMON_DIG0_MUX,&rmon_dig0_mux);
		if (rc) {
			LOG_PRINTF("Reading Error RMON DIG0 MUX\r\n");
		}
		rmon_dig0_mux = rmon_dig0_mux * (100 / rmon_timebase);
		
		rc = read_uio(REG_RMON_DIG1_MUX,&rmon_dig1_mux);
		if (rc) {
			LOG_PRINTF("Reading Error RMON DIG1 MUX\r\n");
		}
		rmon_dig1_mux = rmon_dig1_mux * (100 / rmon_timebase);

		rc = read_uio(REG_RMON_DMA_SOURCE,&rmon_dma_source);
		if (rc) {
			LOG_PRINTF("Reading Error RMON DMA SOURCE\r\n");
		}
		rmon_dma_source = rmon_dma_source * (100 / rmon_timebase);

		rc = read_uio(REG_RMON_DMA,&rmon_dma);
		if (rc) {
			LOG_PRINTF("Reading Error RMON DMA\r\n");
		}
		rmon_dma = rmon_dma * (100 / rmon_timebase);
		// rc = poll_GPIO(dig0_aurora_main_fd,DIG0_MAIN_AURORA_LINK,&dig0_aurora_main_val);
		// if (rc) {
		// 	LOG_PRINTF("Reading Error\r\n");
		// }
		// rc = poll_GPIO(dig0_aurora_backup_fd,DIG0_BACKUP_AURORA_LINK,&dig0_aurora_backup_val);
		// if (rc) {
		// 	LOG_PRINTF("Reading Error\r\n");
		// }
		// rc = poll_GPIO(dig1_aurora_main_fd,DIG1_MAIN_AURORA_LINK,&dig1_aurora_main_val);
		// if (rc) {
		// 	LOG_PRINTF("Reading Error\r\n");
		// }
		// rc = poll_GPIO(dig1_aurora_backup_fd,DIG1_BACKUP_AURORA_LINK,&dig1_aurora_backup_val);
		// if (rc) {
		// 	LOG_PRINTF("Reading Error\r\n");
		// }
		// rc = poll_GPIO(pll_locked_fd,PLL_LOL_N,&pll_locked_val);
		// if (rc) {
		// 	LOG_PRINTF("Reading Error\r\n");
		// }
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
		parsing_mon_environment_status_into_object(jdpb, "plllocked", pll_locked_val);
		parsing_mon_environment_status_into_object(jdpb, "tdmlocked", tdm_locked_val);

		parsing_mon_environment_string_into_object(jdpb,"tdmactivelink", tdm_active_link_str);
		
		// Registers for MGT Switch are inverted (0 means ON)
		parsing_mon_environment_status_into_object(jdpb, "timingmaintx", !(tdm_main_mgt_status & 0x2));
		parsing_mon_environment_status_into_object(jdpb, "timingmainrx", !(tdm_main_mgt_status & 0x1));
		parsing_mon_environment_status_into_object(jdpb, "timingbackuptx", !(tdm_backup_mgt_status & 0x2));
		parsing_mon_environment_status_into_object(jdpb, "timingbackuprx", !(tdm_backup_mgt_status & 0x1));

		parsing_mon_environment_status_into_object(jdig0, "auroramain", dig0_aurora_main_val);
		parsing_mon_environment_status_into_object(jdig0, "aurorabackup", dig0_aurora_backup_val);

		parsing_mon_environment_status_into_object(jdig1, "auroramain", dig1_aurora_main_val);
		parsing_mon_environment_status_into_object(jdig1, "aurorabackup", dig1_aurora_backup_val);

		parsing_mon_environment_data_into_object(jdpb,"boardtemp", temp[0]);

		// DMA
		parsing_mon_environment_status_into_object(jdpb,"dmastatus", dma_flag);
		parsing_mon_environment_string_into_object(jdpb,"dmasource", dma_source_str);
		parsing_mon_environment_integer_into_object(jdpb,"dmapktsize", dma_packet_size);

		//RMON
		parsing_mon_environment_integer_into_object(jdpb,"rmondig0", rmon_dig0);
		parsing_mon_environment_integer_into_object(jdpb,"rmondig1", rmon_dig1);
		parsing_mon_environment_integer_into_object(jdpb,"rmondig0mux", rmon_dig0_mux);
		parsing_mon_environment_integer_into_object(jdpb,"rmondig1mux", rmon_dig1_mux);
		parsing_mon_environment_integer_into_object(jdpb,"rmondmasource", rmon_dma_source);
		parsing_mon_environment_integer_into_object(jdpb,"rmondma", rmon_dma);

		// DPB properties
		parsing_mon_environment_string_into_object(jdpb,"serialnumber", dpb_sn);
		parsing_mon_environment_string_into_object(jdpb,"multibootreg", dpb_multiboot_reg_str);

		//Include SFP data in JSON object if they are connected
		for(int i = 0; i < SFP_NUM; i++){
			if(sfp_connected[i]){
			parsing_mon_channel_data_into_object(jsfps,i,"temperature",sfp_temp[i]);
			parsing_mon_channel_data_into_object(jsfps,i,"biascurr",sfp_txbias[i]);
			parsing_mon_channel_data_into_object(jsfps,i,"txpwr",sfp_txpwr[i]);
			parsing_mon_channel_data_into_object(jsfps,i,"rxpwr",sfp_rxpwr[i]);

			parsing_mon_channel_status_into_object(jsfps,i,"rxlos",sfp_status[i][0]);
			parsing_mon_channel_status_into_object(jsfps,i,"txfault",sfp_status[i][1]);
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
		char dig_response[64];
		char bme_data[32];
		char *dig_mag_str;
		float dig_value;
		int32_t tf;
		if(dig0_connected){
			// Board parameters
			for(int i = 0; i < DIG_MON_BOARD_CODES_SIZE; i++){
				pkt.CreatePacket(digcmd, HkDigCmdList.CmdList[dig_monitor_mag_board_codes[i]].CmdString);
				rc = dig_command_handling(DIGITIZER_0,digcmd,dig_response);
				if(rc){ // If the function returns error, we cancel reading on this board until check_board_presence() returns that it is available again
					goto skip_dig0;
				}
				pktError = pkt.LoadString(dig_response);
				int16_t cmdIdx = pkt.GetNextFiedlAsCOMMAND(HkDigCmdList);
				switch(cmdIdx){
					//String
					case HKDIG_GET_HW_VER:
					case HKDIG_GET_GW_VER:
					case HKDIG_GET_GW_DATE:
    				case HKDIG_GET_SW_VER:
    				case HKDIG_GET_BOARD_STATUS:
    				case HKDIG_GET_BOARD_CNTRL:
					case HKDIG_GET_UPTIME:
					case HKDIG_GET_RMON_PER:
					case HKDIG_GET_TLNK_LOCK:
					case HKDIG_GET_EEPROM_OUI:			// Returns EEPROM OUI code
					case HKDIG_GET_EEPROM_EID:
					case HKDIG_GET_RMON_MUX_N:
					case HKDIG_GET_RMON_RST_N:
					case HKDIG_GET_PED_STAGGER:
					case HKDIG_GET_PED_PERIOD:
					case HKDIG_GET_TB_REG:
					case HKDIG_GET_OD_SEL_REG:
						dig_mag_str=pkt.GetNextField();
						parsing_mon_environment_string_into_object(jdig0, dig_monitor_mag_board_names[i],dig_mag_str);
						break;
					// BME280 commands
					case HKDIG_GET_BME_DATA:
						// Just copy data for next iterations
						dig_mag_str = pkt.GetNextField();
						strcpy(bme_data,dig_mag_str);
						bme280_get_temp(bme_data,dig0_calT,&tf,&dig_value);
						parsing_mon_environment_data_into_object(jdig0, "temp",dig_value);
						bme280_get_relhum(bme_data,dig0_calH,&tf,&dig_value);
						parsing_mon_environment_data_into_object(jdig0, "relathumidity",dig_value);				
						bme280_get_press(bme_data,dig0_calP,&tf,&dig_value);
						parsing_mon_environment_data_into_object(jdig0, "pressure",dig_value);
						break;
					//Float
					case HKDIG_GET_BOARD_3V3A:
					case HKDIG_GET_BOARD_12VA:
					case HKDIG_GET_BOARD_I12V:
					case HKDIG_GET_BOARD_5V0A:
					case HKDIG_GET_BOARD_5V0F:
					case HKDIG_GET_BOARD_C12V:
					case HKDIG_GET_BOARD_I5VA:
					case HKDIG_GET_BOARD_I3V3A:
					case HKDIG_GET_BOARD_I12VA:
					case HKDIG_GET_BOARD_TCH0:
					case HKDIG_GET_BOARD_TCH11:
					case HKDIG_GET_BOARD_TCH0R:
					case HKDIG_GET_BOARD_TCH11R:
					case HKDIG_GET_BOARD_5VOD:
					case HKDIG_GET_BOARD_I5VOD:
						pkt.GetNextFieldAsFLOAT(dig_value);
						dig_value = dig_value / 1000;  //Convert from mV/mA to V/A
						parsing_mon_environment_data_into_object(jdig0, dig_monitor_mag_board_names[i],dig_value);
						break;
					case HKDIG_GET_BOARD_TFE:
					case HKDIG_GET_BOARD_TFPGA:
					case HKDIG_GET_BOARD_TPWR:
						pkt.GetNextFieldAsFLOAT(dig_value);
						dig_value = dig_value / 100;  //Convert 100ths of degrees to degrees
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
						char dig_error[8];
						strcpy(dig_error, "ERROR");
						parsing_mon_environment_string_into_object(jdig0, dig_monitor_mag_board_names[i],dig_error);
						break;
				}
			}

			// Channel parameters
			json_object *jdig0channels = json_object_new_array();
			for(int i = 0; i < DIG_MON_CHAN_CODES_SIZE; i++){
					for(int j = 0; j < 18; j++){
					pkt.CreatePacket(digcmd, HkDigCmdList.CmdList[dig_monitor_mag_chan_codes[i]].CmdString,(uint32_t) j);
					rc = dig_command_handling(DIGITIZER_0,digcmd,dig_response);
					if(rc){ // If the function returns error, we cancel reading on this board until check_board_presence() returns that it is available again
						goto skip_dig0;
					}
					pktError = pkt.LoadString(dig_response);
					int16_t cmdIdx = pkt.GetNextFiedlAsCOMMAND(HkDigCmdList);
					switch(cmdIdx){
						//Float
						case HKDIG_GET_IT_NUM:
						case HKDIG_GET_DT_NUM:
							pkt.GetNextFieldAsFLOAT(dig_value);
							pkt.GetNextFieldAsFLOAT(dig_value);
							parsing_mon_channel_data_into_object(jdig0channels,j, dig_monitor_mag_chan_names[i],dig_value);
							break;

						//String
						case HKDIG_GET_THR_NUM:
						case HKDIG_GET_CHN_STATUS:
						case HKDIG_GET_CHN_CNTRL:
						case HKDIG_GET_RMON_ADC_N:
						case HKDIG_GET_RMON_TDC_N:
						case HKDIG_GET_RMON_FMT_N:
						case HKDIG_GET_CHN_LG_CHG:
						case HKDIG_GET_CHN_HG_CHG:
						case HKDIG_GET_PED_ENABLE:
						case HKDIG_RO_FMON_N:
							dig_mag_str = pkt.GetNextField();
							dig_mag_str = pkt.GetNextField();
							parsing_mon_channel_string_into_object(jdig0channels,j, dig_monitor_mag_chan_names[i],dig_mag_str);
							break;
						//Error
						case HKDIG_ERRO:
						default:
							char dig_error[8];
							strcpy(dig_error, "ERROR");
							parsing_mon_channel_string_into_object(jdig0channels,j, dig_monitor_mag_chan_names[i],dig_error);
							break;
					}
				}
			}
			json_object_object_add(jdig0,"channels",jdig0channels);
		}

skip_dig0:
		//Digitizer 1 Slow Control Monitoring
		if(dig1_connected){
			// Board parameters
			for(int i = 0; i < DIG_MON_BOARD_CODES_SIZE; i++){
				pkt.CreatePacket(digcmd, HkDigCmdList.CmdList[dig_monitor_mag_board_codes[i]].CmdString);
				rc = dig_command_handling(DIGITIZER_1,digcmd,dig_response);
				if(rc){ // If the function returns error, we cancel reading on this board until check_board_presence() returns that it is available again
					goto skip_dig1;
				}
				pktError = pkt.LoadString(dig_response);
				int16_t cmdIdx = pkt.GetNextFiedlAsCOMMAND(HkDigCmdList);
				switch(cmdIdx){
					//String
					case HKDIG_GET_HW_VER:
					case HKDIG_GET_GW_VER:
					case HKDIG_GET_GW_DATE:
    				case HKDIG_GET_SW_VER:
    				case HKDIG_GET_BOARD_STATUS:
    				case HKDIG_GET_BOARD_CNTRL:
					case HKDIG_GET_UPTIME:
					case HKDIG_GET_RMON_PER:
					case HKDIG_GET_TLNK_LOCK:
					case HKDIG_GET_EEPROM_OUI:			// Returns EEPROM OUI code
					case HKDIG_GET_EEPROM_EID:
					case HKDIG_GET_RMON_MUX_N:
					case HKDIG_GET_RMON_RST_N:
					case HKDIG_GET_PED_STAGGER:
					case HKDIG_GET_PED_PERIOD:
					case HKDIG_GET_TB_REG:
					case HKDIG_GET_OD_SEL_REG:
						dig_mag_str=pkt.GetNextField();
						parsing_mon_environment_string_into_object(jdig1, dig_monitor_mag_board_names[i],dig_mag_str);
						break;
					// BME280 commands
					case HKDIG_GET_BME_DATA:
						// Just copy data for next iterations
						dig_mag_str = pkt.GetNextField();
						strcpy(bme_data,dig_mag_str);
						bme280_get_temp(bme_data,dig1_calT,&tf,&dig_value);
						parsing_mon_environment_data_into_object(jdig1, "temp",dig_value);
						bme280_get_relhum(bme_data,dig1_calH,&tf,&dig_value);
						parsing_mon_environment_data_into_object(jdig1, "relathumidity",dig_value);				
						bme280_get_press(bme_data,dig1_calP,&tf,&dig_value);
						parsing_mon_environment_data_into_object(jdig1, "pressure",dig_value);
						break;
					//Float
					case HKDIG_GET_BOARD_3V3A:
					case HKDIG_GET_BOARD_12VA:
					case HKDIG_GET_BOARD_I12V:
					case HKDIG_GET_BOARD_5V0A:
					case HKDIG_GET_BOARD_5V0F:
					case HKDIG_GET_BOARD_C12V:
					case HKDIG_GET_BOARD_I5VA:
					case HKDIG_GET_BOARD_I3V3A:
					case HKDIG_GET_BOARD_I12VA:
					case HKDIG_GET_BOARD_TCH0:
					case HKDIG_GET_BOARD_TCH11:
					case HKDIG_GET_BOARD_TCH0R:
					case HKDIG_GET_BOARD_TCH11R:
					case HKDIG_GET_BOARD_5VOD:
					case HKDIG_GET_BOARD_I5VOD:
						pkt.GetNextFieldAsFLOAT(dig_value);
						dig_value = dig_value / 1000;  //Convert from mV/mA to V/A
						parsing_mon_environment_data_into_object(jdig1, dig_monitor_mag_board_names[i],dig_value);
						break;
					case HKDIG_GET_BOARD_TFE:
					case HKDIG_GET_BOARD_TFPGA:
					case HKDIG_GET_BOARD_TPWR:
						pkt.GetNextFieldAsFLOAT(dig_value);
						dig_value = dig_value / 100;  //Convert 100ths of degrees to degrees
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
						char dig_error[8];
						strcpy(dig_error, "ERROR");
						parsing_mon_environment_string_into_object(jdig1, dig_monitor_mag_board_names[i],dig_error);
						break;
				}
			}

			// Channel parameters
			json_object *jdig1channels = json_object_new_array();
			for(int i = 0; i < DIG_MON_CHAN_CODES_SIZE; i++){
					for(int j = 0; j < 18; j++){
					pkt.CreatePacket(digcmd, HkDigCmdList.CmdList[dig_monitor_mag_chan_codes[i]].CmdString,(uint32_t) j);
					rc = dig_command_handling(DIGITIZER_1,digcmd,dig_response);
					if(rc){ // If the function returns error, we cancel reading on this board until check_board_presence() returns that it is available again
						goto skip_dig1;
					}
					pktError = pkt.LoadString(dig_response);
					int16_t cmdIdx = pkt.GetNextFiedlAsCOMMAND(HkDigCmdList);
					switch(cmdIdx){

						//Float
						case HKDIG_GET_IT_NUM:
						case HKDIG_GET_DT_NUM:
							pkt.GetNextFieldAsFLOAT(dig_value);
							pkt.GetNextFieldAsFLOAT(dig_value);
							parsing_mon_channel_data_into_object(jdig1channels,j, dig_monitor_mag_chan_names[i],dig_value);
							break;

						//String
						case HKDIG_GET_THR_NUM:
						case HKDIG_GET_CHN_STATUS:
						case HKDIG_GET_CHN_CNTRL:
						case HKDIG_GET_RMON_ADC_N:
						case HKDIG_GET_RMON_TDC_N:
						case HKDIG_GET_RMON_FMT_N:
						case HKDIG_GET_CHN_LG_CHG:
						case HKDIG_GET_CHN_HG_CHG:
						case HKDIG_GET_PED_ENABLE:
						case HKDIG_RO_FMON_N:
							dig_mag_str = pkt.GetNextField();
							dig_mag_str = pkt.GetNextField();
							parsing_mon_channel_string_into_object(jdig1channels,j, dig_monitor_mag_chan_names[i],dig_mag_str);
							break;
						//Error
						case HKDIG_ERRO:
						default:
							char dig_error[8];
							strcpy(dig_error, "ERROR");
							parsing_mon_channel_string_into_object(jdig1channels,j, dig_monitor_mag_chan_names[i],dig_error);
							break;
					}
				}
			}
			json_object_object_add(jdig1,"channels",jdig1channels);
		}

skip_dig1:

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
			#ifdef HVLV_NORESISTORS
			strcpy(board_dev,"/dev/ttyUL4");
			#else
			strcpy(board_dev,"/dev/ttyUL3");
			#endif

			//Send Serial number
			parsing_mon_environment_string_into_object(jlv, lv_mag_names[0],LV_SN);

			//Send Firmware
			parsing_mon_environment_string_into_object(jlv, lv_mag_names[1],LV_FW);

			//Read Environment Parameters
			for(int i = 2 ; i < 7; i++){
				strcpy(lv_mon_cmd,lv_mon_root);
				strcat(lv_mon_cmd,lv_board_words[i]);
				strcat(lv_mon_cmd,"\r\n");
				rc = hv_lv_command_handling(board_dev,lv_mon_cmd,response);
				if(rc){
					goto skip_lv;
				}
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
					case 2: // Temperature
					case 3: // BCM Temperature
					case 4: // Relative Humidity
					case 5: // Pressure
						mag_value=(float) atoi(mag_str);
						parsing_mon_environment_data_into_object(jlv,lv_mag_names[i], mag_value);
						break;
					case 6: // Water Leak
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
					rc = hv_lv_command_handling(board_dev,lv_mon_cmd,response);
					if(rc){
						goto skip_lv;
					}
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
skip_lv:
		// HV Slow Control Monitoring
		char hv_mon_root[80];
		char hv_mon_cmd[80];
		int mag_status;
		if(hv_connected){

			json_object *jhvchannels = json_object_new_array();
			strcpy(board_dev,"/dev/ttyUL3");

			//Read Serial Number
			parsing_mon_environment_string_into_object(jhv,hv_mag_names[0], HV_SN);

			// Read Firmware Version
			parsing_mon_environment_string_into_object(jhv,hv_mag_names[1], HV_FW);

			//Read Board Temperature
			strcpy(hv_mon_cmd,"$BD:1,$CMD:MON,PAR:BDTEMP\r\n");
			rc = hv_lv_command_handling(board_dev,hv_mon_cmd,response);
			if(rc){
				goto skip_hv;
			}
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
			parsing_mon_environment_data_into_object(jhv,hv_mag_names[2], mag_value);

			//Read Channel Parameters
			strcpy(hv_mon_root,"$BD:1,$CMD:MON,CH:");
			for(int i = 0; i < 24; i++){
				for(int j = 3; j < HV_CMD_TABLE_SIZE; j++){
					strcpy(hv_mon_cmd,hv_mon_root);
					sprintf(channel_str,"%d",i);
					strcat(hv_mon_cmd,channel_str);
					strcat(hv_mon_cmd,",PAR:");
					strcat(hv_mon_cmd,hv_board_words[j]);
					strcat(hv_mon_cmd,"\r\n");
					rc = hv_lv_command_handling(board_dev,hv_mon_cmd,response);
					if(rc){
						goto skip_hv;
					}

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

					switch(j-3) {
						case 0:
						// If it is status, we strip the least significant bit from the string
						mag_status = atoi(mag_str) & 0x1;
						parsing_mon_channel_status_into_object(jhvchannels,i,hv_mag_names[j],mag_status);
						break;
						case 1:  //Voltage Monitor
						case 2: // Voltage Set
						case 3:	 //Current Monitor
						case 4: // Current Limit
						case 6:  // Temperature
						case 7:  //Rampup Speed
						case 8:  // Rampdown Speed
						case 9: // Trip Time
						mag_value = atof(mag_str);
						parsing_mon_channel_data_into_object(jhvchannels,i,hv_mag_names[j],mag_value);
						break;
						case 5: //Power Status
						parsing_mon_channel_string_into_object(jhvchannels,i,hv_mag_names[j],mag_str);
						break;
						case 10:
						// If it is the channel error, we strip the most significant bit
						mag_status = (atoi(mag_str) & (0x1 << 13)) >> 13;
						parsing_mon_channel_status_into_object(jhvchannels,i,hv_mag_names[j],mag_status);
						break;
						case 11:
						case 12:
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
skip_hv:
		json_object_object_add(jdpb,"SFPs",jsfps);

		json_object_object_add(jdata,"LV", jlv);
		json_object_object_add(jdata,"HV", jhv);
		json_object_object_add(jdata,"Dig0", jdig0);
		json_object_object_add(jdata,"Dig1", jdig1);
		json_object_object_add(jdata,"DPB", jdpb);

		const char *serialized_json = json_object_to_json_string(jdata);

		 #ifdef DAQ_MODE
		 	DAQ_Inter->SendMonitoringData(serialized_json);
		 #else
			rc2 = zmq_send(mon_publisher, serialized_json, strlen(serialized_json), 0);
			if (rc2 < 0) {
				LOG_PRINTF("Error sending JSON\r\n");
			}
		#endif
		json_object_put(jdata);
		wait_period(&info);
	}
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
	//struct DPB_I2cSensors *data = i2c_data;
	struct DPB_I2cSensors *data = static_cast<DPB_I2cSensors *>(arg);
	LOG_PRINTF("Alarms thread period: %3.4fms\n",((float)periods[1])/1000);
	rc = make_periodic(periods[1], &info);
	if (rc) {
		LOG_PRINTF("Error\r\n");
		return NULL;
	}
	int hv_alarms_period = 700000/periods[1]; //in us
	int hv_count = 0;
	sem_post(&thread_sync);

	// Trigger interrupt on falling edges of PLL not locked
	char edge_type[12];
	strcpy(edge_type,"both");
	write_GPIO_edge(PLL_LOL_N,edge_type);
	write_GPIO_edge(TDM_DPB_LOCK,edge_type);

	// Trigger interrupt on both edges of the 4 Aurora Links
	for (int i = 0; i < 4; i ++){
		strcpy(edge_type,"both");
		write_GPIO_edge(DIG0_MAIN_AURORA_LINK + i,edge_type);
	}
	while(1){
		
		if(!eths_shutdown[0]){
			rc = eth_down_alarm("eth0",&eth0_flag);
			if (rc) {
				LOG_PRINTF("Error reading alarm ethernet 0\r\n");
			}
		}

		if(!eths_shutdown[1]){
			rc = eth_down_alarm("eth1",&eth1_flag);
			if (rc) {
				LOG_PRINTF("Error reading alarm ethernet 1\r\n");
			}
		}

		rc = aurora_down_alarm(0,&dig0_main_flag);
		if (rc) {
			LOG_PRINTF("Error reading alarm aurora 0 main\r\n");
		}
		rc = aurora_down_alarm(1,&dig0_backup_flag);
		if (rc) {
			LOG_PRINTF("Error reading alarm aurora 0 backup\r\n");
		}
		rc = aurora_down_alarm(2,&dig1_main_flag);
		if (rc) {
			LOG_PRINTF("Error reading alarm aurora 1 main\r\n");
		}
		rc = aurora_down_alarm(3,&dig1_backup_flag);
		if (rc) {
			LOG_PRINTF("Error reading alarm aurora 1 backup\r\n");
		}
		rc = pll_not_locked_alarm();
		if (rc) {
			LOG_PRINTF("Error reading alarm PLL\r\n");
		}
		rc = tdm_not_locked_alarm();
		if (rc) {
			LOG_PRINTF("Error reading alarm TDM Lock\r\n");
		}
		sem_wait(&i2c_sync); //Semaphore to sync I2C usage
		rc = mcp9844_read_alarms(data);
		if (rc) {
			LOG_PRINTF("Error reading alarm PCB temperature\r\n");
		}
		rc = ina3221_read_alarms(data,0);
		if (rc) {
			LOG_PRINTF("Error reading INA3221 alarms\r\n");
		}
		rc = ina3221_read_alarms(data,1);
		if (rc) {
			LOG_PRINTF("Error reading INA3221 alarms\r\n");
		}
		rc = ina3221_read_alarms(data,2);
		if (rc) {
			LOG_PRINTF("Error reading INA3221 alarms\r\n");
		}
		for (int i = 0; i < SFP_NUM; i++){
			if(sfp_connected[i]){
			rc = sfp_avago_read_alarms(data,i);
				if (rc) {
					// Reset I2C mux to avoid stuck bus
					write_GPIO(I2C_MUX_RESET,1);
					usleep(100);
					write_GPIO(I2C_MUX_RESET,0);
					LOG_PRINTF("Error reading alarm from SFP %d\r\n",i);
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

	LOG_PRINTF("AMS Alarms thread period: %3.4fms\n",((float)periods[0])/1000);
	rc = make_periodic(periods[0], &info);
	if (rc) {
		LOG_PRINTF("Error creating AMS alarm thread\r\n");
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
    			LOG_PRINTF("AMS Voltage file could not be opened!!! \n");/*Any of the files could not be opened*/
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
			LOG_PRINTF("Error\r\n");
		}
		wait_period(&info);
	}
	return NULL;
}

#ifndef DAQ_MODE
/**
 * Periodic thread that is waiting for a command from the DAQ and handling it. Used in nonDAQMode
 *
 * @param arg must be NULL
 *
 * @return  NULL (if exits is because of an error).
 */
static void *command_thread(void *arg){

	struct periodic_info info;
	int rc ;
	struct DPB_I2cSensors *data = static_cast<DPB_I2cSensors *>(arg);

	LOG_PRINTF("Command thread period: %3.4fms\n",((float)periods[3])/1000);
	sem_post(&thread_sync);
	rc = make_periodic(periods[3], &info);
	if (rc) {
		LOG_PRINTF("Error creating command thread\r\n");
		return NULL;
	}
	while(1){
		char aux_buff[256];
		int size;
		char buffer[256];
		char reply[256];
		const char *serialized_json_msg;
		char *reply_bis;
		json_object * jid;
		json_object * jcmd;
		int msg_id;
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
		// Replace spaces by underscores
		for(int i = 0; i < strlen(buffer); i++){
			if(buffer[i] == ' ')
				buffer[i] = '_';
		}
		// Call generic command parse function
		reply_bis = command_parse((const char *)buffer);
		strcpy(reply,reply_bis);
		json_object_put(jmsg);
waitmsg:
	const char* msg_sent = (const char*) reply;
	//FIXME: DAQ Function HERE. Use whole command_thread function as callback function for DAQ library and parse string into DPB command format
	zmq_send(cmd_router,msg_sent, strlen(msg_sent), 0);
	wait_period(&info);
	}

	return NULL;
}

/**
 * Periodic thread that handles configuration updates by retrieving and parsing configuration data.
 * This thread runs continuously at a specified period awaiting for configuration changes coming from the  * ZMQ socket in standalone mode and applies them.
 *
 * @param arg Thread argument (unused, should be NULL)
 *
 * @return NULL (if exits is because of an error).
 */
static void *config_thread(void *arg){

	struct periodic_info info;
	LOG_PRINTF("Configuration thread period: %3.4fms\n",((float)periods[3])/1000);
	int rc = make_periodic(periods[3], &info);
	if (rc) {
		LOG_PRINTF("Error creating configuration thread\r\n");
		return NULL;
	}

	sem_post(&thread_sync);
	while(1){
		config_get();
		int rc = config_parse(config_to_apply);
		if(rc){
			zmq_send(config_router,"Error in reading configuration",strlen("Error in reading configuration"),0);
		}
		else{
			zmq_send(config_router,"Configuration applied",strlen("Configuration applied"),0);
		}
		wait_period(&info);
	}
}
#endif
/** @} */

/************************** Main function ******************************/
/**
 * @brief Main function that initializes signal handlers, creates threads, and manages the main loop.
 * 
 * This function sets up signal handlers for SIGTERM, SIGINT, and SIGSEGV. It initializes a semaphore
 * for thread synchronization and blocks all real-time signals to be used for timers. It then creates
 * several threads for handling AMS alarms, I2C alarms, and monitoring magnitudes. If not running in
 * DAQ mode, it also creates a command thread. The main loop runs indefinitely, checking for a break
 * flag to exit the loop.
 * 
 * @return int Returns 0 upon successful completion.
 */
int main(int argc, char *argv[]){

	setbuf(stdout, NULL);
	sigset_t alarm_sig;
	int i;
	int rc;
	int	n;
	CCOPacket pkt(COPKT_DEFAULT_START, COPKT_DEFAULT_STOP, COPKT_DEFAULT_SEP);

	for(int i = 1 ; i < 6; i++) {
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
				case 5:
				periods[i-1] = HV_LV_SLEEP_DELAY_DEFAULT;
				break;
			}
		else
			periods[i-1] = atoi(argv[i]);
	}
	hv_lv_sleep_delay = periods[4];

	/* Block all real time signals so they can be used for the timers.
	Note: this has to be done in main() before any threads are created
	so they all inherit the same mask. Doing it later is subject to
	race conditions*/

	sigemptyset(&alarm_sig);
	for (i = SIGRTMIN; i <= SIGRTMAX; i++)
		sigaddset(&alarm_sig, i);
	sigprocmask(SIG_BLOCK, &alarm_sig, NULL);

	rc = dpbsc_lib_init(&data);
	if(rc){
		goto end;
	}

	rc = iio_event_monitor_up(); //Initialize iio event monitor
	if (rc) {
		LOG_PRINTF("Error\r\n");
		return rc;
	}

	// Create Signal handlers
	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGSEGV, segmentation_handler);

	sem_init(&thread_sync,0,0);

	pthread_create(&t_1, NULL, ams_alarms_thread,NULL); //Create thread 1 - reads AMS alarms
	sem_wait(&thread_sync);
	pthread_create(&t_2, NULL, i2c_alarms_thread,(void *)&data); //Create thread 2 - reads I2C alarms every x miliseconds
	sem_wait(&thread_sync);
	pthread_create(&t_3, NULL, monitoring_thread,(void *)&data);//Create thread 3 - monitors magnitudes every x seconds
	sem_wait(&thread_sync); //Avoids race conditions
	// Create command thread only if we are not running in DAQ Mode
	#ifndef DAQ_MODE
	pthread_create(&t_4, NULL, command_thread,(void *)&data);//Create thread 4 - waits and attends commands
	sem_wait(&thread_sync); //Avoids race conditions
	pthread_create(&t_5, NULL, config_thread,NULL); //Create thread 5 - waits and attends configuration commands
	#endif

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
