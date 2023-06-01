#include <BLEDevice.h>
#include <esp_now.h>
#include <WiFi.h>
#include "typedef.hpp"
#include "to_tate.hpp"


/* グローバル変数定義  */
uint8_t slaveAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // broadcast
// uint16_t prev_seq[BEACON_MAX_NUM] = {};
std::string rx_buff;
std::string rx_name;

volatile bool is_found[TX_PACKET_LENGTH] = {false, false, false, false, false, false, false, false};
volatile Beacon_Typedef beacon[TX_PACKET_LENGTH];
volatile int out_of_range = false;
volatile int start_time, stop_time;

AdvertisedData_Typedef advertisedData = {};


// アドバタイジング受信時コールバック
// 150回くらい実行して、基本50usから60usで、最大87us
class advertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        start_time = micros();
        rx_buff = advertisedDevice.getManufacturerData();

        if (rx_buff.length() == 19 && rx_buff[0] == 0x0C && rx_buff[1] == 0x0F && rx_buff[2] == 0x01) { // この長さはTxで送るバイトとは異なる
            int n = (rx_buff[4] << 8) | rx_buff[3]; // 65536のAPBMを管理できない
            // if (n >= BEACON_MAX_NUM) {
            //     out_of_range = n;
            //     return;
            // }
            decode_data();
            // if (prev_seq[n] != advertisedData.ad2.counter) { // 同じデータは2回来ないことが予想される
                // prev_seq[n] = advertisedData.ad2.counter;
                for (int i = 0; i < TX_PACKET_LENGTH; ++i) {
                    if (!(is_found[i])) {
                        beacon[i].beacon_id = n;
                        beacon[i].count = advertisedData.ad2.counter;
                        beacon[i].rssi = advertisedDevice.getRSSI();
                        stop_time = micros();
                        is_found[i] = true;
                        break;
                    }
                }
            // }
        }
    }
};

void ble_main(void* a) {
    while (1) {
        BLEDevice::getScan()->start(1);
        BLEDevice::getScan()->stop();
    } 
}

/// @brief SPI115200だと110usから5msくらいかかる
/// @param a 
void wifi_main(void* a) {
    static int num_valid = 0;
    static volatile Beacon_Typedef* b;
    static uint8_t* ptr;
    static uint8_t tx_buff[100]; // [0]にデータ長、[1]にserver_id、それ以降にbeaconのデータをいれる
    
    while (1) {
        num_valid = 0;
        ptr = tx_buff + TOP_DATA_LENGTH;
        for (int i = 0; i < TX_PACKET_LENGTH; ++i) {
            if (is_found[i]) {
                ++num_valid;
                b = &(beacon[i]);
                Serial.printf("9999: packet(%d) APBM %d #%d %ddBm %dus\n", i,  b->beacon_id, b->count, (int8_t)(b->rssi), stop_time - start_time);
                memcpy(ptr, (uint8_t *)b, BEACON_SIZE);
                ptr += BEACON_SIZE;
                is_found[i] = false;
            }
        }
        if (num_valid) {
            tx_buff[DATA_LENGTH_ID] = TOP_DATA_LENGTH + BEACON_SIZE * num_valid;
            tx_buff[SERVERID_ID] = SERVER_NUMBER;
            // not use wifi
            // delay(300 + 10 * SERVER_NUMBER);
            // print_esp_now_send_result(esp_now_send(slaveAddress, (uint8_t *)tx_buff, 2 + BEACON_SIZE * num_valid));
        }               
        if (out_of_range) {
            Serial.printf("APBM %d is out of range!!!\n", out_of_range);
            out_of_range = 0;
        }
    } 
}

/*****************************************************************************
 *                          Predetermined Sequence                           *
 *****************************************************************************/
void setup() {
    // 初期化処理を行ってBLEデバイスを初期化する
    Serial.begin(SPI_SPEED);
    Serial.printf("\nI am server%d.\n", SERVER_NUMBER);
    Serial.printf("Client application start...\n");

    // ble
    // for (int i = 0; i < BEACON_MAX_NUM; ++i) {
    //     prev_seq[i] = -1;
    // }

    // Scanオブジェクトを取得してコールバックを設定する
    Serial.println("BLE Client start ...");
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new advertisedDeviceCallbacks());
    pBLEScan->setActiveScan(false);

    // not use wifi
    // WiFi.mode(WIFI_STA);
    // WiFi.disconnect();
    // while (esp_now_init() != ESP_OK) {
    //     Serial.println("ESPNow Init");
    // }
    // esp_now_peer_info_t peerInfo={};
    // memcpy(peerInfo.peer_addr, slaveAddress, 6);
    // peerInfo.channel = 0;
    // peerInfo.encrypt = false;
    // if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    //     Serial.println("Failed to add peer");
    //     return;
    // }

    Serial.printf("Initialized successfully.\n");

    xTaskCreatePinnedToCore(ble_main, "ble_main", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(wifi_main, "wifi_main", 8192, NULL, 1, NULL, 1);
}

void loop() {}
