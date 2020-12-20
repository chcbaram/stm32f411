/*
 * flash.c
 *
 *  Created on: 2020. 12. 14.
 *      Author: baram
 */


#include "flash.h"


#ifdef _USE_HW_FLASH


#define FLASH_SECTOR_MAX          64



typedef struct
{
  uint32_t addr;
  uint16_t length;
} flash_tbl_t;


static flash_tbl_t flash_tbl[FLASH_SECTOR_MAX];



static bool flashInSector(uint16_t sector_num, uint32_t addr, uint32_t length);


bool flashInit(void)
{
  for (int i=0; i<FLASH_SECTOR_MAX; i++)
  {
    flash_tbl[i].addr   = 0x8000000 + i*1024;
    flash_tbl[i].length = 1024;
  }

  return true;
}

bool flashErase(uint32_t addr, uint32_t length)
{
  bool ret = false;
  HAL_StatusTypeDef status;
  FLASH_EraseInitTypeDef init;
  uint32_t page_error;

  int16_t  start_sector_num = -1;
  uint32_t sector_count = 0;



  for (int i=0; i<FLASH_SECTOR_MAX; i++)
  {
    if (flashInSector(i, addr, length) == true)
    {
      if (start_sector_num < 0)
      {
        start_sector_num = i;
      }
      sector_count++;
    }
  }


  if (sector_count > 0)
  {
    HAL_FLASH_Unlock();

    init.TypeErase   = FLASH_TYPEERASE_PAGES;
    init.Banks       = FLASH_BANK_1;
    init.PageAddress = flash_tbl[start_sector_num].addr;
    init.NbPages     = sector_count;

    status = HAL_FLASHEx_Erase(&init, &page_error);
    if (status == HAL_OK)
    {
      ret = true;
    }

    HAL_FLASH_Lock();
  }

  return ret;
}

bool flashWrite(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret = true;
  HAL_StatusTypeDef status;


  if (addr%2 != 0)
  {
    return false;
  }

  HAL_FLASH_Unlock();

  for (int i=0; i<length; i+=2)
  {
    uint16_t data;

    data  = p_data[i+0] << 0;
    data |= p_data[i+1] << 8;

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, (uint64_t)data);
    if (status != HAL_OK)
    {
      ret = false;
      break;
    }
  }

  HAL_FLASH_Lock();

  return ret;
}

bool flashRead(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret = true;
  uint8_t *p_byte = (uint8_t *)addr;


  for (int i=0; i<length; i++)
  {
    p_data[i] = p_byte[i];
  }

  return ret;
}

bool flashInSector(uint16_t sector_num, uint32_t addr, uint32_t length)
{
  bool ret = false;

  uint32_t sector_start;
  uint32_t sector_end;
  uint32_t flash_start;
  uint32_t flash_end;


  sector_start = flash_tbl[sector_num].addr;
  sector_end   = flash_tbl[sector_num].addr + flash_tbl[sector_num].length - 1;
  flash_start  = addr;
  flash_end    = addr + length - 1;


  if (sector_start >= flash_start && sector_start <= flash_end)
  {
    ret = true;
  }

  if (sector_end >= flash_start && sector_end <= flash_end)
  {
    ret = true;
  }

  if (flash_start >= sector_start && flash_start <= sector_end)
  {
    ret = true;
  }

  if (flash_end >= sector_start && flash_end <= sector_end)
  {
    ret = true;
  }

  return ret;
}


#endif
