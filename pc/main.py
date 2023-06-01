import serial
import re
import time
import numpy as np
COM_RX = "COM8" # Model-T -> ESP32 -> PC のCOM。本当にRx_only
COM_TX = "COM5" # PC -> ESP32 -> SwitchBot のCOM。本当は
known_id = [1, 2, 3] # todo: 関数で管理。今はif xxx in xxxで代用している
VALID_TEXT = "9999"
bitrate = 115200

ser_modelt = serial.Serial(COM_RX, bitrate, timeout=2)
print("Serial for Rx-from-model-T found.")
ser_switchbot = serial.Serial(COM_TX, bitrate, timeout=2)
print("Serial for Tx-to-SwitchBot found.")

rx_data = np.array([[0, 0, 0]])

def getTemperatureCommand(data):
    # 本来ここにはdataを使ったアルゴリズムを使うが、ここでは配列の個数からランダムに温度を設定する
    t = data.shape[0] % 7 - 3
    # data.clear()
    return ('+' if t >= 0 else '-') + f'{abs(t)}'

while True:
    start_time = time.time()
    while time.time() - start_time < 30:
    # RX_from ser_modelt
        string_tmp = str(ser_modelt.readline())
        result = re.findall(r"\d+", string_tmp)
        if ((len(result) >= 5) and result[0] == VALID_TEXT):
            sensor_id = int(result[2])
            sensor_count = int(result[3])
            sensor_data = int(result[4]) # とりあえずデータという汎用的な名前とする
            if not sensor_id in known_id:
                continue
            print(f'Sensor{sensor_id} says {sensor_count}th data is {sensor_data}. Data_len = {rx_data.shape[0]}')
            rx_data = np.append(rx_data, [[sensor_id, sensor_count, sensor_data]], axis=0)
        delta_t = getTemperatureCommand(rx_data)


    print("TX start!!!!")
    # TX
    text = getTemperatureCommand(rx_data)
    print(text)

    if text != '+0':
        print('We should change temperature by ' + delta_t + ' degree')
        text_bin = text.encode('utf-8')
        ser_switchbot.write(text_bin)

    time.sleep(1)
    print(ser_switchbot.read_all().decode('utf-8'))
