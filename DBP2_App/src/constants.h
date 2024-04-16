/************************** Constant Definitions *****************************/
#define INA3221_NUM_CHAN 3
#define AMS_TEMP_NUM_CHAN 3
#define AMS_VOLT_NUM_CHAN 21
/************************** Global Flags Definitions *****************************/
int eth0_flag = 0;
int eth1_flag = 0;
int dig0_main_flag = 0;
int dig1_main_flag = 0;
int dig0_backup_flag = 0;
int dig1_backup_flag = 0;
int break_flag = 0;
/************************** GPIO Pins Definitions *****************************/
#define DIG0_MAIN_AURORA_LINK 40
#define DIG0_BACKUP_AURORA_LINK 41
#define DIG1_MAIN_AURORA_LINK 42
#define DIG1_BACKUP_AURORA_LINK 43
#define SFP0_PWR_ENA 0
#define SFP1_PWR_ENA 1
#define SFP2_PWR_ENA 2
#define SFP3_PWR_ENA 3
#define SFP4_PWR_ENA 4
#define SFP5_PWR_ENA 5
#define SFP0_TX_ENA 6
#define SFP1_TX_ENA 7
#define SFP2_TX_ENA 8
#define SFP3_TX_ENA 9
#define SFP4_TX_ENA 10
#define SFP5_TX_ENA 11
#define SFP0_RX_LOS 14
/******************************************************************************
* Temperature Sensor Register Set - Temperature value, alarm value and alarm flags.
****************************************************************************/
#define MCP9844_TEMP_UPPER_LIM_REG 0x2
#define MCP9844_TEMP_LOWER_LIM_REG 0x3
#define MCP9844_TEMP_CRIT_LIM_REG 0x4
#define MCP9844_TEMP_UPPER_LIM 0x0
#define MCP9844_TEMP_LOWER_LIM 0x1
#define MCP9844_TEMP_CRIT_LIM 0x2
#define MCP9844_TEMP_REG 0x5
/******************************************************************************
* Temperature Sensor Register Set - Configuration and resolution value.
****************************************************************************/
#define MCP9844_RES_REG 0x9
#define MCP9844_CONFIG_REG 0x1
/******************************************************************************
* Temperature Sensor Register Set - Manufacturer and device ID.
****************************************************************************/
#define MCP9844_MANUF_ID_REG 0x6
#define MCP9844_DEVICE_ID_REG 0x7
/******************************************************************************
* SFP Register Set - Real Time Magnitudes.
****************************************************************************/
#define SFP_MSB_TEMP_REG 0x60
#define SFP_LSB_TEMP_REG 0x61
#define SFP_MSB_VCC_REG 0x62
#define SFP_LSB_VCC_REG 0x63
#define SFP_MSB_TXBIAS_REG 0x64
#define SFP_LSB_TXBIAS_REG 0x65
#define SFP_MSB_TXPWR_REG 0x66
#define SFP_LSB_TXPWR_REG 0x67
#define SFP_MSB_RXPWR_REG 0x68
#define SFP_LSB_RXPWR_REG 0x69
/******************************************************************************
* SFP Register Set - Alarms.
****************************************************************************/
#define SFP_MSB_HTEMP_ALARM_REG 0x0
#define SFP_LSB_HTEMP_ALARM_REG 0x1
#define SFP_MSB_LTEMP_ALARM_REG 0x2
#define SFP_LSB_LTEMP_ALARM_REG 0x3
#define SFP_MSB_HVCC_ALARM_REG 0x8
#define SFP_LSB_HVCC_ALARM_REG 0x9
#define SFP_MSB_LVCC_ALARM_REG 0xA
#define SFP_LSB_LVCC_ALARM_REG 0xB
#define SFP_MSB_HTXBIAS_ALARM_REG 0x10
#define SFP_LSB_HTXBIAS_ALARM_REG 0x11
#define SFP_MSB_LTXBIAS_ALARM_REG 0x12
#define SFP_LSB_LTXBIAS_ALARM_REG 0x13
#define SFP_MSB_HTXPWR_ALARM_REG 0x18
#define SFP_LSB_HTXPWR_ALARM_REG 0x19
#define SFP_MSB_LTXPWR_ALARM_REG 0x1A
#define SFP_LSB_LTXPWR_ALARM_REG 0x1B
#define SFP_MSB_HRXPWR_ALARM_REG 0x20
#define SFP_LSB_HRXPWR_ALARM_REG 0x21
#define SFP_MSB_LRXPWR_ALARM_REG 0x22
#define SFP_LSB_LRXPWR_ALARM_REG 0x23
/******************************************************************************
* SFP Register Set - Warnings.
****************************************************************************/
#define SFP_MSB_HTEMP_WARN_REG 0x4
#define SFP_LSB_HTEMP_WARN_REG 0x5
#define SFP_MSB_LTEMP_WARN_REG 0x6
#define SFP_LSB_LTEMP_WARN_REG 0x7
#define SFP_MSB_HVCC_WARN_REG 0xC
#define SFP_LSB_HVCC_WARN_REG 0xD
#define SFP_MSB_LVCC_WARN_REG 0xE
#define SFP_LSB_LVCC_WARN_REG 0xF
#define SFP_MSB_HTXBIAS_WARN_REG 0x14
#define SFP_LSB_HTXBIAS_WARN_REG 0x15
#define SFP_MSB_LTXBIAS_WARN_REG 0x16
#define SFP_LSB_LTXBIAS_WARN_REG 0x17
#define SFP_MSB_HTXPWR_WARN_REG 0xC
#define SFP_LSB_HTXPWR_WARN_REG 0xD
#define SFP_MSB_LTXPWR_WARN_REG 0xE
#define SFP_LSB_LTXPWR_WARN_REG 0xF
#define SFP_MSB_HRXPWR_WARN_REG 0x24
#define SFP_LSB_HRXPWR_WARN_REG 0x25
#define SFP_MSB_LRXPWR_WARN_REG 0x26
#define SFP_LSB_LRXPWR_WARN_REG 0x27
/******************************************************************************
* SFP Register Set - Status and flags.
****************************************************************************/
#define SFP_PHYS_DEV 0x0
#define SFP_FUNCT 0x1
#define SFP_CHECKSUM2_A0 0x40
#define SFP_STAT_REG 0x6E
#define SFP_FLG1_REG 0x70
#define SFP_FLG2_REG 0x71
#define SFP_FLG3_REG 0x74
#define SFP_FLG4_REG 0x75
/******************************************************************************
* Voltage and Current Sensor Register Set - Shunt and bus voltages
****************************************************************************/
#define INA3221_SHUNT_VOLTAGE_1_REG 0x1
#define INA3221_BUS_VOLTAGE_1_REG 0x2
#define INA3221_SHUNT_VOLTAGE_2_REG 0x3
#define INA3221_BUS_VOLTAGE_2_REG 0x4
#define INA3221_SHUNT_VOLTAGE_3_REG 0x5
#define INA3221_BUS_VOLTAGE_3_REG 0x6
/******************************************************************************
* Voltage and Current Sensor Register Set - Warnings and critical alerts
****************************************************************************/
#define INA3221_SHUNT_VOLTAGE_CRIT1_REG 0x7
#define INA3221_SHUNT_VOLTAGE_WARN1_REG 0x8
#define INA3221_SHUNT_VOLTAGE_CRIT2_REG 0x9
#define INA3221_SHUNT_VOLTAGE_WARN2_REG 0xA
#define INA3221_SHUNT_VOLTAGE_CRIT3_REG 0xB
#define INA3221_SHUNT_VOLTAGE_WARN3_REG 0xC
#define INA3221_CH1 0x0
#define INA3221_CH2 0x1
#define INA3221_CH3 0x2
/******************************************************************************
* Voltage and Current Sensor Register Set - Device Configuration, alert status configuration and enabling
****************************************************************************/
#define INA3221_CONFIG_REG 0x0
#define INA3221_MASK_ENA_REG 0xF
/******************************************************************************
* Voltage and Current Sensor Register Set - Manufacturer and device ID.
****************************************************************************/
#define INA3221_MANUF_ID_REG 0xFE
#define INA3221_DIE_ID_REG 0xFF
/******************************************************************************
*SFP set.
****************************************************************************/
#define DEV_SFP0 0x0
#define DEV_SFP1 0x1
#define DEV_SFP2 0x2
#define DEV_SFP3 0x3
#define DEV_SFP4 0x4
#define DEV_SFP5 0x5
/******************************************************************************
*INA3221 set.
****************************************************************************/
#define DEV_SFP0_2_VOLT 0x0
#define DEV_SFP3_5_VOLT 0x1
#define DEV_SOM_VOLT 0x2
/******************************************************************************
*Threads timers (ms).
****************************************************************************/
#define MONIT_THREAD_PERIOD 5000000
#define ALARMS_THREAD_PERIOD 100
#define AMS_ALARMS_THREAD_PERIOD 100
#define COMMAND_THREAD_PERIOD 50
/******************************************************************************
*GPIO base address
****************************************************************************/
int GPIO_BASE_ADDRESS = 0;
/******************************************************************************
*Shared Memory.
****************************************************************************/
#define MEMORY_KEY 7890

struct wrapper
{
    char ev_type[8];
    char ch_type[16];
    int chn;
    __s64 tmpstmp;
    sem_t empty;
    sem_t full;
    sem_t ams_sync;
};
int memoryID;
struct wrapper *memory;

//static int monitoring_thread_count;

/******************************************************************************
*AMS channel descrpitor.
****************************************************************************/
char *ams_channels[] = {
        "PS LPD Temperature",
        "PS FPD Temperature",
        "VCC PS LPD voltage",
        "VCC PS FPD voltage",
        "PS Aux voltage reference",
        "DDR I/O VCC voltage",
        "PS IO Bank 503 voltage",
        "PS IO Bank 500 voltage",
        "VCCO_PSIO1 voltage",
        "VCCO_PSIO2 voltage",
        "VCC_PS_GTR voltage",
        "VTT_PS_GTR voltage",
        "VCC_PSADC voltage",
        "PL Temperature",
        "PL Internal voltage",
        "PL Auxiliary voltage",
        "ADC Reference P+ voltage",
        "ADC Reference N- voltage",
        "PL Block RAM voltage",
        "LPD Internal voltage",
        "FPD Internal voltage",
        "PS Auxiliary voltage",
        "PL VCCADC voltage"
    };
