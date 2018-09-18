import numpy as np
import pyaudio
import time
import serial,time, os
import sys
screenPort = "COM9"
screen = serial.Serial(screenPort,115200)
screen.timeout = (2)


class SpectrumAnalyzer:
    FORMAT = pyaudio.paFloat32
    CHANNELS = 1
    RATE = 44100
    CHUNK = 768
    START = 0
    N = 768

    cnt=0
    hz_l = []
    data = []

    def __init__(self):
        self.pa = pyaudio.PyAudio()
        self.stream = self.pa.open(format = self.FORMAT,
            channels = self.CHANNELS,
            rate = self.RATE,
            input = True,
            output = False,
            frames_per_buffer = self.CHUNK)
        # Main loop
        self.last_update = time.time()
        self.loop()

    def loop(self):
        try:
            last_spec = [0]*self.CHUNK
            while True :
                self.data = self.audioinput()
                spec = np.add(self.fft(),last_spec)

                self.show_led(spec)
                last_spec = np.multiply(spec,0.35)
                # sync?


        except KeyboardInterrupt:
            screen.write(b"r")
            self.pa.close(self.stream)


        print("End...")

    def audioinput(self):
        ret = self.stream.read(self.CHUNK)
        ret = np.fromstring(ret, np.float32)
        return ret

    def fft(self):
        y = np.fft.fft(self.data[self.START:self.START + self.N])
        return [np.sqrt(c.real ** 2 + c.imag ** 2) for c in y]

    def show_led(self,spec):
        screen.write(b"b")
        screen.read()
        step=5
        last = 0
        buf = []
        for i in range(64):
            # increase step size for high frequencies
            step=round(i/8)+1
            num = np.max(spec[int(last+1):int(last+step+1)])
            last += step
            num = np.log(0.9*num+1)*3.3
            num = int(min(max(num*np.log((i)/3 +4.6),0),32))
            buf.append(num)
        screen.write(bytes(buf))


        if screen.read() == b"f":
            self.cnt += 1
            self.hz_l.append((time.time()-self.last_update))
            self.last_update = time.time()
            if self.cnt % 5 == 0:
                self.cnt=0
                sys.stdout.write("\rRefreshrate:"+str(round(1/np.mean(self.hz_l)))+"Hz ")
                self.hz_l.clear()
                sys.stdout.flush()


if __name__ == "__main__":
    os.system('mode con: cols=30 lines=3')
    spec = SpectrumAnalyzer()
