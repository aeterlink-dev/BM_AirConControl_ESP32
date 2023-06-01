#include <BLEDevice.h>
#include <Arduino.h>
#include "typedef.hpp"
#include "to_tate.hpp"

extern AdvertisedData_Typedef advertisedData;
extern std::string rx_buff;

void decode_data() {
// 必要でない限りここはコメントアウトして10usくらい時間短縮
//    advertisedData.ad2.COMPANY_CODE = rx_buff[1] | (rx_buff[0] << 8);
//    advertisedData.ad2.Device_code = rx_buff[2];
//    advertisedData.ad2.HW_version = rx_buff[3];
//    advertisedData.ad2.FW_version = rx_buff[4];
//    advertisedData.ad2.Battery_Monitor = rx_buff[5]; // o 3.6*(x/256)*2する
//    advertisedData.ad2.Error_Flag = rx_buff[6];
//    advertisedData.ad2.Temperature = rx_buff[8] | (rx_buff[7] << 8); // o div
//    advertisedData.ad2.Humidity = rx_buff[10] | (rx_buff[9] << 8); // o div100する必要はある
//    advertisedData.ad2.Illuminance = rx_buff[13] | (rx_buff[12] << 8) | (rx_buff[11] << 16); // o
//    advertisedData.ad2.CO2 = rx_buff[15] | (rx_buff[14] << 8);
    advertisedData.ad2.counter = rx_buff[17] | (rx_buff[16] << 8);
}

/**
 * 未実装です
 */
void print_data() {
    Serial.printf("ad1\n");
    Serial.printf("Length : \n");
    Serial.printf("Flags : \n");
    Serial.printf("Flag_value : \n");
    Serial.printf("ad2\n");
    Serial.printf("Length : \n");
    Serial.printf("spec : \n");
    Serial.printf("COMPANY_CODE : 0x%04X\n", advertisedData.ad2.COMPANY_CODE);
    Serial.printf("Device_code : 0x%02x\n", advertisedData.ad2.Device_code);
    Serial.printf("HW_version : 0x%02x\n", advertisedData.ad2.HW_version);
    Serial.printf("FW_version : 0x%02x\n", advertisedData.ad2.FW_version);
    Serial.printf("Battery_Monitor : %fV\n", 3.6f * 2 * advertisedData.ad2.Battery_Monitor / 256.0f);
    Serial.printf("Error_Flag : %d\n", advertisedData.ad2.Error_Flag);
    Serial.printf("Temperature : %2.2f\n", advertisedData.ad2.Temperature / 100.0f);
    Serial.printf("Humidity : %2.2f%%\n", advertisedData.ad2.Humidity / 100.0f);
    Serial.printf("Illuminance : %dlx\n", advertisedData.ad2.Illuminance);
    Serial.printf("CO2 : %dppm\n", advertisedData.ad2.CO2);
    Serial.printf("Counter : %d\n", advertisedData.ad2.counter);
    Serial.printf("ad3\n");
    Serial.printf("Length : \n");
    Serial.printf("AD_type : \n");
    Serial.printf("Local_name : \n");
}
