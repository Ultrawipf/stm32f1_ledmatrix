#include "font.h"

//Register Bitmasks
#define R1  0x00000001
#define G1  0x00000002
#define B1  0x00000004
#define R2  0x00000008
#define G2  0x00000010
#define B2  0x00000020
#define A   0x00000040 //5
#define B   0x00000080 //4
#define C   0x00000100 //27
#define D   0x00000200 //26
#define CLK 0x00000400
#define NCLK 0x04000000

#define LAT 0x00002000 //22
#define OE  0x00004000 //21
#define NR1  R1 << 16
#define NG1  G1 << 16
#define NB1  B1 << 16
#define NR2  R2 << 16
#define NG2  G2 << 16
#define NB2  B2 << 16

#define ROWMASK 0b0000001111

//5bit
#define R_MASK 0b0000000000011111
#define G_MASK 0b0000001111100000
#define B_MASK 0b0111110000000000

#define A_PIN  5
#define B_PIN  4
#define C_PIN  27
#define D_PIN  26
#define LAT_PIN 22
#define OE_PIN  21
#define CLK_PIN  25

#define TMR_BASE 1700

#define HORIZ_COUNT 64

HardwareTimer timer(2); //Timer for refreshes

uint16_t row=0;
uint16_t col=0;
uint16_t currentStep=0;

uint16_t frame1[HORIZ_COUNT][32];
uint16_t frame2[HORIZ_COUNT][32];
bool waiting=false;
bool waiting_bars=false;
int x=0;
int y=0;
bool dots=false;
byte barDat[64];
volatile bool swap=false;
volatile bool bufIndex=0;
volatile uint16_t (*fptr)[32]=frame1;

void swapBuffer(){
  swap=true;
}

void copyBuffer(){
  for(int r=0;r<32;r++)
    for(int c=0;c<64;c++){
      uint16_t (*ptr)[32]= fptr==frame1?frame2:frame1;
      ptr[c][r]=fptr[c][r];

    }

}

void setPixel(uint16_t x,uint16_t y,uint8_t r,uint8_t g,uint8_t b){
  if(x>63 || y>31)
    return;
  uint16_t (*ptr)[32]= fptr==frame1?frame2:frame1;
  (ptr)[x][y]=rgbToFramebuffer(r,g,b);
}

void setPixel(uint16_t x,uint16_t y,uint16_t dat){ //direct
  if(x>63 || y>31)
    return;
  uint16_t (*ptr)[32]= fptr==frame1?frame2:frame1;
  (ptr)[x][y]=dat;
}

uint16_t rgbToFramebuffer(uint8_t r,uint8_t g,uint8_t b){
  return ((r & R_MASK) | ((g << 5) & G_MASK) | ((b << 10) & B_MASK));
}

void clearBuffer(){
  uint16_t (*ptr)[32]= fptr==frame1?frame2:frame1;
  for(int r=0;r<32;r++)
    for(int c=0;c<64;c++){
      (ptr)[c][r]=0;
    }

}

void clearMatrix(){
  for(int r=0;r<32;r++)
    for(int c=0;c<64;c++){
        frame2[c][r]=0;
        frame1[c][r]=0;
    }
}

void writeChar(uint16_t px,uint16_t py,uint8_t r,uint8_t g,uint8_t b,char c){ //8x8 font
  if(c>127)
    c=0;
  for (int y = 7; y >= 0; y--) {
    byte f = font[c][y];
    for (int x = 7; x >= 0; x--) {
      if (f & 0b1 << x){
        setPixel(x+px,y+py,r,g,b);
      }
    }
  }
}

void printMatrix(uint16_t px,uint16_t py,uint8_t r,uint8_t g,uint8_t b,String s,boolean cont){
  uint16_t x=0,y=0;

  for(char c : s){
    writeChar(px+x,py+y,r,g,b,c);
    x+=8;
    if(x+8>64){
      if(cont){
        x=0;
        y+=8;
      }else
        return;
    }
  }
}

void setup()
{
  Serial.begin(115200);
  for(int i=4;i<12;i++)
    pinMode(i, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(22, OUTPUT);
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(27, OUTPUT);

  digitalWrite(OE_PIN,LOW);
  digitalWrite(A_PIN,LOW);
  digitalWrite(B_PIN,LOW);
  digitalWrite(C_PIN,LOW);
  digitalWrite(D_PIN,LOW);
  digitalWrite(LAT_PIN,LOW);
  pinMode(33, OUTPUT); //led
  pinMode(32, INPUT);

  clearMatrix();

  timer.pause();
  timer.setPrescaleFactor(2);
  // Set up an interrupt on channel 1
  timer.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  timer.setCompare(TIMER_CH1, 1);  // Interrupt 1 count after each update
  timer.attachInterrupt(1, timer_handler);

  timer.setOverflow(TMR_BASE);


  for(int r=0;r<32;r++){
    for(int c=0;c<64;c++){
        setPixel(c,r,r,c,r+c);

    }
  }
  swapBuffer();
  timer.resume();
  delay(3000);


}

void sendStatus(){
  for(int r=0;r<32;r++){
    for(int c=0;c<64;c++){
      //Serial.print(frame[r][c][0]);
      Serial.print(',');
      }
      Serial.println(',');
    }
}


void timer_handler(){ //refresh
  uint16_t row2=row+16;
  int i=0;
  uint16_t dat1;
  uint16_t dat2;
  uint16_t data;
  uint16_t rows = ((row & 0b1111) << 6);
  //------------------------------------------------------------
 #define wub dat1 = (fptr[i][row] >> currentStep);\
    dat2 = (fptr[i][row2] >> currentStep);\
    data = ((0b1 & dat1) | (0b1 & (dat1 >> 5)) << 1|(0b1 & (dat1 >> 10)) << 2|(0b1 & (dat2)) << 3|(0b1 & (dat2 >> 5)) << 4 | (0b1 & (dat2 >> 10)) << 5);\
    GPIOA->regs->BSRR = data;\
    GPIOA->regs->BSRR = CLK;\
    ++i;\
    GPIOA->regs->BRR = CLK | 0b111111;
  //------------------------------------------------------------ execute 64 Times per row. Timing optimized for ghosting reduction.

  wub wub wub wub wub wub wub wub
  wub wub wub wub wub wub wub wub
  wub wub wub wub wub wub wub wub
  wub wub wub wub wub wub wub wub
  GPIOA->regs->BSRR = OE; //Disable Matrix
  wub wub wub wub wub wub wub wub
  wub wub wub wub wub wub wub wub
  GPIOA->regs->BSRR = ((~rows) & 0b1111000000) << 16 | rows; //Set Rows
  wub wub wub wub wub wub wub wub
  wub wub wub wub wub wub wub wub
  //-----------------------------------------------------------

  GPIOA->regs->BSRR = LAT; //Latch

  //setRows(row);
  GPIOA->regs->BRR = LAT;
  GPIOA->regs->BRR = OE;
  //Next Timer:
  if(++row > 15){
      row=0;
    if(currentStep < 4){ //bitdepth-1
      timer.setOverflow(TMR_BASE << currentStep++);

    }else{
      if(swap){
        fptr = fptr==frame1?frame2:frame1;
        swap=false;
        clearBuffer();
      }
      currentStep=0;
      timer.setOverflow(TMR_BASE);
    }

  }


}

void setRows(uint16_t rows){
  rows = ((rows & 0b1111) << 6);
  GPIOA->regs->BSRR = (~rows & 0b1111000000) << 16 | rows;
}



void loop()
{
   if(!swap && Serial.available() > waiting?1:0){ //don't do stuff if bufferswap remains
    char c = Serial.read();
    if(waiting){ //Todo.
        
        if(Serial.available()>0){
          byte b = Serial.read();
          uint16_t dat=c << 8 | b;

          setPixel(x,y,dat);//todo
        }
      

        if(++x>63){
          Serial.print('>');
          x=0;
          if(++y > 31){
            swapBuffer();
            waiting=false;
          }
        }

    }else if(c=='r'){
      x=0;
      y=0;
      waiting_bars=false;
      waiting=false;
      clearMatrix();
    }else if(c=='c'){
      x=0;
      y=0;
      waiting_bars=false;
      waiting=false;
      clearBuffer();
    }else if(c=='s'){
      swapBuffer();
    }else if(waiting_bars && c!='b'){
      if(x<64){
        byte d=min(32,c);

        if(dots){
            setPixel(x,d,d>24?d:0, d>3 && d<29 ? 60:0, d<7 ? 63-d*2 : 0);
        }else{
          for(byte h=0;h<d;h++){
              //frame[x][31-h] = rgbToFramebuffer(h>24?h:0, h>3 && h<29 ? 60:0, h<7 ? 63-h*2 : 0);
              //setPixel(x,31-h, h>24?h:0, h>3 && h<29 ? 20:0, h<7 ? 31-h*2 : 0);
              setPixel(x,31-h, constrain(4*(h-22),0,31), constrain(31-abs((2*(h-15))),0,31), constrain(31-(h*3),0,31));
          }
        }
        x++;
      }if(x>=64){

        waiting_bars=false;
        swapBuffer();
        Serial.print('f');
        x=0;

      }
    }else if(c == '>'){//Text Mode

      String serialBuf;
      char nxt = Serial.peek();
      int r=31,g=31,b=31;
      uint8_t line=0;
      uint16_t cnt=0;
      boolean cont=true;
      boolean refresh=true;
      if(nxt >= '0' && nxt <='9'){ //color and line mode
        //copyBuffer();//Save current state to new buffer
        refresh=false;
        line=min(3,nxt-'0')*8;
        cont=false;
        Serial.read();
        while(Serial.available()<1){
          delay(1);
        }if(Serial.read()==';'){
          while(Serial.available()<2){
            delay(1);
          }
         uint16_t cbuf=Serial.parseInt();
         //byte t = Serial.read();
         //uint16_t cbuf = t | Serial.read() << 8;
         if(Serial.peek()==';')
          Serial.read();
         r=cbuf & R_MASK;
         g=(cbuf & G_MASK) >> 5;
         b=(cbuf & B_MASK) >> 10;
        }
      }else{
        //clearBuffer();
      }
      serialBuf=Serial.readStringUntil('\n');
      printMatrix(0,line,r,g,b,serialBuf,cont);
      if(refresh){
        swapBuffer();
      }
    }else if(c =='d'){ //Raw Data, one line, todo
      waiting=true;
      x=0;
      y=0;
      Serial.print('>');
    }
    else if(c=='b'){//bars
      clearBuffer();
      waiting_bars=true;
      x=0;
      dots=false;
      Serial.print('>');
    }else if(c=='p'){//points
      clearBuffer();
      waiting_bars=true;
      x=0;
      dots=true;
      Serial.print('>');
    }

  
  }

}
