import serial
import io
import time
from time import strftime, localtime

port = 'COM4'

ser = serial.Serial(port, 115200, timeout=0)
sio = io.TextIOWrapper(io.BufferedRWPair(ser, ser))

def asbPkgEncode(mtype, target, source, port, data):
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

time.sleep(5)

sio.write(asbPkgEncode(0x01, 0x0122, 0x001, -1, [0x51, 0x01]))
sio.flush() # it is buffering. required to get the data out *now*
