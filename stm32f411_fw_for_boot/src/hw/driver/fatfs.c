/*
 * fatfs.c
 *
 *  Created on: 2020. 12. 25.
 *      Author: baram
 */


#include "fatfs.h"
#include "cli.h"

#ifdef _USE_HW_FATFS
#include "ff_gen_drv.h"
#include "sd_diskio.h"



static bool is_init = false;

FATFS SDFatFs;  /* File system object for SD card logical drive */
char SDPath[4]; /* SD card logical drive path */


#ifdef _USE_HW_CLI
static void cliFatfs(cli_args_t *args);
#endif

bool fatfsInit(void)
{
  bool ret = true;


  if(FATFS_LinkDriver(&SD_Driver, SDPath) == 0)
  {
    if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) == FR_OK)
    {
      is_init = true;
    }
  }

#ifdef _USE_HW_CLI
  cliAdd("fatfs", cliFatfs);
#endif

  return ret;
}


#ifdef _USE_HW_CLI

FRESULT fatfsDir(char* path)
{
  FRESULT res;
  DIR dir;
  FILINFO fno;


  res = f_opendir(&dir, path);                       /* Open the directory */
  if (res == FR_OK)
  {
    for (;;)
    {
      res = f_readdir(&dir, &fno);                   /* Read a directory item */
      if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
      if (fno.fattrib & AM_DIR)
      {                    /* It is a directory */
        cliPrintf(" %s/%s \n", path, fno.fname);
      }
      else
      {                                       /* It is a file. */
        cliPrintf(" %s/%32s \t%d bytes\n", path, fno.fname, (int)fno.fsize);
      }
    }
    f_closedir(&dir);
  }

  return res;
}

void cliFatfs(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    cliPrintf("fatfs init \t: %d\n", is_init);

    if (is_init == true)
    {
      FATFS *fs;
       DWORD fre_clust, fre_sect, tot_sect;
       FRESULT res;

       /* Get volume information and free clusters of drive 1 */
       res = f_getfree("", &fre_clust, &fs);
       if (res == FR_OK)
       {
         /* Get total sectors and free sectors */
         tot_sect = (fs->n_fatent - 2) * fs->csize;
         fre_sect = fre_clust * fs->csize;

         /* Print the free space (assuming 512 bytes/sector) */
         cliPrintf("%10lu KiB total drive space.\n%10lu KiB available.\n", tot_sect / 2, fre_sect / 2);
       }
       else
       {
         cliPrintf(" err : %d\n", res);
       }
    }

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "dir") == true)
  {
    FRESULT res;

    res = fatfsDir("/");
    if (res != FR_OK)
    {
      cliPrintf(" err : %d\n", res);
    }

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "test") == true)
  {
    FRESULT fp_ret;
    FIL log_file;
    uint32_t pre_time;

    pre_time = millis();
    fp_ret = f_open(&log_file, "1.csv", FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
    if (fp_ret == FR_OK)
    {
      f_printf(&log_file, "test1, ");
      f_printf(&log_file, "test2, ");
      f_printf(&log_file, "test3, ");
      f_printf(&log_file, ", ");
      f_printf(&log_file, "\n");

      for (int i=0; i<8; i++)
      {
        f_printf(&log_file, "%d \n", i);
      }

      f_rewind(&log_file);


      UINT len;
      uint8_t data;

      while(cliKeepLoop())
      {
        len = 0;
        fp_ret = f_read (&log_file, &data, 1, &len);

        if (fp_ret != FR_OK)
        {
          break;
        }
        if (len == 0)
        {
          break;
        }

        cliPrintf("%c", data);
      }

      f_close(&log_file);
    }
    else
    {
      cliPrintf("f_open fail\r\n");
    }
    cliPrintf("%d ms\r\n", millis()-pre_time);

    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("fatfs info\n");
    cliPrintf("fatfs dir\n");
    cliPrintf("fatfs test\n");
  }
}

#endif



#endif
