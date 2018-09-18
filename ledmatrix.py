import serial,time,json, os,sys

import numpy as np
import bitstring as bs
import random,time
from PIL import Image,ImageEnhance
#screenPort = 'COM25'


def c(r,g, b):
     res = r & 0x1F | (g & 0x1F) << 5 | (b & 0x1F) << 10
     return str(res) + ';'
def c(col):
     res = col[0] & 0x1F | (col[1] & 0x1F) << 5 | (col[2] & 0x1F) << 10
     return str(res) + ";"

class Led_matrix:
    def __init__(self,port):

        self.screen = serial.Serial(port,115200)
        self.screen.timeout = (2)
        print("Matrix initialized on port",port)


    def print_screen(self,color,line,l):
        self.screen.write(bytes('>'+str(l)+';'+c(color)+str(line)+'\n','utf-8'))

    def __exit__(self, exc_type, exc_value, traceback):
        self.screen.close()
    def __enter__(self):
        return self
    def send_raw_im(self,im):

        im = im.quantize(96)
        im = im.convert(mode="RGB")
        im = im.point(lambda x: min(31,round((x)/8)))
        dat = im.getdata()


        self.send_buffer(dat)

    def clear(self):
        self.screen.write(b"r")
    def show(self):
        self.screen.write(b"s")
    def clearbuf(self):
        self.screen.write(b"c")

    def send_buffer(self,dat):
        buf = np.ndarray(len(dat),dtype=">u2")
        #print(dat)

        self.screen.write(b"d")
        self.screen.read()
        #t = time.time()
        pos=0
        for p in dat:
            d=((p[0] & 0x1f) | ((p[1] & 0x1f)<<5 | (p[2] & 0x1f) << 10 ) ) & 0x7fff
            buf[pos] = d
            pos+=1

        self.screen.write(buf)
        self.screen.read(32)

    def send_im(self,im, rotate=False):
        if rotate:
            im = im.transpose(rotate)
        if not im.size == (64,32):
            im = im.resize((64,32),resample=Image.BILINEAR)

        enhancer = ImageEnhance.Color(im)
        im = enhancer.enhance(1.5)
        min_v = np.min(im)
        max_v = np.max(im)
        im = im.point(lambda x: x - min_v)

        im = im.quantize(96)
        im = im.convert(mode="RGB")


        im = im.point(lambda x: min(31,round((x)/8)))
        dat = im.getdata()
        self.send_buffer(dat)

    def send_frame(self,frame):
        #print(frame)
        buf = bs.BitArray()#np.zeros(32,dtype=np.uint8)
        self.screen.write(b"d")
        #screen.write([1]*3*32*64)
        for y in range(32):
            #screen.write(b"l"+bs.Bits(uint=y,length=8))
            line = frame[y]
            #time.sleep(0.1)
            for px in line:
                d =bs.Bits(length=16,uint=((px[0] & 0x1f) | ((px[1] & 0x1f)<<5 | (px[2] & 0x1f) << 10 ) ) & 0x7fff)
                buf.append(d)


        self.screen.write(buf.bytes)

        buf.clear()
