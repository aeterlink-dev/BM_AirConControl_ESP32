import sys
import keyboard
import serial
import time 
import csv

import datetime

# port assignments
PORT_SER1 = "COM12"
PORT_SER2 = "COM14"
PORT_SER3 = "COM13"

# PTxMP Assignments
PTXMP_SID_LIST_SER1 = [0xc0, 0xc1, 0xc2, 0xc3, 0xc4]
PTXMP_SID_LIST_SER2 = [0xc5, 0xc6, 0xc7, 0xc8, 0xc9]
PTXMP_SID_LIST_SER3 = [0xca, 0xcb, 0xcc, 0xcd, 0xd0]

PTXMP_DICT = {0xc0:0, 0xc1:1, 0xc2:2, 0xc3:3, 0xc4:4,
              0xc5:5, 0xc6:6, 0xc7:7, 0xc8:8, 0xc9:9,
              0xca:10, 0xcb:11, 0xcc:12, 0xcd:13, 0xd0:14}
# Commands
PTX_PWR_ON = 0x10
PTX_PWR_OFF = 0x11
PTX_PWR_STAT_REQ = 0x12
BM_GET_DATA_REQ = 0xa1
NTWK_SET_SLAVE_ID = 0xf0
NTWK_PING = 0xf1
NTWK_CLEAR_SLAVE_ID = 0xf2

# RSSI Log
RSSI_LOG_INDEX = ["Sensor ID", "PTxMP ID", "Time", "Counter", "RSSI"]

class ResponseTimeout(Exception):
    pass

def compute_crc(data):
    crc_register = 0xFFFF
    for data_byte in data:
        tmp = crc_register ^ data_byte
        shift_num = 0
        while(shift_num < 8):
            shift_num += 1
            if(tmp&1 == 1):
                tmp = tmp >> 1
                tmp = 0xA001 ^ tmp
            else:
                tmp = tmp >> 1
        crc_register = tmp
    crc = crc_register.to_bytes(2, 'little')
    return crc

def sendAndReceive(ser, slave_id, function_code, payload=[]):

    data = [slave_id, function_code, len(payload)] + payload
    data += compute_crc(data)
    req = bytearray(data)

    #print("SEND: ", req)

    ser.reset_input_buffer()
    ser.write(req)
    ser.flush()

    resp = bytearray(ser.read(len(req)))
    if resp != req:
        #raise Exception("Echoback is not correct")
        print("Ecoback is not correct")
        return

    if slave_id == 0x00:
        return None

    header = bytearray(ser.read(3))
    if len(header) != 3:
        #raise ResponseTimeout("Header Timeout")
        print("ResponseTimeout(Header Timeout)", len(header))
        return
    
    slave_id = header[0]
    function_code = header[1]
    length = header[2]

    body = bytearray(ser.read(length+3))
        
    if len(body) != length+3:
        #raise ResponseTimeout("Payload Timeout")
        print("ResponseTimeout(Payload Timeout)")
        return

    payload = body[:length]
    status_code = body[length]
    crc = body[length+1:]

    #print (crc)
    #print(compute_crc(header + body[:length+1]))

    #if crc != bytearray(compute_crc(header + body[:length+1])):
    #    raise ResponseTimeout("Invalid CRC")

    #print("RECV: ", slave_id, function_code, length, payload, status_code)

    #return list(resp)
    return list(payload)


def string_to_array(s):
    return list(bytearray(s.encode("ascii")))

def main():

    ser1 = serial.Serial(PORT_SER1, 1000000, timeout=2)
    ser2 = serial.Serial(PORT_SER2, 1000000, timeout=2)        
    ser3 = serial.Serial(PORT_SER3, 1000000, timeout=2)

    sers = [ser1, ser2, ser3]
    ptxmp_sids = [PTXMP_SID_LIST_SER1, PTXMP_SID_LIST_SER2, PTXMP_SID_LIST_SER3]

    for id in range(len(sers)):
            for slave_id in ptxmp_sids[id]:
               ret = sendAndReceive(sers[id], slave_id, PTX_PWR_ON)            
            time.sleep(2.0) 

    while True:
        for id in range(len(sers)):
            for slave_id in ptxmp_sids[id]:
                ret = sendAndReceive(sers[id], slave_id, BM_GET_DATA_REQ)            
            time.sleep(0.2) 
    
    for id in range(len(sers)):
        sers[id].close()

class SerialCom:
    def __init__(self):
        ser1 = serial.Serial(PORT_SER1, 1000000, timeout=2)
        ser2 = serial.Serial(PORT_SER2, 1000000, timeout=2)        
        ser3 = serial.Serial(PORT_SER3, 1000000, timeout=2)

        self.sers = [ser1, ser2, ser3]
        self.ptxmp_sids = [PTXMP_SID_LIST_SER1, PTXMP_SID_LIST_SER2, PTXMP_SID_LIST_SER3]

    def ptxmp_power_on(self):
        for id in range(len(self.sers)):
            for slave_id in self.ptxmp_sids[id]:
                ret = sendAndReceive(self.sers[id], slave_id, PTX_PWR_ON)            

    def rssi_monitor(self, time_now):
        data_array = []
        for id in range(len(self.sers)):
            for slave_id in self.ptxmp_sids[id]:
                payload = sendAndReceive(self.sers[id], slave_id, BM_GET_DATA_REQ)
                if len(payload) >= 25:
                    sensor_id = payload[8] * 256 + payload[9]
                    rssi = payload[24]
                    counter = payload[21] * 256 + payload[22]
                    data = [sensor_id, PTXMP_DICT[slave_id], "{}:{}:{}".format(time_now.hour, time_now.minute, time_now.second), counter, rssi]
                    print(data)
                    data_array.append(data)

        return data_array

    def __del__(self):
        for id in range(len(self.sers)):
            self.sers[id].close()

class RssiLog:
    def __init__(self, out):
        self.out_csv = open(out + ".csv", "w", newline="")
        self.write_data(RSSI_LOG_INDEX)

    def write_data(self, data):
        csv.writer(self.out_csv).writerow(data)

    def write_data_array(self, data_array):
        writer = csv.writer(self.out_csv)
        for array in data_array:
            writer.writerow(array)

    def __del__(self):
        self.out_csv.close()

if __name__ == "__main__":

    args = sys.argv

    if len(args) != 2:
        if len(args) == 1:
            print("Usage: Rssi_logger.py [output file name]")
        else:
            print("Argument is not appropriate")
    else:
        ser_com = SerialCom()
        rssi_log = RssiLog(args[1])

        ser_com.ptxmp_power_on()

        while True:
            rssi_log.write_data_array(ser_com.rssi_monitor(datetime.datetime.now()))

            if keyboard.is_pressed("q"):
                print("quit")
                sys.exit()
