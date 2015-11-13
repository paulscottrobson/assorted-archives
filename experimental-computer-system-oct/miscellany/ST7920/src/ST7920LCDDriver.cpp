// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//    Name:     ST7920LCDDriver.cpp
//    Purpose:  LCD Interface for STD7920 128x64 LCD Display
//    Created:  20th October 2015
//    Authors:  Paul Robson (paul@robsons.org.uk), David Crocker (original code)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include "ST7920LCDDriver.h"
#include <pins_arduino.h>
#include <avr/interrupt.h>

// LCD basic instructions. These all take 72us to execute except LcdDisplayClear, which takes 1.6ms
const uint8_t LcdDisplayClear = 0x01;
const uint8_t LcdHome = 0x02;
const uint8_t LcdEntryModeSet = 0x06;      
const uint8_t LcdDisplayOff = 0x08;
const uint8_t LcdDisplayOn = 0x0C;         
const uint8_t LcdFunctionSetBasicAlpha = 0x20;
const uint8_t LcdFunctionSetBasicGraphic = 0x22;
const uint8_t LcdFunctionSetExtendedAlpha = 0x24;
const uint8_t LcdFunctionSetExtendedGraphic = 0x26;
const uint8_t LcdSetDdramAddress = 0x80;  

// LCD extended instructions
const uint8_t LcdSetGdramAddress = 0x80;

const unsigned int LcdCommandDelayMicros = 72 - 24; // 72us required, less 24us time to send the command @ 1MHz
const unsigned int LcdDataDelayMicros = 10;         // Delay between sending data bytes
const unsigned int LcdDisplayClearDelayMillis = 3;  // 1.6ms should be enough

ST7920LCDDriver::ST7920LCDDriver(uint8_t cPin, uint8_t dPin, bool spi) : clockPin(cPin), dataPin(dPin), useSpi(spi)
{
}

void ST7920LCDDriver::begin(bool gmode)
{
  
  // Set up the SPI interface for talking to the LCD. We have to set MOSI, SCLK and SS to outputs, then enable SPI.
  digitalWrite(clockPin, LOW);
  digitalWrite(dataPin, LOW);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  if (useSpi)
  {
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);  // enable SPI, master mode, clock low when idle, data sampled on rising edge, clock = f/16 (= 1MHz), send MSB first
    //SPSR = (1 << SPI2X);  // double the speed to 2MHz (optional)
  }

  gfxMode = false;
  sendLcdCommand(LcdFunctionSetBasicAlpha);
  delay(1);
  sendLcdCommand(LcdFunctionSetBasicAlpha);
  commandDelay();
  sendLcdCommand(LcdEntryModeSet);
  commandDelay();
  extendedMode = false;
  gfxMode = true;
  sendLcdCommand(LcdDisplayOn);
  commandDelay();
}

uint16_t blocks[] = { 0x00,0x03,0x0C,0x0F, 
                      0x30,0x33,0x3C,0x3F, 
                      0xC0,0xC3,0xCC,0xCF, 
                      0xF0,0xF3,0xFC,0xFF};

void ST7920LCDDriver::draw(uint8_t col,uint8_t row,uint8_t *data,uint8_t count)
{
    setGraphicsAddress(row,col);
    while (count-- > 0) {
      uint8_t b = *data++;
      sendLcdData(blocks[b >> 4]);
      sendLcdData(blocks[b & 0x0F]);
    }
    delayMicroseconds(LcdDataDelayMicros);
}
  
void ST7920LCDDriver::clear(void) 
{
    for (uint8_t y = 0;y < 64;y++) {
      setGraphicsAddress(y,0);
      for (uint8_t x = 0;x < 8;x++) {
        sendLcdData(0);
        sendLcdData(0);
        delayMicroseconds(LcdDataDelayMicros);
      }
    }
}

void ST7920LCDDriver::setGraphicsAddress(unsigned int r, unsigned int c)
{
  if (gfxMode)
  {
    ensureExtendedMode();
    sendLcdCommand(LcdSetGdramAddress | (r & 31));
    //commandDelay();  // don't seem to need this one
    sendLcdCommand(LcdSetGdramAddress | c | ((r & 32) >> 2));
    commandDelay();    // we definitely need this one
  }
}

void ST7920LCDDriver::commandDelay()
{
  delayMicroseconds(LcdCommandDelayMicros);
}

// Send a command to the LCD
void ST7920LCDDriver::sendLcdCommand(uint8_t command)
{
  sendLcd(0xF8, command);
}

// Send a data byte to the LCD
void ST7920LCDDriver::sendLcdData(uint8_t data)
{
  sendLcd(0xFA, data);
}

// Send a command to the lcd. Data1 is sent as-is, data2 is split into 2 bytes, high nibble first.
void ST7920LCDDriver::sendLcd(uint8_t data1, uint8_t data2)
{
  if (useSpi)
  {
    SPDR = data1;
    while ((SPSR & (1 << SPIF)) == 0) { }
    SPDR = data2 & 0xF0;
    while ((SPSR & (1 << SPIF)) == 0) { }
    SPDR = data2 << 4;
    while ((SPSR & (1 << SPIF)) == 0) { }
  }
  else
  {
    sendLcdSlow(data1);
    sendLcdSlow(data2 & 0xF0);
    sendLcdSlow(data2 << 4);
  }
}

void ST7920LCDDriver::sendLcdSlow(uint8_t data)
{
  // Fast shiftOut function
  volatile uint8_t *sclkPort = portOutputRegister(digitalPinToPort(clockPin));
  volatile uint8_t *mosiPort = portOutputRegister(digitalPinToPort(dataPin));
  uint8_t sclkMask = digitalPinToBitMask(clockPin);
  uint8_t mosiMask = digitalPinToBitMask(dataPin);
  
  uint8_t oldSREG = SREG;
  cli();
  for (uint8_t i = 0; i < 8; ++i)
  {
    if (data & 0x80)
    {
      *mosiPort |= mosiMask;
    }
    else
    {
      *mosiPort &= ~mosiMask;
    }
    *sclkPort |= sclkMask;
    *sclkPort &= ~sclkMask;
    data <<= 1;
  }
  SREG = oldSREG;
  }

void ST7920LCDDriver::ensureBasicMode()
{
  if (extendedMode)
  {
    sendLcdCommand(gfxMode ? LcdFunctionSetBasicGraphic : LcdFunctionSetBasicAlpha);
    commandDelay();
    extendedMode = false;
  }
}

void ST7920LCDDriver::ensureExtendedMode()
{
  if (!extendedMode)
  {
    sendLcdCommand(gfxMode ? LcdFunctionSetExtendedGraphic : LcdFunctionSetExtendedAlpha);
    commandDelay();
    extendedMode = true;
  }
}