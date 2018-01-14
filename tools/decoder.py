import serial
import io
import re
import time
from time import strftime, localtime

ser = serial.Serial('COM4', 115200, timeout=0)
sio = io.TextIOWrapper(io.BufferedRWPair(ser, ser))

def asbPkgDecode(line):
    m = re.search('\x01([0-9A-F]*)\x1f([0-9A-F]*)\x1f([0-9A-F]*)\x1f([0-9A-F]*)\x1f([0-9A-F]*)\x02((([0-9A-F]*)\x1f)*)\x04', line)
    if not m:
        return False

    pkg = {
        #Metadata
        'meta': {
            '''
            * Message type
            *  0 -> Broadcast
            *  1 -> Multicast
            *  2 -> Unicast
            '''
            'type': 0x00,

            '''
            * Port
            * 0x00 - 0x1F
            * Only used in Unicast Mode, otherwise -1

            '''
            'port': -1,

            '''
            * Target address
            * Unicast:             0x0001 - 0x07FF
            * Milticast/Broadcast: 0x0001 - 0xFFFF
            * 0x0000 = invalid packet
            '''
            'target': 0x0000,

            '''
            * Source address
            * 0x0001 - 0x07FF
            * 0x0000 = invalid packet
            '''
            'source': 0x0000
        },
        # length in bytes, 0-8. -1 indicates invalid packet
        'len': -1,
        # Payload
        'data': []
    }

    pkg['meta']['type'] = int(m.group(1), 16)
    pkg['meta']['target'] = int(m.group(2), 16)
    pkg['meta']['source'] = int(m.group(3), 16)
    pkg['meta']['port'] = int(m.group(4), 16)
    pkg['len'] = int(m.group(5), 16)

    if(pkg['len'] > 0):
        dm = re.findall('([0-9A-F]*)\x1f', m.group(6))

        dmc = 0
        while dmc < pkg['len']:
            pkg['data'].append(int(dm[dmc], 16))
            dmc=dmc+1

    return pkg

def asbPkgDecodeArrToUnsignedInt(a1, a2):
    return int((a1<<8) & a2)

def asbPkgDecodeArrToSignedInt(a1, a2):
    aint = asbPkgDecodeArrToUnsignedInt(a1, a2)
    if aint > pow(2,15):
        aint = 1-(aint-pow(2,15))

    return aint

def asbPkgDecodeArrToUnsignedLong(a1, a2, a3, a4):
    return ((a1<<24) & (a2<<16) & (a3<<8) & a4)

def asbPkgDecodeArrToSignedLong(a1, a2, a3, a4):
    aint = asbPkgDecodeArrToUnsignedLong(a1, a2, a3, a4)
    if aint > pow(2,31):
        aint = 1-(aint-pow(2,31))

    return aint

def asbPkgDecodeCmd(data):
    if len(data) < 1:
        return ''

    if data[0] == 0x21: return 'The sending node has just booted'
    if data[0] == 0x40: return 'The sending node requested the current state of this group'
    if data[0] == 0x50: return '0-bit-message'
    if data[0] == 0x51: return '1-bit-message, state is ' + str(data[1])
    if data[0] == 0x52: return 'percental Message, state is ' + str(data[1]) + '%'
    if data[0] == 0x70: return 'PING request'
    if data[0] == 0x71: return 'PONG (PING response)'
    if data[0] == 0x80: return 'Request to read configuration register ' + "{0:#0{1}x}".format(asbPkgDecodeArrToUnsignedInt(data[1], data[2]),6)
    if data[0] == 0x81: return 'Request to write configuration register ' + "{0:#0{1}x}".format(asbPkgDecodeArrToUnsignedInt(data[1], data[2]),6) + ' with value ' + "{0:#0{1}x}".format(data[3],4)
    if data[0] == 0x82: return 'Request to activate configuration register ' + "{0:#0{1}x}".format(asbPkgDecodeArrToUnsignedInt(data[1], data[2]),6)
    if data[0] == 0x85: return 'Request to change node-ID to ' + "{0:#0{1}x}".format(asbPkgDecodeArrToUnsignedInt(data[1], data[2]),6)
    if data[0] == 0xA0: return 'Temperature is ' + str(asbPkgDecodeArrToSignedInt(data[1], data[2])/10) + '°C'
    if data[0] == 0xA1: return 'Humidity is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2])/10) + '%RH'
    if data[0] == 0xA2: return 'Pressure is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2])/10) + 'hPa'
    if data[0] == 0xA5: return 'LUX is ' + str(asbPkgDecodeArrToUnsignedLong(data[1], data[2], data[3], data[4]))
    if data[0] == 0xA6: return 'UV-Index is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2])/10)
    if data[0] == 0xA7: return 'IR is ' + str(asbPkgDecodeArrToUnsignedLong(data[1], data[2], data[3], data[4]))
    #if data[0] == 0xB0:
    #if data[0] == 0xB1:
    if data[0] == 0xC0: return 'Voltage is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2])/10) + 'V'
    if data[0] == 0xC1: return 'Ampere is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2])/10) + 'A'
    if data[0] == 0xC2: return 'Power is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2])/10) + 'VA'
    if data[0] == 0xD0: return 'percental sensor is ' + str(data[1]) + '%'
    if data[0] == 0xD1: return 'permille sensor is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2])) + '‰'
    if data[0] == 0xD2: return 'parts per million sensor is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2]))
    if data[0] == 0xD5: return 'x per year sensor is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2]))
    if data[0] == 0xD6: return 'x per month sensor is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2]))
    if data[0] == 0xD7: return 'x per day sensor is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2]))
    if data[0] == 0xD8: return 'x per hour sensor is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2]))
    if data[0] == 0xD9: return 'x per minute sensor is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2]))
    if data[0] == 0xDA: return 'x per second sensor is ' + str(asbPkgDecodeArrToUnsignedInt(data[1], data[2]))
    return ''

def asbPkgDecodeType(v):
    if v == 0x00: return 'Broadcast'
    if v == 0x01: return 'Multicast'
    if v == 0x02: return 'Unicast'
    return 'unknown'

while True:
    message = sio.readline()
    if message.strip():
        print(strftime("%Y-%m-%d %H:%M:%S", localtime()) + ' << ' + message.rstrip())
        pkg = asbPkgDecode(message.rstrip())

        if pkg:
            print('---')
            print(strftime("%Y-%m-%d %H:%M:%S", localtime()));
            print("Packet type: " + asbPkgDecodeType(pkg['meta']['type']) + ' (' +  "{0:#0{1}x}".format(pkg['meta']['type'],4) + ')');
            print("Target:      " +  "{0:#0{1}x}".format(pkg['meta']['target'],6));
            print("Source:      " +  "{0:#0{1}x}".format(pkg['meta']['source'],6));
            print("Port:        " +  "{0:#0{1}x}".format(pkg['meta']['port'],4));
            print("Length:      " +  "{0:#0{1}x}".format(pkg['len'],4));

            dbc = 0
            for db in pkg['data']:
                print("  " + str(dbc) + " => " + "{0:#0{1}x}".format(db,4));
                dbc = dbc + 1;

            print(asbPkgDecodeCmd(pkg['data']))
            print('---')

    time.sleep(0.1)
