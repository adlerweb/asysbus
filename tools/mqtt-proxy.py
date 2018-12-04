# This script will relay 0x51 and 0x52 messages (on/off and percent) between
# a aSysBus-Node connected via serial and a MQTT broker

# Requires:
#   asyncio
#   aiomqtt (+ paho-mqtt)
#   pyserial-asyncio (+ pyserial)

import asyncio
import serial_asyncio
from functools import partial

import os

from time import strftime, localtime
import time

import re

import random

import aiomqtt

port = '/dev/ttyUSB0'
baud = 115200
myid = 0x123

mqtt_server = 'localhost'
mqtt_port = 1883
mqtt_topicBase = '/asysbus'
mqtt_user = 'asysbus'
mqtt_pass = 'topsecret'
mqtt_ca = ''

if not os.path.exists(port):
    print('Serial port does not exist')

class canRxTx(asyncio.Protocol):
    def __init__(self, queueRX, queueTX):
        super().__init__()
        self.transport = None
        self.buf = None
        self.queueRX = queueRX
        self.queueTX = queueTX

    def connection_made(self, transport):
        self.transport = transport
        self.buf = bytes()
        print('CAN serial port opened', transport)
        asyncio.ensure_future(self.send())
        #asyncio.ensure_future(self.debugWorker())

    def data_received(self, data):
        self.buf += data
        if b'\n' in self.buf:
            lines = self.buf.split(b'\n')
            self.buf = lines[-1] # whatever was left over
            for line in lines[:-1]:
                asyncio.ensure_future(self.queueRX.put(line))

    def connection_lost(self, exc):
        print('CAN serial port closed')
        asyncio.get_event_loop().stop()

    async def send(self):
        while True:
            print('Waiting for TX job...')
            msg = await self.queueTX.get()
            self.transport.serial.write(bytes(msg, 'ascii'))
            print(f'Writer sent: {msg}')

    async def debugWorker(self):
        while True:
            sleep_for = random.uniform(1.0, 3.0)
            command = random.choice([0, 1])

            await asyncio.sleep(sleep_for)

            print(f'debugWorker has slept for {sleep_for:.2f} seconds')

            dummyout = asbPkgEncode(0x01, 0x1001, myid, -1, [0x51, command])
            asyncio.ensure_future(self.queueTX.put(dummyout))
            print(f'Queued: {dummyout}')

async def procCANRx(queue, mqttTxQueue):

    while True:
        msg = await queue.get()
        msg = msg.decode()
        msg = msg.strip()
        pkg = False

        try:
            pkg = asbPkgDecode(msg)
        except:
            #print('Decode failed!')
            True

        if pkg:
            print('[', strftime("%Y-%m-%d %H:%M:%S", localtime()), '] CAN Message received: ', msg)
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

            if pkg['len'] == 2 and (pkg['data'][0] == 0x51): #1-Bit-Message
                asyncio.ensure_future(mqttTxQueue.put(["/" + "{0:0{1}x}".format(pkg['meta']['target'],4) + "/get/switch", str(pkg['data'][1]), True]))
                
            if pkg['len'] == 2 and (pkg['data'][0] == 0x52): #% message
                asyncio.ensure_future(mqttTxQueue.put(["/" + "{0:0{1}x}".format(pkg['meta']['target'],4) + "/get/level", str(pkg['data'][1]), True]))
                
            if pkg['len'] == 1 and (pkg['data'][0] == 0x21): #Boot
                asyncio.ensure_future(mqttTxQueue.put(["/" + "{0:0{1}x}".format(pkg['meta']['source'],4) + "/lastboot", time.time(), False]))
                
        queue.task_done()

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
            try:
                pkg['data'].append(int(dm[dmc], 16))
            except:
                pass
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
    if data[0] == 0xA0: return 'Temperature is ' + str(asbPkgDecodeArrToSignedInt(data[1], data[2])/10) + 'C'
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
    if data[0] == 0x02: return 'Legacy 4 Byte Message'
    return ''

def asbPkgDecodeType(v):
    if v == 0x00: return 'Broadcast'
    if v == 0x01: return 'Multicast'
    if v == 0x02: return 'Unicast'
    return 'unknown'

def asbPkgEncode(mtype, target, source, port, data):
    put = (bytes)
    out =       chr(0x01)
    out = out + format(mtype, 'x')
    out = out + chr(0x1F)
    out = out + format(target, 'x')
    out = out + chr(0x1F)
    out = out + format(source, 'x')
    out = out + chr(0x1F)
    if port < 0:
        out = out + 'ff'
    else:
        out = out + format(port, 'x')
    out = out + chr(0x1F)
    out = out + format(len(data), 'x')
    out = out + chr(0x02)
    for db in data:
        out = out + format(db, 'x')
        out = out + chr(0x1F)
    out = out + chr(0x04)
    out = out + "\r\n"
    return out.upper()

async def mqtt(mqttTxQueue, canTxQueue, server, port, topicBase, user, passwd, ca):
    client = aiomqtt.Client(loop)
    client.loop_start()

    connected = asyncio.Event(loop=loop)
    def on_connect(client, userdata, flags, rc):
        connected.set()
    client.on_connect = on_connect

    client.will_set(topicBase + '/LWT', 'OFF', 0, False)
    if ca:
        client.tls_set(ca);
    if user and passwd:
        client.username_pw_set(user, passwd)
    
    await client.connect(server, port)
    await connected.wait()
    print("MQTT connected")

    subscribed = asyncio.Event(loop=loop)
    def on_subscribe(client, userdata, mid, granted_qos):
        subscribed.set()
    client.on_subscribe = on_subscribe

    client.subscribe(topicBase + "/+/set/#")
    await subscribed.wait()
    print("MQTT Subscribed to " + topicBase)

    def on_message(client, userdata, msg):
        print("MQTT RX: " + msg.topic + " = " + str(msg.payload))
        topic = msg.topic
        topic = topic.replace(mqtt_topicBase + "/", '')
        topic = topic.split('/')

        if len(topic) != 3:
            print('MQTT message topic length error')
            return

        if topic[1] != 'set':
            print('MQTT message topic type error')
            return

        topici = int(topic[0], 16)
        
        if topic[2] == 'switch':
            print(asbPkgEncode(0x01, topici, myid, -1, [0x51, int(msg.payload)]))
            asyncio.ensure_future(canTxQueue.put(asbPkgEncode(0x01, topici, myid, -1, [0x51, int(msg.payload)])))
            asyncio.ensure_future(mqttTxQueue.put(["/" + topic[0] + "/get/switch", msg.payload.decode(), True]))
           
        if topic[2] == 'level':
            print(asbPkgEncode(0x01, topici, myid, -1, [0x52, int(msg.payload)]))
            asyncio.ensure_future(canTxQueue.put(asbPkgEncode(0x01, topici, myid, -1, [0x52, int(msg.payload)])))
            asyncio.ensure_future(mqttTxQueue.put(["/" + topic[0] + "/get/level", msg.payload.decode(), True]))
        
    client.on_message = on_message

    lwtPublish = client.publish(mqtt_topicBase + "/LWT", 'ON')
    await lwtPublish.wait_for_publish()
    print("MQTT LWT published!")

    while True:
        msg = await mqttTxQueue.get()
        try:
            msgPublish = client.publish(mqtt_topicBase + msg[0], msg[1], retain=msg[2])
            await msgPublish.wait_for_publish()
            print("MQTT publish - Retain: " + str(msg[2]) + " Topic: " + mqtt_topicBase + msg[0] + " - Message: " + str(msg[1]))
        except IndexError:
            print("MQTT publish failed: ")
            print(msg)

canRxQueue = asyncio.Queue()
canTxQueue = asyncio.Queue()
mqttTxQueue = asyncio.Queue()
canPartial = partial(canRxTx, canRxQueue, canTxQueue)

loop = asyncio.get_event_loop()
canAIO = serial_asyncio.create_serial_connection(loop, canPartial, port, baudrate=baud)
asyncio.ensure_future(canAIO)
asyncio.ensure_future(procCANRx(canRxQueue, mqttTxQueue))
asyncio.ensure_future(mqtt(mqttTxQueue, canTxQueue, mqtt_server, mqtt_port, mqtt_topicBase, mqtt_user, mqtt_pass, mqtt_ca))

loop.run_forever()
loop.close()
