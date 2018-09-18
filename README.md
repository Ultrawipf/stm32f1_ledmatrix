This is a simple stm32f103 based led matrix driver designed for a single 32x64 1/16 scan led matrix and a maple mini clone or similar.

Features 5 bit colors, usb serial text functions, high speed spectrum analyzer effects with example python script and direct frame sending which is a bit slower but much faster than standard arduino libs.

## Instructions for maple mini boards:
Connect the matrix as following:

The pins are optimized for fast data writing.

These are the numbers on a maple mini like documented here: https://wiki.stm32duino.com/index.php?title=Maple_Mini


Matrix pin - board pin  

R1 - 11  
G1 - 10  
B1 - 9  
R2 - 8  
G2 - 7  
B2 - 6  
A - 5  
B - 4  
C - 27  
D - 26  
CLK - 25  
LAT - 22  
OE - 21  
GND - GND  


Sending text is as easy as sending a string formatted like `><line>;<text>\n`.
Or use the example python library functions like
```
from ledmatrix import Led_matrix
matrix = Led_matrix("COM5")
matrix.print_screen((0,31,0),0,"Test")
```
Colors are 5 bit (r,g,b) 0-31.

Or to send a vertical image where test.bmp is a 32x64 image:
```
from PIL import Image
from ledmatrix import Led_matrix
matrix = Led_matrix("COM5")
im = Image.open("test.bmp")
matrix.send_im(im,rotate = Image.ROTATE_270)
```

The led_fft.py script is a fast fft spectrum analyzer grabbing the audio from the default input device. Change the `screenPort` to the correct serial port.
