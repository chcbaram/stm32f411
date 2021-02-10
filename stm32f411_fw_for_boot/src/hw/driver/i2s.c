/*
 * i2s.c
 *
 *  Created on: 2021. 1. 9.
 *      Author: baram
 */


#include "i2s.h"
#include "cli.h"
#include "files.h"
#include "gpio.h"
#include "lcd.h"


#ifdef _USE_HW_I2S



static bool is_init = false;
static bool is_started = false;

static uint32_t i2s_sample_rate = 16000;


#define I2S_BUF_LEN   (1024*4)
#define I2S_BUF_MS    (10)


typedef struct
{
  int16_t left;
  int16_t right;
} i2s_sample_t;

static volatile uint32_t q_in;
static volatile uint32_t q_out;
static volatile uint32_t q_len;
static volatile uint32_t q_buf_len;
static i2s_sample_t q_buf[I2S_BUF_LEN];


const int16_t q_buf_zero[I2S_BUF_LEN*2] = {0, };
//uint16_t q_buf_zero[I2S_BUF_LEN*2] = {0, };

I2S_HandleTypeDef hi2s5;
DMA_HandleTypeDef hdma_spi5_tx;


#ifdef _USE_HW_CLI
static void cliI2S(cli_args_t *args);
#endif

bool i2sInit(void)
{
  bool ret = true;


  hi2s5.Instance            = SPI5;
  hi2s5.Init.Mode           = I2S_MODE_MASTER_TX;
  hi2s5.Init.Standard       = I2S_STANDARD_PHILIPS;
  hi2s5.Init.DataFormat     = I2S_DATAFORMAT_16B;
  hi2s5.Init.MCLKOutput     = I2S_MCLKOUTPUT_DISABLE;
  hi2s5.Init.AudioFreq      = I2S_AUDIOFREQ_16K;
  hi2s5.Init.CPOL           = I2S_CPOL_LOW;
  hi2s5.Init.ClockSource    = I2S_CLOCK_PLL;
  hi2s5.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s5) != HAL_OK)
  {
    ret = false;
  }

#if 0
  for (int i=0; i<I2S_BUF_LEN*2; i+=2)
  {
    q_buf_zero[i + 0] = i % 1000;
    q_buf_zero[i + 1] = i % 1000;
  }
#endif

  q_buf_len = (i2s_sample_rate * 1) / (1000/I2S_BUF_MS);
  q_in  = 0;
  q_out = 0;
  q_len = I2S_BUF_LEN / q_buf_len;

  i2sStart();
  i2sStop();

  delay(50);
  gpioPinWrite(_PIN_GPIO_SPK_EN, true);

  is_init = ret;

#ifdef _USE_HW_CLI
  cliAdd("i2s", cliI2S);
#endif

  return ret;
}

bool i2sSetSampleRate(uint8_t ch, uint32_t freq)
{
  bool ret = true;


  i2s_sample_rate = freq;

  q_buf_len = (i2s_sample_rate * 1) / (1000/I2S_BUF_MS);
  q_in  = 0;
  q_out = 0;
  q_len = I2S_BUF_LEN / q_buf_len;

  gpioPinWrite(_PIN_GPIO_SPK_EN, false);

  hi2s5.Init.AudioFreq = freq;
  if (HAL_I2S_Init(&hi2s5) != HAL_OK)
  {
    ret = false;
  }

  i2sStart();
  i2sStop();

  delay(10);
  gpioPinWrite(_PIN_GPIO_SPK_EN, true);

  return ret;
}

bool i2sStart(void)
{
  HAL_StatusTypeDef status;
  I2S_HandleTypeDef *p_i2s = &hi2s5;

  q_in  = 0;
  q_out = 0;

  status = HAL_I2S_Transmit_DMA(p_i2s, (uint16_t *)q_buf_zero, q_buf_len*2);
  if (status == HAL_OK)
  {
    is_started = true;
  }

  return is_started;
}

bool i2sStop(void)
{
  is_started = false;

  return true;
}



void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  uint32_t len;

  if (is_started != true)
  {
    return;
  }

  len = (q_len + q_in - q_out) % q_len;

  if (len > 0)
  {
    HAL_I2S_Transmit_DMA(hi2s, (uint16_t *)&q_buf[q_out*q_buf_len], q_buf_len * 2);

    if (q_out != q_in)
    {
      q_out = (q_out + 1) % q_len;
    }
  }
  else
  {
    HAL_I2S_Transmit_DMA(hi2s, (uint16_t *)q_buf_zero, q_buf_len*2);
  }
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{

}


void HAL_I2S_MspInit(I2S_HandleTypeDef* i2sHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2sHandle->Instance==SPI5)
  {
  /* USER CODE BEGIN SPI5_MspInit 0 */
    __HAL_RCC_DMA2_CLK_ENABLE();
  /* USER CODE END SPI5_MspInit 0 */
    /* I2S5 clock enable */
    __HAL_RCC_SPI5_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2S5 GPIO Configuration
    PB0     ------> I2S5_CK
    PB1     ------> I2S5_WS
    PB8     ------> I2S5_SD
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI5;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2S5 DMA Init */
    /* SPI5_TX Init */
    hdma_spi5_tx.Instance = DMA2_Stream4;
    hdma_spi5_tx.Init.Channel = DMA_CHANNEL_2;
    hdma_spi5_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi5_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi5_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi5_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_spi5_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_spi5_tx.Init.Mode = DMA_NORMAL;
    hdma_spi5_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_spi5_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_spi5_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(i2sHandle,hdmatx,hdma_spi5_tx);

    /* I2S5 interrupt Init */
    HAL_NVIC_SetPriority(SPI5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SPI5_IRQn);
  /* USER CODE BEGIN SPI5_MspInit 1 */
    HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
  /* USER CODE END SPI5_MspInit 1 */
  }
}

void HAL_I2S_MspDeInit(I2S_HandleTypeDef* i2sHandle)
{

  if(i2sHandle->Instance==SPI5)
  {
  /* USER CODE BEGIN SPI5_MspDeInit 0 */

  /* USER CODE END SPI5_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI5_CLK_DISABLE();

    /**I2S5 GPIO Configuration
    PB0     ------> I2S5_CK
    PB1     ------> I2S5_WS
    PB8     ------> I2S5_SD
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_8);

    /* I2S5 DMA DeInit */
    HAL_DMA_DeInit(i2sHandle->hdmatx);

    /* I2S5 interrupt Deinit */
    HAL_NVIC_DisableIRQ(SPI5_IRQn);
  /* USER CODE BEGIN SPI5_MspDeInit 1 */

  /* USER CODE END SPI5_MspDeInit 1 */
  }
}



#ifdef _USE_HW_CLI

typedef struct wavfile_header_s
{
  char    ChunkID[4];     /*  4   */
  int32_t ChunkSize;      /*  4   */
  char    Format[4];      /*  4   */

  char    Subchunk1ID[4]; /*  4   */
  int32_t Subchunk1Size;  /*  4   */
  int16_t AudioFormat;    /*  2   */
  int16_t NumChannels;    /*  2   */
  int32_t SampleRate;     /*  4   */
  int32_t ByteRate;       /*  4   */
  int16_t BlockAlign;     /*  2   */
  int16_t BitsPerSample;  /*  2   */

  char    Subchunk2ID[4];
  int32_t Subchunk2Size;
} wavfile_header_t;

#include "mp3/mp3dec.h"



#define BLOCK_X_CNT     9
#define BLOCK_Y_CNT     12


#define READBUF_SIZE  1940

typedef struct
{
  uint8_t read_buf [READBUF_SIZE*2];
  uint8_t *read_ptr;
  int16_t out_buf  [2 * 1152];
  int     bytes_left;

  uint32_t pre_time_lcd;
  uint8_t  update_cnt;
  q15_t    buf_q15[512*2];

  uint8_t block_target[BLOCK_X_CNT];
  uint8_t block_peak[BLOCK_X_CNT];
  uint8_t block_value[BLOCK_X_CNT];

} i2s_cli_t;



static int fillReadBuffer(uint8_t *read_buf, uint8_t *read_ptr, int buf_size, int bytes_left, FILE *infile)
{
  int nRead;

  /* move last, small chunk from end of buffer to start, then fill with new data */
  memmove(read_buf, read_ptr, bytes_left);
  nRead = fread( read_buf + bytes_left, 1, buf_size - bytes_left, infile);
  /* zero-pad to avoid finding false sync word after last frame (from old data in readBuf) */
  if (nRead < buf_size - bytes_left)
  {
    memset(read_buf + bytes_left + nRead, 0, buf_size - bytes_left - nRead);
  }
  return nRead;
}

static void drawBlock(int16_t bx, int16_t by, uint16_t color)
{
  int16_t x;
  int16_t y;
  int16_t bw;
  int16_t bh;

  bw = (lcdGetWidth() / BLOCK_X_CNT);
  bh = (lcdGetHeight() / BLOCK_Y_CNT);

  x = bx*bw;
  y = lcdGetHeight() - bh*by - bh;

  lcdDrawFillRect(x, y, bw-2, bh-2, color);
}

static void lcdUpdate(i2s_cli_t *p_args)
{
  if (millis()-p_args->pre_time_lcd >= 50 && lcdDrawAvailable() == true)
  {
    p_args->pre_time_lcd = millis();

    lcdClearBuffer(black);


    for (int i=0; i<512; i++)
    {
      p_args->buf_q15[2*i  ] = p_args->out_buf[i*2];
      p_args->buf_q15[2*i+1] = 0;
    }

    arm_cfft_q15(&arm_cfft_sR_q15_len512, p_args->buf_q15, 0, 1);

    int16_t xi;

    xi = 0;
    for (int i=0; i<BLOCK_X_CNT; i++)
    {
      int16_t h;
      int16_t max_h;

      max_h = 0;
      for (int j=0; j<lcdGetWidth()/BLOCK_X_CNT/2; j++)
      {
        h = p_args->buf_q15[2*xi + 1];
        h = constrain(h, 0, 500);
        h = map(h, 0, 500, 0, 80);
        if (h > max_h)
        {
          max_h = h;
        }
        xi++;
      }
      h = map(max_h, 0, 80, 0, BLOCK_Y_CNT-1);

      p_args->block_target[i] = h;

      if (p_args->update_cnt%2 == 0)
      {
        if (p_args->block_peak[i] > 0)
        {
          p_args->block_peak[i]--;
        }
      }
      if (h >= p_args->block_peak[i])
      {
        p_args->block_peak[i] = h;
        p_args->block_value[i] = h;
      }
    }

    p_args->update_cnt++;

    for (int i=0; i<BLOCK_X_CNT; i++)
    {
      drawBlock(i, p_args->block_peak[i], red);

      if (p_args->block_value[i] > p_args->block_target[i])
      {
        p_args->block_value[i]--;
      }
      for (int j=0; j<p_args->block_value[i]; j++)
      {
        drawBlock(i, j, yellow);
      }
    }

#if 0
    int16_t xi;

    xi = 0;

    for (int i=0; i<lcdGetWidth(); i+=2)
    {
      int16_t h;

      h = p_args->buf_q15[4*xi + 1];
      h = constrain(h, 0, 500);
      h = map(h, 0, 500, 0, 80);

      lcdDrawLine(i, lcdGetHeight()-1, i, lcdGetHeight() - h - 1, white);

      xi++;
    }
#endif


    lcdRequestDraw();
  }
}

void cliI2S(cli_args_t *args)
{
  bool ret = false;
  i2s_cli_t i2s_args;


  memset(i2s_args.block_peak, 0, sizeof(i2s_args.block_peak));
  memset(i2s_args.block_value, 0, sizeof(i2s_args.block_value));
  memset(i2s_args.block_target, 0, sizeof(i2s_args.block_target));


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {

    cliPrintf("i2s init : %d\n", is_init);

    cliPrintf("q_in     : %d\n", q_in);
    cliPrintf("q_out    : %d\n", q_out);
    cliPrintf("q_buf_len: %d\n", q_buf_len);
    cliPrintf("q_len    : %d\n", q_len);
    cliPrintf("time     : %d ms\n", q_len * 10);

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "play_wav") == true)
  {
    char *file_name;
    FILE *fp;
    wavfile_header_t header;
    uint32_t r_len;

    file_name = args->getStr(1);

    cliPrintf("FileName      : %s\n", file_name);


    fp = fopen(file_name, "r");
    if (fp == NULL)
    {
      cliPrintf("fopen fail : %s\n", file_name);
      return;
    }
    fread(&header, sizeof(wavfile_header_t), 1, fp);

    cliPrintf("ChunkSize     : %d\n", header.ChunkSize);
    cliPrintf("Format        : %c%c%c%c\n", header.Format[0], header.Format[1], header.Format[2], header.Format[3]);
    cliPrintf("Subchunk1Size : %d\n", header.Subchunk1Size);
    cliPrintf("AudioFormat   : %d\n", header.AudioFormat);
    cliPrintf("NumChannels   : %d\n", header.NumChannels);
    cliPrintf("SampleRate    : %d\n", header.SampleRate);
    cliPrintf("ByteRate      : %d\n", header.ByteRate);
    cliPrintf("BlockAlign    : %d\n", header.BlockAlign);
    cliPrintf("BitsPerSample : %d\n", header.BitsPerSample);
    cliPrintf("Subchunk2Size : %d\n", header.Subchunk2Size);

    lcdClearBuffer(black);
    lcdSetFont(LCD_FONT_HAN);
    lcdPrintf(24,16*0, green, "WAV 플레이");

    lcdSetFont(LCD_FONT_07x10);
    lcdPrintf(20,18*1+10*0, white, "%s", file_name);
    lcdPrintf(20,18*1+10*1, white, "%d Khz", header.SampleRate/1000);
    lcdPrintf(20,18*1+10*2, white, "%d ch", header.NumChannels);
    lcdRequestDraw();

    i2sSetSampleRate(_DEF_I2S1, header.SampleRate);

    i2sStart();

    r_len = q_buf_len;

    int16_t buf_frame[q_buf_len*2];

    fseek(fp, sizeof(wavfile_header_t), SEEK_SET);

    while(cliKeepLoop())
    {
      uint32_t buf_len;
      int len;

      buf_len = ((q_len + q_in - q_out) % q_len);
      buf_len = (q_len - buf_len) - 1;

      if (buf_len > 0)
      {
        len = fread(buf_frame, r_len, 2*header.NumChannels, fp);

        if (len != r_len*2*header.NumChannels)
        {
          break;
        }

        uint32_t q_offset;

        q_offset = q_in*q_buf_len;

        for (int i=0; i<r_len; i++)
        {
          if (header.NumChannels == 2)
          {
            q_buf[q_offset + i].left  = buf_frame[i*2 + 0];
            q_buf[q_offset + i].right = buf_frame[i*2 + 1];
          }
          else
          {
            q_buf[q_offset + i].left  = buf_frame[i];
            q_buf[q_offset + i].right = buf_frame[i];
          }
        }

        if (((q_in + 1) % q_len) != q_out)
        {
          q_in = (q_in+1) % q_len;
        }
      }
    }

    i2sStop();

    fclose(fp);

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "play_mp3") == true)
  {
    HMP3Decoder h_dec;
    char *file_name;

    file_name = args->getStr(1);


    h_dec = MP3InitDecoder();

    if (h_dec != 0)
    {
      MP3FrameInfo frameInfo;
      FILE *fp;

      cliPrintf("init ok\n");


      fp = fopen(file_name, "r");

      if( fp == NULL )
      {
        cliPrintf( "File is null\n" );
        return;
      }
      else
      {
        cliPrintf( "File is not null\n" );
      }

      //int offset;
      int err;
      int n_read;

      //fread( buf, 4096, 1, fp );

      i2s_args.bytes_left = 0;
      i2s_args.read_ptr = i2s_args.read_buf;

      n_read = fillReadBuffer(i2s_args.read_buf, i2s_args.read_ptr, READBUF_SIZE, i2s_args.bytes_left, fp);
      i2s_args.bytes_left += n_read;
      i2s_args.read_ptr = i2s_args.read_buf;

      n_read = MP3FindSyncWord(i2s_args.read_ptr, READBUF_SIZE);
      cliPrintf("Offset: %d\n", n_read);

      i2s_args.bytes_left -= n_read;
      i2s_args.read_ptr += n_read;

      n_read = fillReadBuffer(i2s_args.read_buf, i2s_args.read_ptr, READBUF_SIZE, i2s_args.bytes_left, fp);
      i2s_args.bytes_left += n_read;
      i2s_args.read_ptr = i2s_args.read_buf;


      err = MP3GetNextFrameInfo(h_dec, &frameInfo, i2s_args.read_ptr);
      if (err != ERR_MP3_INVALID_FRAMEHEADER)
      {
        cliPrintf("samplerate     %d\n", frameInfo.samprate);
        cliPrintf("bitrate        %d\n", frameInfo.bitrate);
        cliPrintf("nChans         %d\n", frameInfo.nChans);
        cliPrintf("outputSamps    %d\n", frameInfo.outputSamps);
        cliPrintf("bitsPerSample  %d\n", frameInfo.bitsPerSample);

        i2sSetSampleRate(_DEF_I2S1, frameInfo.samprate);

        q_buf_len = frameInfo.outputSamps / frameInfo.nChans;
        q_in  = 0;
        q_out = 0;
        q_len = I2S_BUF_LEN / q_buf_len;

        i2sStart();
      }

      while(cliKeepLoop())
      {
        if (i2s_args.bytes_left < READBUF_SIZE)
        {
          n_read = fillReadBuffer(i2s_args.read_buf, i2s_args.read_ptr, READBUF_SIZE, i2s_args.bytes_left, fp);
          if (n_read == 0 )
          {
            break;
          }
          i2s_args.bytes_left += n_read;
          i2s_args.read_ptr = i2s_args.read_buf;
        }


        n_read = MP3FindSyncWord(i2s_args.read_ptr, i2s_args.bytes_left);
        if (n_read >= 0)
        {
          i2s_args.read_ptr += n_read;
          i2s_args.bytes_left -= n_read;


          //fill the inactive outbuffer
          err = MP3Decode(h_dec, &i2s_args.read_ptr, (int*) &i2s_args.bytes_left, i2s_args.out_buf, 0);

          if (err)
          {
            // sometimes we have a bad frame, lets just nudge forward one byte
            if (err == ERR_MP3_INVALID_FRAMEHEADER)
            {
              i2s_args.read_ptr   += 1;
              i2s_args.bytes_left -= 1;
            }
          }
          else
          {

            uint32_t pre_time;
            uint32_t valid_len;

            pre_time = millis();
            while(1)
            {
              valid_len = (q_len - 1) - ((q_len + q_in - q_out) % q_len);

              if (valid_len > 0)
              {
                break;
              }
              if (millis()-pre_time >= 100)
              {
                break;
              }
            }


            uint32_t q_offset;

            q_offset   = q_in*q_buf_len;

            for (int j=0; j<q_buf_len; j++)
            {
              q_buf[q_offset + j].left  = i2s_args.out_buf[j*2 + 0];
              q_buf[q_offset + j].right = i2s_args.out_buf[j*2 + 1];
            }
            if (((q_in + 1) % q_len) != q_out)
            {
              q_in = (q_in+1) % q_len;
            }

            lcdUpdate(&i2s_args);
          }
        }
      }
      i2sStop();
      fclose(fp);
      ret = true;
    }
  }



  if (ret != true)
  {
    cliPrintf("i2s info\n");
    cliPrintf("i2s play_wav filename\n");
    cliPrintf("i2s play_mp3 filename\n");
  }
}
#endif

#endif
