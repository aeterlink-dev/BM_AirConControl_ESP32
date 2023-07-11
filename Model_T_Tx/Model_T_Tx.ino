#include <BLEDevice.h>
#include <BLEServer.h>
//#include <M5StickCPlus.h>        // Deep sleep用
/* 基本属性定義  */
#define DEVICE_NAME "APBM05"         // デバイス名
#define SPI_SPEED   115200          // SPI通信速度
constexpr int serial_no = 1;
constexpr char serial_no_0_to_7 = (serial_no >> 8) & 0xFF;
constexpr char serial_no_8_to_15 = serial_no & 0xFF;

RTC_DATA_ATTR static uint16_t seq_number = 1;    // RTCメモリー上のシーケンス番号

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


uint8_t tx_buff[18] = {};

constexpr uint32_t TX_BUF_SIZE = sizeof(AdvertisedData_Typedef);
AdvertisedData_Typedef advertisedData = {};
int start_time = 0;

void encode_data() {
//    tx_buff[0] = advertisedData.ad1.Length;
//    tx_buff[1] = advertisedData.ad1.Flags;
//    tx_buff[2] = advertisedData.ad1.Flag_value;
//    tx_buff[3] = advertisedData.ad2.Length;
//    tx_buff[4] = advertisedData.ad2.spec;
//    tx_buff[5] = advertisedData.ad2.COMPANY_CODE | 0xFFU;
//    tx_buff[6] = (advertisedData.ad2.COMPANY_CODE | 0xFF00U) >> 8;
//    tx_buff[7] = advertisedData.ad2.Device_code;
//    tx_buff[8] = advertisedData.ad2.HW_version;
//    tx_buff[9] = advertisedData.ad2.FW_version;
//    tx_buff[10] = advertisedData.ad2.Battery_Monitor;
//    tx_buff[11] = advertisedData.ad2.Error_Flag;
//    tx_buff[12] = advertisedData.ad2.Temperature | 0xFFU;
//    tx_buff[13] = (advertisedData.ad2.Temperature | 0xFF00U) >> 8;
//    tx_buff[14] = advertisedData.ad2.Humidity | 0xFFU;
//    tx_buff[15] = (advertisedData.ad2.Humidity | 0xFF00U) >> 8;
//    tx_buff[16] = advertisedData.ad2.Illuminance | 0xFFU;
//    tx_buff[17] = (advertisedData.ad2.Illuminance | 0xFF00U) >> 8;
//    tx_buff[18] = (advertisedData.ad2.Illuminance | 0xFF0000U) >> 16;
//    tx_buff[19] = advertisedData.ad2.CO2 | 0xFFU;
//    tx_buff[20] = advertisedData.ad2.CO2 | 0xFF00U;
//    tx_buff[21] = けしたらもったいないので、のこす


}
 
void setAdvertisementData(BLEAdvertising* pAdvertising) { 
    // デバイス名とフラグをセットし、送信情報を組み込んでアドバタイズオブジェクトに設定する
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

    // number1->2->3の順にsetするはずだが...
    // 31byteを超えると結合されないらしい。
    // AD structure number1 (3bytesらしい)
    oAdvertisementData.setFlags(0x06); // 1かも
    // Data lengthとFlag valueの設定はいるか？

    // AD structure number2 (21bytes)
    // string領域に送信情報を連結する
    std::string strData = "";
    strData += (char)(0xFF);                  // Manufacturer specific data (5バイト目)
    strData += (char)(0x0C); // manufacturer(AETERLINK) ID low byte (6)
    strData += (char)(0x0F); // manufacturer(AETERLINK) ID high byte (7)
    strData += (char)(0x01);         // model code (8)
    strData += serial_no_0_to_7;     // Serial No.[0] (9)
    strData += serial_no_8_to_15;    // Serial No.[1] (10)
    strData += (char)(0x00);         // HW version (11)
    strData += (char)(0x00);         // FW version (12)
    strData += (char)(0x00);         // Detect Power (13)
    strData += (char)(0x00);         // Battery Monotor (14)
    strData += (char)(0x00);         // Error Flag (15)
    strData += (char)(0x00);         // Sensor_Data_0 (16)
    strData += (char)(0x00);         // Sensor_Data_1 (17) 
    strData += (char)(0x00);         // Sensor_Data_2 (18) 
    strData += (char)(0x00);         // Sensor_Data_3 (19) 
    strData += (char)(0x00);         // Sensor_Data_4 (20) 
    strData += (char)(0x00);         // Sensor_Data_5 (21) 
    strData += (char)((seq_number & 0xFF00) >> 8);   // counter上8bit (22)
    strData += (char)(seq_number & 0xFF);       // counter下8bit (23)
    strData += (char)(0x00);         // Data_Priority (24) 
    strData = (char)strData.length() + strData; // 先頭にLengthを設定 (4)
    oAdvertisementData.addData(strData);
    
    // AD structure number3 ((2bytes?)"APBM05"とかだと8bytesで、計32bytesになってしまう。。。)
    // そのため、一度消す。送るバイトは24bytes
    // oAdvertisementData.setName(DEVICE_NAME);
    // Data lengthとLocalNameの設定はいるか？

    // send
    std::string payload = oAdvertisementData.getPayload();
    for (int i = 0; i < payload.length(); ++i) {
        Serial.printf("%02X ", payload[i]);
    }
    Serial.println();
    pAdvertising->setAdvertisementData(oAdvertisementData);
}

 

void setup() {
    Serial.begin(SPI_SPEED);
    pinMode(10, OUTPUT); // 10か27
    digitalWrite(10, LOW);
    delay(100);
    digitalWrite(10, HIGH);
    delay(100);
    digitalWrite(10, LOW);
    delay(100);
    digitalWrite(10, HIGH);
    delay(100);
    setCpuFrequencyMhz(80);
    Serial.println(getCpuFrequencyMhz());
    BLEDevice::init(DEVICE_NAME);             // BLEデバイスを初期化する 
    // BLEサーバーを作成してアドバタイズオブジェクトを取得する
    BLEServer *pServer = BLEDevice::createServer();
    BLEAdvertising *pAdvertising = pServer->getAdvertising();

    Serial.printf("Hello I am %s.\n", DEVICE_NAME);

    // 送信情報を設定してシーケンス番号をインクリメントする
    while (1) {
        start_time = millis();
        setAdvertisementData(pAdvertising);
        pAdvertising->start();
        digitalWrite(10, LOW);
        delay(1);
        Serial.printf("#%d\n", seq_number);
        digitalWrite(10, HIGH);  
        delay(180); // 190ms送信すれば13時間あまり持つ
        pAdvertising->stop();
        if (seq_number == 0xFFFF) seq_number = 1;
        else seq_number++;
        esp_sleep_enable_timer_wakeup((2500 - (millis() - start_time)) * 1000);
        esp_light_sleep_start();
    }
};
 
void loop() {
}
