/*
 * ssd1306.c
 *
 *  Created on: 2020. 12. 30.
 *      Author: baram
 */


#include "lcd/ssd1306.h"
#include "lcd/ssd1306_regs.h"
#include "i2c.h"


#ifdef _USE_HW_SSD1306

#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64


static uint8_t i2c_ch  = _DEF_I2C1;
static uint8_t i2c_dev = 0x78>>1; // 0x3C
static void (*frameCallBack)(void) = NULL;


static bool ssd1306Reset(void);
static void ssd1306SetWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
static uint16_t ssd1306GetWidth(void);
static uint16_t ssd1306GetHeight(void);
static bool ssd1306SendBuffer(uint8_t *p_data, uint32_t length, uint32_t timeout_ms);
static bool ssd1306SetCallBack(void (*p_func)(void));
static void ssd1306Fill(uint16_t color);
static bool ssd1306UpdateDraw(void);
static void ssd1306DrawPixel(uint8_t x, uint8_t y, uint16_t color);


static uint8_t ssd1306_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];



bool ssd1306Init(void)
{
  bool ret;

  ret = ssd1306Reset();

  return ret;
}

bool ssd1306InitDriver(lcd_driver_t *p_driver)
{
  p_driver->init        = ssd1306Init;
  p_driver->reset       = ssd1306Reset;
  p_driver->setWindow   = ssd1306SetWindow;
  p_driver->getWidth    = ssd1306GetWidth;
  p_driver->getHeight   = ssd1306GetHeight;
  p_driver->setCallBack = ssd1306SetCallBack;
  p_driver->sendBuffer  = ssd1306SendBuffer;
  return true;
}

bool ssd1306WriteCmd(uint8_t cmd_data)
{
  return i2cWriteByte(i2c_ch, i2c_dev, 0x00, cmd_data, 10);
}

bool ssd1306Reset(void)
{
  bool ret;

  ret = i2cBegin(i2c_ch, 800);

  if (ret != true)
  {
    return false;
  }

  if (i2cIsDeviceReady(i2c_ch, i2c_dev) != true)
  {
    return false;
  }

  /* Init LCD */
  ssd1306WriteCmd(0xAE); //display off
  ssd1306WriteCmd(0x20); //Set Memory Addressing Mode
  ssd1306WriteCmd(0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
  ssd1306WriteCmd(0xB0); //Set Page Start Address for Page Addressing Mode,0-7
  ssd1306WriteCmd(0xC8); //Set COM Output Scan Direction
  ssd1306WriteCmd(0x00); //---set low column address
  ssd1306WriteCmd(0x10); //---set high column address
  ssd1306WriteCmd(0x40); //--set start line address
  ssd1306WriteCmd(0x81); //--set contrast control register
  ssd1306WriteCmd(0xFF);
  ssd1306WriteCmd(0xA1); //--set segment re-map 0 to 127
  ssd1306WriteCmd(0xA6); //--set normal display
  ssd1306WriteCmd(0xA8); //--set multiplex ratio(1 to 64)
  ssd1306WriteCmd(0x3F); //
  ssd1306WriteCmd(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
  ssd1306WriteCmd(0xD3); //-set display offset
  ssd1306WriteCmd(0x00); //-not offset
  ssd1306WriteCmd(0xD5); //--set display clock divide ratio/oscillator frequency
  ssd1306WriteCmd(0xF0); //--set divide ratio
  ssd1306WriteCmd(0xD9); //--set pre-charge period
  ssd1306WriteCmd(0x22); //
  ssd1306WriteCmd(0xDA); //--set com pins hardware configuration
  ssd1306WriteCmd(0x12);
  ssd1306WriteCmd(0xDB); //--set vcomh
  ssd1306WriteCmd(0x20); //0x20,0.77xVcc
  ssd1306WriteCmd(0x8D); //--set DC-DC enable
  ssd1306WriteCmd(0x14); //
  ssd1306WriteCmd(0xAF); //--turn on SSD1306 panel

  ssd1306Fill(black);
  ssd1306UpdateDraw();

  return true;
}

void ssd1306SetWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
}

uint16_t ssd1306GetWidth(void)
{
  return LCD_WIDTH;
}

uint16_t ssd1306GetHeight(void)
{
  return LCD_HEIGHT;
}

bool ssd1306SendBuffer(uint8_t *p_data, uint32_t length, uint32_t timeout_ms)
{
  uint16_t *p_buf = (uint16_t *)p_data;


  for (int y=0; y<SSD1306_HEIGHT; y++)
  {
    for (int x=0; x<SSD1306_WIDTH; x++)
    {
      ssd1306DrawPixel(x, y, p_buf[y*LCD_WIDTH + x]);
    }
  }

  ssd1306UpdateDraw();

  if (frameCallBack != NULL)
  {
    frameCallBack();
  }
  return true;
}

bool ssd1306SetCallBack(void (*p_func)(void))
{
  frameCallBack = p_func;

  return true;
}

void ssd1306Fill(uint16_t color)
{
  uint32_t i;

  for(i = 0; i < sizeof(ssd1306_buffer); i++)
  {
    ssd1306_buffer[i] = (color > 0) ? 0xFF : 0x00;
  }
}

bool ssd1306UpdateDraw(void)
{
  uint8_t i;

  for (i = 0; i < 8; i++)
  {
    ssd1306WriteCmd(0xB0 + i);
    ssd1306WriteCmd(0x00);
    ssd1306WriteCmd(0x10);

    if (i2cWriteBytes(i2c_ch, i2c_dev, 0x40, &ssd1306_buffer[SSD1306_WIDTH * i], SSD1306_WIDTH, 100) == false)
    {
      return false;
    }
  }

  return true;
}

void ssd1306DrawPixel(uint8_t x, uint8_t y, uint16_t color)
{
  if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
  {
    return;
  }


  if (color > 0)
  {
    ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
  }
  else
  {
    ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
  }
}

#endif
