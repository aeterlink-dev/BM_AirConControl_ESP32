#include <BLEDevice.h>
#include <BLEServer.h>
/* 基本属性定義  */
#define SPI_SPEED   115200          // SPI通信速度
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

static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEClient*  pClient = NULL;
static BLEAdvertisedDevice target_plus;
static BLEAdvertisedDevice target_minus;
static bool doSendCommand = false;
static bool plus_found = false;
static bool minus_found = false;

constexpr int RX_BUFF_SIZE = 1000;
int start_time = 0;
 
void dbg(const char *format, ...) {
  char b[512];
  va_list va;
  va_start(va, format);
  vsnprintf(b, sizeof(b), format, va);
  va_end(va);
  Serial.print(b);
}

// アドバタイズ検出時のコールバック
class advdCallback: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    dbg("BLE device found: ");
    String addr = advertisedDevice.getAddress().toString().c_str();
    dbg("addr=[%s]\n", addr.c_str());
    // SwitchBot を発見
    if (addr.equalsIgnoreCase(addrSwitchBot_plus)) {
      dbg("found plus\n");
      target_plus = advertisedDevice;
      plus_found = true;
    }
    if (addr.equalsIgnoreCase(addrSwitchBot_minus)) {
      dbg("found minus\n");
      minus_found = true;
      target_minus = advertisedDevice;
    }

    if (plus_found && minus_found) {
      advertisedDevice.getScan()->stop();
      doSendCommand = true;
    }
  }
};

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  Serial.printf("Notify callback for characteristic ");
  Serial.printf("%s", pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.printf(" of data length %d\n", length);
  Serial.print("data: ");
  for (int i = 0; i <= length - 1; i++) {
    Serial.printf("%02X ", *(pData + i));
  }
  Serial.println("");
}

// SwitchBot の GATT サーバへ接続 ～ Press コマンド送信
static bool connectAndSendCommand(BLEAdvertisedDevice t, int times) {
  dbg("start connectAndSendCommand\n");
  pClient  = BLEDevice::createClient();

  pClient->connect(&t);
  dbg("connected\n");

  // 対象サービスを得る
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    dbg("target service not found\n");
    return false;
  }
  dbg("found target service\n");

  // 対象キャラクタリスティックを得る
  pRemoteCharacteristic = pRemoteService->getCharacteristic(notifyUUID);
  pRemoteCharacteristic->registerForNotify(notifyCallback);

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    dbg("target characteristic not found\n");
    return false;
  }
  dbg("found target characteristic\n");

  // キャラクタリスティックに Press コマンドを書き込む
  for (int i = 0; i < times; ++i) {
    pRemoteCharacteristic->writeValue(cmdPress, sizeof(cmdPress), false);
    delay(3000);
  }
  if (pClient) {
    pClient->disconnect();
    pClient = NULL;
  }
  return true;
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

    if (Serial.available()) {       // 受信データがあるか？
        Serial.printf("Loop start\n");
        key = Serial.read();            // 1文字だけ読み込む
        if (key == '+' || key == '-' ) {
            while (!Serial.available()) {
                Serial.printf("Waiting serial\n");
                delay(1000);
            }
            temp_diff = Serial.read();
            if (temp_diff >= '1' && temp_diff <= '9') {
                Serial.printf("%c%c\n", key, temp_diff);
                  if (doSendCommand == true) {
                      if (connectAndSendCommand(key == '+' ? target_plus : target_minus, temp_diff - '0' + 1)) {
                          dbg("connectAndSendCommand succeded\n");
                      } else {
                          dbg("connectAndSendCommand failed\n");
                      }
                      // doSendCommand = false;
                      dbg("done\n");
                  }
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