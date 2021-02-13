/*
 * ap.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "ap.h"
#include "boot/boot.h"


enum
{
  FILE_TYPE_FW,
  FILE_TYPE_BIN,
};



#define TX_BLOCK_LENGTH     512




int32_t getFileSize(char *file_name);
bool addTagToBin(char *src_filename, char *dst_filename);



void apInit(void)
{
  logPrintf("stm32cli V210213R1\n\n");

  cliOpen(_DEF_UART1, 57600);
}

void apMain(int argc, char *argv[])
{
  bool ret;
  uint8_t err_code;
  uint8_t  uart_ch;
  char    *uart_port;
  uint32_t uart_baud;
  uint8_t boot_ver[32];
  uint8_t boot_name[32];
  uint8_t firm_ver[32];
  uint8_t firm_name[32];
  uint32_t pre_time;
  uint32_t exe_time;
  uint8_t  file_type;
  uint32_t file_addr;
  int32_t  file_size;
  char     file_name[256];
  char     dst_filename[256];
  bool     file_run = false;
  FILE     *fp;


  (void)file_type;

  if (argc != 7)
  {
    logPrintf("stm32cli com1 57600 type[fw:bin] 0x8010000 file_name run[0:1]\n");
    apExit();
  }


  //-- 파라메터 처리
  //
  uart_ch   = _DEF_UART2;
  uart_port = argv[1];
  uart_baud = (int32_t)strtoul(argv[2], (char **)NULL, (int) 0);

  logPrintf("uart ch   : %d\n", uart_ch);
  logPrintf("uart port : %s\n", uart_port);
  logPrintf("uart baud : %d bps\n", uart_baud);


  if (strcmp(argv[3], "fw") == 0)
  {
    file_type = FILE_TYPE_FW;
    logPrintf("file type : firmware\n");
  }
  else if (strcmp(argv[3], "bin") == 0)
  {
    file_type = FILE_TYPE_BIN;
    logPrintf("file type : binary\n");
  }
  else
  {
    logPrintf("file type error\n");
    apExit();
  }

  file_addr = (uint32_t)strtoul(argv[4], (char **)NULL, (int) 0);
  logPrintf("file addr : 0x%X\n", file_addr);

  strcpy(file_name, argv[5]);
  logPrintf("file name : %s\n", file_name);

  file_size = getFileSize(file_name);
  if (file_size <= 0)
  {
    logPrintf("file size error\n");
    apExit();
  }
  logPrintf("file size : %d bytes\n", file_size);

  if (*argv[6] == '1')
  {
    file_run = true;
    logPrintf("file run  : true\n");
  }
  else
  {
    logPrintf("file run  : false\n");
  }


  if (file_type == FILE_TYPE_FW)
  {
    logPrintf("\r\nadd tag...\r\n");
    strcpy(dst_filename, file_name);
    strcat(dst_filename, ".fw");

    if(addTagToBin(file_name, dst_filename) != true)
    {
      fprintf( stderr, "  Add tag info to binary Fail! \n");
      apExit();
    }


    strcpy(file_name, dst_filename);
  }


  //-- boot 시작
  //
  ret = bootInit(uart_ch, uart_port, uart_baud);
  if (ret != true)
  {
    logPrintf("bootInit Fail\n");
    apExit();
  }

  logPrintf("\n\nboot start...\n");

  while(1)
  {
    //-- 부트로더 버전 읽기
    //
    err_code = bootCmdReadBootVersion(boot_ver);
    if (err_code != CMD_OK)
    {
      logPrintf("bootCmdReadBootVersion fail : %d\n", err_code);
      break;
    }
    logPrintf("boot ver \t: %s\n",  boot_ver);


    //-- 부트로더 이름 읽기
    //
    err_code = bootCmdReadBootName(boot_name);
    if (err_code != CMD_OK)
    {
      logPrintf("bootCmdReadBootName fail : %d\n", err_code);
      break;
    }
    logPrintf("boot name \t: %s\n", boot_name);


    //-- 펌웨어 버전 읽기
    //
    err_code = bootCmdReadFirmVersion(firm_ver);
    if (err_code != CMD_OK)
    {
      logPrintf("bootCmdReadFirmVersion fail : %d\n", err_code);
      break;
    }
    logPrintf("firm ver \t: %s\n",  firm_ver);


    //-- 펌웨어 이름 읽기
    //
    err_code = bootCmdReadFirmName(firm_name);
    if (err_code != CMD_OK)
    {
      logPrintf("bootCmdReadFirmName fail : %d\n", err_code);
      break;
    }
    logPrintf("firm name \t: %s\n", firm_name);


    //-- Flash Erase
    //
    logPrintf("flash erase \t: ");
    pre_time = millis();
    err_code = bootCmdFlashErase(file_addr, file_size, 5000);
    exe_time = millis()-pre_time;
    if (err_code != CMD_OK)
    {
      logPrintf("\nbootCmdFlashErase fail : %d\n", err_code);
      break;
    }
    logPrintf("OK (%dms)\n", exe_time);



    //-- Flash Write
    //
    if ((fp = fopen(file_name, "rb")) == NULL)
    {
      logPrintf("Unable to open %s\n", file_name);
      apExit();
    }


    uint32_t addr;
    uint32_t len;
    bool     write_done = false;
    uint8_t  tx_buf[TX_BLOCK_LENGTH];
    uint16_t write_percent;
    uint16_t pre_percent = 0;

    addr = file_addr;
    pre_time = millis();
    while(1)
    {
      if (!feof(fp))
      {
        len = fread(tx_buf, 1, TX_BLOCK_LENGTH, fp);

        err_code = bootCmdFlashWrite(addr, tx_buf, len, 1000);
        if (err_code == CMD_OK)
        {
          addr += len;

          write_percent = (addr-file_addr) * 100 / file_size;

          if ((write_percent/10) != pre_percent)
          {
            logPrintf("flash write \t: %d%%\r", write_percent);
            pre_percent = (write_percent/10);
          }


          if ((addr-file_addr) >= file_size)
          {
            write_done = true;
            break;
          }
        }
        else
        {
          logPrintf("bootCmdFlashWrite fail : 0x%x, %d\n", addr, err_code);
          break;
        }
      }
      else
      {
        break;
      }
    }

    fclose(fp);
    exe_time = millis()-pre_time;

    logPrintf("\n");

    if (write_done == true)
    {
      logPrintf("flash write \t: OK (%dms)\n", exe_time);

      if (file_run == true && file_type == FILE_TYPE_FW)
      {
        pre_time = millis();
        err_code = bootCmdJumpToFw();
        exe_time = millis()-pre_time;
        if (err_code == CMD_OK)
        {
          logPrintf("jump to fw \t: OK (%dms)\n", exe_time);
        }
        else
        {
          logPrintf("jump to fw \t: fail, %d\n", err_code);
        }
        bootDeInit(uart_ch);
      }
    }
    else
    {
      logPrintf("flash write \t: Fail \n");
    }

    break;
  }


  apExit();
}

void apExit(void)
{
  for (int i=0; i<UART_MAX_CH; i++)
  {
    uartClose(i);
  }
  printf("\ncli exit\n");
  exit(0);
}

int32_t getFileSize(char *file_name)
{
  int32_t ret = -1;
  FILE *fp;

  if ((fp = fopen( file_name, "rb")) == NULL)
  {
    fprintf( stderr, "Unable to open %s\n", file_name );
    return -1;
  }
  else
  {
    fseek( fp, 0, SEEK_END );
    ret = ftell( fp );
    fclose(fp);
  }

  return ret;
}

bool addTagToBin(char *src_filename, char *dst_filename)
{
  FILE    *p_fd;
  uint8_t *buf;
  size_t   src_len;
  uint16_t t_crc = 0;
  firm_tag_t *p_tag;


  if (!strcmp(src_filename, dst_filename))
  {
    fprintf( stderr, "  src file(%s) and dst file(%s) is same! \n", src_filename, dst_filename );
    apExit();
  }

  /* Open src file */
  if ((p_fd = fopen(src_filename, "rb")) == NULL)
  {
    fprintf( stderr, "  unable to open src file(%s)\n", src_filename );
    exit( 1 );
  }


  fseek( p_fd, 0, SEEK_END );
  src_len = ftell( p_fd );
  fseek( p_fd, 0, SEEK_SET );


  if ((buf = (uint8_t *) calloc(src_len, sizeof(uint8_t))) == NULL)
  {
    fclose(p_fd);
    fprintf( stderr, "  malloc Error \n");
    apExit();
  }

  /* Copy read fp to buf */
  if(fread( &buf[0], 1, src_len, p_fd ) != src_len)
  {
    fclose(p_fd);
    free(buf);
    fprintf( stderr, "  length is wrong! \n" );
    apExit();
  }
  fclose(p_fd);


  p_tag = (firm_tag_t *)buf;


  if (p_tag->magic_number == FLASH_MAGIC_NUMBER)
  {
    free(buf);
    fprintf( stderr, "  already magic number\n");
    return true;
  }
  if (p_tag->magic_number != 0xAAAA5555)
  {
    free(buf);
    fprintf( stderr, "  wrong magic number 0x%X \n", p_tag->magic_number);
    return false;
  }



  /* Calculate CRC16 */
  size_t i;
  uint32_t tag_size;

  tag_size = p_tag->size_tag;

  for (i = 0; i<src_len-tag_size; i++)
  {
    utilUpdateCrc(&t_crc, buf[tag_size + i]);
  }

  p_tag->magic_number     = FLASH_MAGIC_NUMBER;
  p_tag->tag_flash_start  = p_tag->addr_fw;
  p_tag->tag_flash_end    = p_tag->addr_fw + (src_len - tag_size);
  p_tag->tag_flash_length = p_tag->tag_flash_end - p_tag->tag_flash_start;
  p_tag->tag_length       = tag_size;
  strcpy((char *)p_tag->tag_date_str, __DATE__);
  strcpy((char *)p_tag->tag_time_str, __TIME__);
  p_tag->tag_flash_crc = t_crc;



  /* Store data to dst file */
  if ((p_fd = fopen(dst_filename, "wb")) == NULL)
  {
   free(buf);
   fprintf( stderr, "  unable to open dst file(%s)\n", dst_filename );
   apExit();
  }

  if(fwrite(buf, 1, src_len, p_fd) != src_len)
  {
   fclose(p_fd);
   free(buf);
   //_unlink(dst_filename);
   fprintf( stderr, "  total write fail! \n" );
   apExit();
  }

  printf("created file  : %s (%d KB)\n", dst_filename, (int)((src_len)/1024) );
  printf("tag fw start  : 0x%08X \n", p_tag->tag_flash_start);
  printf("tag fw end    : 0x%08X \n", p_tag->tag_flash_end);
  printf("tag crc       : 0x%04X \n", p_tag->tag_flash_crc);
  printf("tag date      : %s \n", p_tag->tag_date_str);
  printf("tag time      : %s \n", p_tag->tag_time_str);

  fclose(p_fd);
  free(buf);

  return true;
}
