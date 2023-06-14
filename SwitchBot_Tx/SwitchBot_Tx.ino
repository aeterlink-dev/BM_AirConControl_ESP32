#include <BLEDevice.h>
#include <BLEServer.h>
/* 基本属性定義  */
constexpr int SPI_SPEED = 115200;      // SPI通信速度
// 手元の SwitchBot のアドレス
static String addrSwitchBot_plus = "c7:4c:61:9f:32:69";
static String addrSwitchBot_minus = "d0:c9:7b:58:81:88";
// SwitchBot のユーザ定義サービス
static BLEUUID serviceUUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
// 上記サービス内の対象キャラクタリスティック
static BLEUUID charUUID("cba20002-224d-11e6-9fb8-0002a5d5c51b");
static BLEUUID notifyUUID("cba20003-224d-11e6-9fb8-0002a5d5c51b");
// SwitchBot の Press コマンド
static uint8_t cmdPress[3] = {0x57, 0x01, 0x00};

static BLEAdvertisedDevice target_plus;
static BLEAdvertisedDevice target_minus;
static bool canSendCommand = false;
static bool plus_found = false;
static bool minus_found = false;

typedef enum {
  REMOTESERVICE_NOT_FOUND = 1,
  REMOTECHARACTERISTIC_NOT_FOUND = 2,
};

constexpr int RX_BUFF_SIZE = 1000;
int start_time = 0;

// アドバタイズ検出時のコールバック
class advdCallback : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.printf("BLE device found: ");
    String addr = advertisedDevice.getAddress().toString().c_str();
    Serial.printf("addr=[%s]\n", addr.c_str());
    // SwitchBot を発見
    if (addr.equalsIgnoreCase(addrSwitchBot_plus)) {
      Serial.printf("found plus\n");
      target_plus = advertisedDevice;
      plus_found = true;
    }
    if (addr.equalsIgnoreCase(addrSwitchBot_minus)) {
      Serial.printf("found minus\n");
      minus_found = true;
      target_minus = advertisedDevice;
    }

    if (plus_found && minus_found) {
      advertisedDevice.getScan()->stop();
      canSendCommand = true;
    }
  }
};

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  Serial.printf("Notify callback for characteristic ");
  // Serial.printf("%s", pBLERemoteCharacteristic->getUUID().toString().c_str());
  // Serial.printf(" of data length %d\n", length);
  // Serial.print("data: ");
  // for (int i = 0; i <= length - 1; i++) {
  //   Serial.printf("%02X ", *(pData + i));
  // }
  // Serial.println("");
}

static void print_result(int result) {
  if (result == 0) {
    Serial.printf("connectAndSendCommand succeded\n");
  } else if (result == 1) {
    Serial.printf("REMOTESERVICE_NOT_FOUND\n");
  } else if (result == 2) {
    Serial.printf("REMOTECHARACTERISTIC_NOT_FOUND\n");
  } else {
    Serial.printf("error #%d\n", result);
  }
}

// SwitchBot の GATT サーバへ接続 ～ Press コマンド送信
static int connectAndSendCommand(BLEAdvertisedDevice t, int times, int delay_ms) {
  static BLEClient*  pClient = BLEDevice::createClient();
  static BLERemoteCharacteristic* pRemoteCharacteristic;

  Serial.println("loop start");
  if (!pClient->isConnected()) {
    if (!pClient->connect(&t)) {
      return REMOTESERVICE_NOT_FOUND;
    } 

    // 対象サービスを得る
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      return REMOTESERVICE_NOT_FOUND;
    }
    // 対象キャラクタリスティックを得る
    pRemoteCharacteristic = pRemoteService->getCharacteristic(notifyUUID);
    pRemoteCharacteristic->registerForNotify(notifyCallback);

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      return REMOTECHARACTERISTIC_NOT_FOUND;
    }
  }
  // キャラクタリスティックに Press コマンドを書き込む
  for (int i = 0; i < times; ++i) {
    pRemoteCharacteristic->writeValue(cmdPress, sizeof(cmdPress), false);
    delay(delay_ms);
  }
  // disconnect
  // if (pClient) {
  //   pClient->disconnect();
  //   pClient = NULL;
  // }
  return 0;
}

void setup() {
    Serial.begin(SPI_SPEED);
    Serial.println("Hello I am SwitchBot controller.");

    // BLE 初期化
    BLEDevice::init("");
    // デバイスからのアドバタイズをスキャン
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new advdCallback());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(30); 
}

void loop() {
    static char key;     // 受信データを格納するchar型の変数
    static char temp_diff;
    static int result;

    if (Serial.available()) {       // 受信データがあるか？
        Serial.printf("Loop start\n");
        key = Serial.read();            // 1文字だけ読み込む
        if (key == '+' || key == '-' ) {
            while (!Serial.available()) {
                Serial.printf("Waiting serial\n");
                delay(1000);
            }
            temp_diff = Serial.read();
            Serial.printf("%c%c\n", key, temp_diff);
            if (temp_diff >= '1' && temp_diff <= '9') {
                if (canSendCommand == true) {
                    // print_result(connectAndSendCommand(target_plus, 1)); // 空打ち
                    print_result(connectAndSendCommand(key == '+' ? target_plus : target_minus, temp_diff - '0', 3000));
                }
            } else if (temp_diff == '0') {
                print_result(connectAndSendCommand(target_minus, 1, 5000));
                // print_result(connectAndSendCommand(target_minus, 1, 5000));
            } else {
                Serial.printf("temp_diff is too large: %d\n", temp_diff);
            }
        } else {
            Serial.printf("I received %c (int: %d)\n", key, key);
        }

        while (Serial.available()) {
            key = Serial.read();
            Serial.printf("I received %c (int: %d)\n", key, key);
        }
        Serial.printf("Loop end\n\n");
    }
}