#include "constant.hpp"


typedef struct 
{
    uint8_t Length;
    uint8_t Flags;
    uint8_t Flag_value;
} AD_Structure1;

typedef struct
{
    uint8_t Length;
    uint8_t spec;
    uint16_t COMPANY_CODE;
    uint8_t Device_code;
    uint8_t HW_version;
    uint8_t FW_version;
    uint8_t Battery_Monitor;
    uint8_t Error_Flag;
    uint16_t Temperature;
    uint16_t Humidity;
    uint32_t Illuminance; // 本当は24bit
    uint16_t CO2;
    uint16_t counter;
} AD_Structure2;

typedef struct
{
    uint8_t Length;
    uint8_t AD_type;
    uint8_t Local_Name[6];
} AD_Structure3;

typedef struct 
{
    AD_Structure1 ad1;
    AD_Structure2 ad2;
    AD_Structure3 ad3;
} AdvertisedData_Typedef;

typedef struct
{
    uint16_t beacon_id;
    uint16_t count;
    uint8_t rssi;
} Beacon_Typedef;

constexpr uint32_t BEACON_SIZE = sizeof(Beacon_Typedef);
