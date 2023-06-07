import serial
import re
import time
import numpy as np
import sys
COM_RX = "/dev/ttyUSB1" # Model-T -> ESP32 -> PC のCOM。本当にRx_only
COM_TX = "/dev/ttyUSB0" # PC -> ESP32 -> SwitchBot のCOM。本当はTxもRxもある
# COM_RX = "COM3" # Model-T -> ESP32 -> PC のCOM。本当にRx_only
# COM_TX = "COM4" # PC -> ESP32 -> SwitchBot のCOM。本当はTxもRxもある

known_id = {'1' : 0, '22' : 1, '32' : 2, '42' : 3} # todo: 関数で管理。今はif xxx in xxxで代用している -> 辞書式で解決
VALID_TEXT = "9999"
RX_TEXT_LENGTH = 5
bitrate = 115200
current_temperature = 25
temperature_bool = 0
state = 0

def getTargetTemperature(data, t_now):
    # return t_now + data.shape[0] % 5 - 2 
    global temperature_bool
    temperature_bool = (temperature_bool + 1) % 3
    return t_now + (temperature_bool - 1)

def getCommand(t_now, t_target):
    t = t_target - t_now
    return ('+' if t >= 0 else '-') + f'{abs(t)}'

if __name__ == '__main__':
    args = sys.argv
    if 2 <= len(args) and args[1].isdigit():
        current_temperature = int(args[1])
        print('current temperature is ' + args[1])
    else:
        print('current temperature is not defined.')
        exit()
    
    ser_modelt = serial.Serial(COM_RX, bitrate, timeout=2)
    print("Serial for Rx-from-model-T found.")
    ser_switchbot = serial.Serial(COM_TX, bitrate, timeout=2)
    print("Serial for Tx-to-SwitchBot found.")

    save_data = np.array([[0, 0, 0, 0, 0]]) # id1, id2, id3, id4, 制御値
    count = 0

    # 温度の差
    # logを終わらせる
    # list
    try:
        while True:
            start_time = time.time()
            while time.time() - start_time < 20:
            # RX_from ser_modelt
                string_tmp = str(ser_modelt.readline())
                result = re.findall(r"\d+", string_tmp)
                if ((len(result) >= RX_TEXT_LENGTH) and result[0] == VALID_TEXT):
                    sensor_id = result[2]
                    sensor_count = int(result[3])
                    sensor_data = int(result[4]) # とりあえずデータという汎用的な名前とする
                    if not sensor_id in known_id:
                        continue
                    # print(f'Sensor{sensor_id} says {sensor_count}th data is {sensor_data}. save_data =\n {save_data}')
                    save_data[count, known_id[sensor_id]] = sensor_data
                    print('.', end='', flush=True)

            print("TX start!!!!")
            # TX
            if state == 0:
                t_target = getTargetTemperature(save_data, current_temperature)
                text = getCommand(current_temperature, t_target)
                print(current_temperature, '->', t_target)
                current_temperature = t_target


                print('We should change temperature by ' + text + ' degree')
                text_bin = text.encode('utf-8')
            else:
                print('send +0')
                text_bin = '+0'.encode('utf-8')

            ser_switchbot.write(text_bin)
            time.sleep(1)
            print(ser_switchbot.read_all().decode('utf-8'))

            save_data = np.append(save_data, [[0, 0, 0, 0, 0]], axis=0)
            count += 1
            state = (state + 1) % 3
    except KeyboardInterrupt:
        np.save('log', save_data)