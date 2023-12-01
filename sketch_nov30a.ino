#include <Arduino.h>
#include <EEPROM.h>
#include "Font_RUS2.h"
#include <avr/pgmspace.h>
#include <SPI.h> 
#include "HUB08SPI.h" // add library https://github.com/emgoz/HUB08SPI
#include <TimerOne.h> // add library https://github.com/PaulStoffregen/TimerOne

#define FIRST_RUN true // false

#define WIDTH   64
#define HEIGHT  16

uint8_t displaybuf[WIDTH * HEIGHT / 8];

#define UART_BAUNDRATE 9600
#define PIN_INCREASE_BUTTON A1 // "+1"
#define PIN_CON_INCREASE_BUTTON A2 // "+"
#define PIN_CON_DECREASE_BUTTON A3 // "-"
#define PIN_SAVE_BUTTON A4 // "save"
#define REFRESH_RATE 150 // display refresh delay measured in parrots. less delay == more fps

#define DEBOUNCE_TIME 500 // delay time for "+", "-"" and "save" buttons

#define EEPROM_ADDRESS 0

HUB08SPI display;
int counter = 0;
bool buttonPlus1Pressed = false; // hold current state of "+1 button"
unsigned long lastDebounceTime = 0;
boolean flag_len = 0;
int length = WIDTH;
volatile uint16_t Nx = 0; // "cursor" position

char cstr[8]; // output chars

void setup() {
  pinMode(PIN_INCREASE_BUTTON, INPUT);
  pinMode(PIN_CON_INCREASE_BUTTON, INPUT);
  pinMode(PIN_CON_DECREASE_BUTTON, INPUT);
  pinMode(PIN_SAVE_BUTTON, INPUT);

  // set low level for buttons
  // poorly documented feature of atmega
  // setting PORTX to low enables internal binding to gnd
  digitalWrite(PIN_INCREASE_BUTTON, LOW); 
  digitalWrite(PIN_CON_INCREASE_BUTTON, LOW);
  digitalWrite(PIN_CON_DECREASE_BUTTON, LOW);
  digitalWrite(PIN_SAVE_BUTTON, LOW);

  Serial.begin(UART_BAUNDRATE);

  if (FIRST_RUN) {
    EEPROM.write(EEPROM_ADDRESS, 0);
  }
  counter = EEPROM.read(EEPROM_ADDRESS); 

  Timer1.initialize(200);
  Timer1.attachInterrupt(refresh);   
}

void loop() {
  // Check "+1" button
  if (digitalRead(PIN_INCREASE_BUTTON) == HIGH && !buttonPlus1Pressed) {
    counter++;
    EEPROM.write(EEPROM_ADDRESS, counter); // Save to EEPROM
    buttonPlus1Pressed = true;
    Serial.println("Increase button pressed");
    printCounter();
  } else if (digitalRead(PIN_INCREASE_BUTTON) == LOW && buttonPlus1Pressed) {
    buttonPlus1Pressed = false;
    Serial.println("Increase button released");
  }

  // Check "+" button
  if (digitalRead(PIN_CON_INCREASE_BUTTON) == HIGH && isButtonsAvailable()) {
    counter++;
    lastDebounceTime = millis();
    Serial.println("Con. increase button pressed");
    printCounter();
  }

  // Check "-" button
  if (digitalRead(PIN_CON_DECREASE_BUTTON) == HIGH && isButtonsAvailable()) {
    counter--;
    lastDebounceTime = millis();
    Serial.println("Con. decrease button pressed");
    printCounter();
  }

  // Check "Save" button
  if (digitalRead(PIN_SAVE_BUTTON) == HIGH && isButtonsAvailable()) {
    EEPROM.write(EEPROM_ADDRESS, counter); // Save to EEPROM
    lastDebounceTime = millis();
    Serial.println("Save button pressed");
  }

  // Print curront counter
  Serial.println("Counter: " + String(counter));

}

/*
* returns true if more than [DEBOUNCE_TIME] ms has passed since the last button click
*/
bool isButtonsAvailable() {
  return (millis() - lastDebounceTime) > DEBOUNCE_TIME;
}

/**
* clear the display, cast [counter] int to char array and put into [cstr] (only 8 digits), print [cstr]
*/
void printCounter() {
  display.clear();
  sprintf(cstr, "%08d", counter);
  printString(cstr, 0, 0);
}



/*port from https://www.drive2.ru/c/490697746899009800/ */

//функция обновления строки,(прерывания таймера)
void refresh() { 
  
  static uint16_t count = 0; 
  count++;
  if (count > REFRESH_RATE) { 
    count= 0; 
    Nx++;
    if(Nx > length) Nx = 0;  //reset после xxx pixels
  } 
  display.scan();
}

void printString(char* msg, int x,int8_t ZX) {
  while (*msg) {
    int c = *msg;
    if (c>-49 && c<32 ) {  //если это первый байт кириллицы,пропускаем
      msg++; 
      continue;
    }
    c<-47 ? c+=224 : c-=32;//если кириллица прибавляем 256-32.иначе -32.
    if(flag_len) {
      printChar(c,x,ZX);//печатаем после подсчета длины фразы
    } 
    x+= pgm_read_byte_near(font_rus + (c * 33))+1;//складываем первые байты из массива(ширину)
    msg++;
  }
  if (!flag_len) { //если нужно посчитать ширину фразы
    length = x; 
    flag_len = 1; //заканчиваем расчет ширины фразы
    length <= WIDTH ? length = WIDTH : length += WIDTH;
  }
}

void printChar(byte c, int x, int y) {
  byte l = pgm_read_byte_near(font_rus + (c * 33))+1; //получить ширину в пикселях характер
  for (int a=0;a<16;a++) {   
    clearLine(x,y+a,l);
    writeByte(x,y+a,pgm_read_byte_near(font_rus + (c * 33) +a*2 +1));
    writeByte(x+8,y+a,pgm_read_byte_near(font_rus + (c * 33) +a*2 +2));
  }
}

void writeByte(int x, int y, uint8_t data) {
    if (x >= WIDTH ||y >= HEIGHT || x+8<=0 || y < 0) return; //outside screen
    uint8_t offset = x & 7;  //bit offset//7
    if (offset) { //не выравнивается
        writeByte(x-offset,y,data>>offset);
        writeByte(x+8-offset,y,data<<(8-offset));
    } else { //выравниваем byte
        uint8_t col = x / 8;
        displaybuf[y*8+col] |= data;
    }
}

void clearLine(int x,int y, int w) {
  if (y < 0 || y >= HEIGHT || x >= WIDTH) return;
  if (x < 0) {
      w = w+x;
      x = 0;
  }
  if (x + w > WIDTH) w = WIDTH-x;
  if (w <= 0) return;

  if ((x& 7)+w <= 8) {
      uint8_t m = 0xFF << (8-w);
      m >>= (x& 7);
      displaybuf[y*8+x/8] &= ~m;
  } else {
    uint16_t start = y*8+(x+7)/8;  //included
    uint16_t end =   y*8+(x+w)/8;  //not included
    if (x& 7) displaybuf[start-1] &= ~(0xFF >> (x& 7));
    for (uint16_t p = start; p < end ; p++) {
      displaybuf[p] = 0;
    }
    if ((x+w)& 7) displaybuf[end] &= ~(0xFF << (8-(x+w)& 7));
  }  
}
 
