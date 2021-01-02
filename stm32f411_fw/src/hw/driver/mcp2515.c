/*
 * mcp2515.c
 *
 *  Created on: 2020. 12. 30.
 *      Author: baram
 */


#include "mcp2515.h"
#include "cli.h"
#include "spi.h"
#include "gpio.h"


#ifdef _USE_HW_MCP2515





#define _PIN_DEF_CS     4


static const uint8_t MCP_SIDH = 0;
static const uint8_t MCP_SIDL = 1;
static const uint8_t MCP_EID8 = 2;
static const uint8_t MCP_EID0 = 3;
static const uint8_t MCP_DLC  = 4;
static const uint8_t MCP_DATA = 5;

static const uint8_t TXB_EXIDE_MASK = 0x08;
static const uint8_t DLC_MASK       = 0x0F;
//static const uint8_t RTR_MASK       = 0x40;

//static const uint8_t RXBnCTRL_RXM_STD    = 0x20;
//static const uint8_t RXBnCTRL_RXM_EXT    = 0x40;
static const uint8_t RXBnCTRL_RXM_STDEXT = 0x00;
static const uint8_t RXBnCTRL_RXM_MASK   = 0x60;
//static const uint8_t RXBnCTRL_RTR        = 0x08;
static const uint8_t RXB0CTRL_BUKT       = 0x04;
static const uint8_t RXB0CTRL_FILHIT_MASK = 0x03;
static const uint8_t RXB1CTRL_FILHIT_MASK = 0x07;
static const uint8_t RXB0CTRL_FILHIT = 0x00;
static const uint8_t RXB1CTRL_FILHIT = 0x01;


const uint8_t spi_ch = _DEF_SPI2;
static bool is_init = false;
static McpBaud is_baud = MCP_BAUD_125K;



uint8_t mcp2515ReadReg(uint8_t addr);
bool mcp2515ReadRegs(uint8_t addr, uint8_t *p_data, uint16_t length);
bool mcp2515WriteReg(uint8_t addr, uint8_t data);
bool mcp2515WriteRegs(uint8_t addr, uint8_t *p_data, uint16_t length);
bool mcp2515ModifyReg(uint8_t addr, uint8_t mask, uint8_t data);



#ifdef _USE_HW_CLI
static void cliMCP2515(cli_args_t *args);
#endif



static void TransferDoneISR(void)
{

}





bool mcp2515Init(void)
{
  bool ret = true;

  ret = spiBegin(spi_ch);
  spiAttachTxInterrupt(spi_ch, TransferDoneISR);

  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);

  is_init = mcp2515Reset();

  if (is_init == true)
  {
    uint8_t zeros[14];


    memset(zeros, 0, sizeof(zeros));
    mcp2515WriteRegs(MCP_TXB0CTRL, zeros, 14);
    mcp2515WriteRegs(MCP_TXB1CTRL, zeros, 14);
    mcp2515WriteRegs(MCP_TXB2CTRL, zeros, 14);

    mcp2515WriteReg(MCP_RXB0CTRL, 0);
    mcp2515WriteReg(MCP_RXB1CTRL, 0);

    mcp2515WriteReg(MCP_CANINTE, CANINTF_RX0IF | CANINTF_RX1IF | CANINTF_ERRIF | CANINTF_MERRF);

    // receives all valid messages using either Standard or Extended Identifiers that
    // meet filter criteria. RXF0 is applied for RXB0, RXF1 is applied for RXB1
    mcp2515ModifyReg(MCP_RXB0CTRL,
                   RXBnCTRL_RXM_MASK | RXB0CTRL_BUKT | RXB0CTRL_FILHIT_MASK,
                   RXBnCTRL_RXM_STDEXT | RXB0CTRL_BUKT | RXB0CTRL_FILHIT);
    mcp2515ModifyReg(MCP_RXB1CTRL,
                   RXBnCTRL_RXM_MASK | RXB1CTRL_FILHIT_MASK,
                   RXBnCTRL_RXM_STDEXT | RXB1CTRL_FILHIT);


    for (int i=0; i<MCP_FILTER_MAX; i++)
    {
      bool ext;

      if (i == 1)
      {
        ext = true;
      }
      else
      {
        ext = false;
      }
      mcp2515SetFilter(i, ext, 0);
    }
    for (int i=0; i<MCP_MASK_MAX; i++)
    {
      mcp2515SetFilterMask(i, true, 0);
    }


    mcp2515SetMode(MCP_MODE_LOOPBACK);
    mcp2515SetBaud(MCP_BAUD_125K);
  }

#ifdef _USE_HW_CLI
  cliAdd("mcp2515", cliMCP2515);
#endif
  return ret;
}

bool mcp2515Reset(void)
{
  bool ret;
  uint8_t buf[1];

  buf[0] = MCP_INST_RESET;
  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);
  ret = spiTransfer(spi_ch, buf, buf, 1, 10);
  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);

  delay(10);

  return ret;
}

bool mcp2515SetMode(McpMode mode)
{
  bool ret;
  uint8_t data = 0;
  uint8_t mask = 0;
  uint32_t pre_time;


  data = ((uint8_t)mode)<<5;
  mask = 0x07<<5;

  ret = mcp2515ModifyReg(MCP_CANCTRL, mask, data);

  pre_time = millis();
  while(millis()-pre_time < 10)
  {
    if ((mcp2515ReadReg(MCP_CANSTAT)&mask) == data)
    {
      break;
    }
  }
  return ret;
}

McpMode mcp2515GetMode(void)
{
  McpMode ret;
  uint8_t data;

  data = (mcp2515ReadReg(MCP_CANSTAT) >> 5) & 0x07;
  ret = data;

  return ret;
}

bool mcp2515SetBaud(McpBaud baud)
{
  bool ret;
  uint8_t cfg1;
  uint8_t cfg2;
  uint8_t cfg3;
  McpMode mode;

  mode = mcp2515GetMode();

  switch(baud)
  {
    case MCP_BAUD_125K:
      cfg1 = 0x01;
      cfg2 = 0xB1;
      cfg3 = 0x85;
      break;

    case MCP_BAUD_250K:
      cfg1 = 0x00;
      cfg2 = 0xB1;
      cfg3 = 0x85;
      break;

    case MCP_BAUD_500K:
      cfg1 = 0x00;
      cfg2 = 0x90;
      cfg3 = 0x82;
      break;

    case MCP_BAUD_1000K:
      cfg1 = 0x00;
      cfg2 = 0x80;
      cfg3 = 0x80;
      break;
  }

  mcp2515SetMode(MCP_MODE_CONFIG);

  ret = mcp2515WriteReg(MCP_CNF1, cfg1);
  ret = mcp2515WriteReg(MCP_CNF2, cfg2);
  ret = mcp2515WriteReg(MCP_CNF3, cfg3);
  is_baud = baud;

  mcp2515SetMode(mode);

  return ret;
}

McpBaud mcp2515GetBaud(void)
{
  return is_baud;
}

uint8_t mcp2515ReadStatus(void)
{
  uint8_t ret = 0;
  uint8_t buf[2];

  buf[0] = MCP_INST_READ_STATUS;
  buf[1] = 0;

  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);
  if (spiTransfer(spi_ch, buf, buf, 2, 10) == true)
  {
    ret = buf[1];
  }
  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);


  return ret;
}

uint8_t mcp2515ReadErrorFlags(void)
{
  return mcp2515ReadReg(MCP_EFLG);
}

void mcp2515PrepareID(uint8_t *buffer, const bool ext, const uint32_t id)
{
  uint16_t canid = (uint16_t)(id & 0x0FFFF);

  if (ext)
  {
    buffer[MCP_EID0] = (uint8_t) (canid & 0xFF);
    buffer[MCP_EID8] = (uint8_t) (canid >> 8);
    canid = (uint16_t)(id >> 16);
    buffer[MCP_SIDL] = (uint8_t) (canid & 0x03);
    buffer[MCP_SIDL] += (uint8_t) ((canid & 0x1C) << 3);
    buffer[MCP_SIDL] |= 0x08;
    buffer[MCP_SIDH] = (uint8_t) (canid >> 5);
  }
  else
  {
    buffer[MCP_SIDH] = (uint8_t) (canid >> 3);
    buffer[MCP_SIDL] = (uint8_t) ((canid & 0x07 ) << 5);
    buffer[MCP_EID0] = 0;
    buffer[MCP_EID8] = 0;
  }
}

bool mcp2515SetFilterMask(uint8_t index, const bool ext, const uint32_t data)
{
  bool ret;
  uint8_t buf[4];
  McpMode mode;

  if (index >= MCP_MASK_MAX) return false;


  mcp2515PrepareID(buf, ext, data);

  mode = mcp2515GetMode();
  mcp2515SetMode(MCP_MODE_CONFIG);

  ret = mcp2515WriteRegs(MCP_RXMSIDH(index), buf, 4);

  mcp2515SetMode(mode);

  return ret;
}

bool mcp2515SetFilter(uint8_t index, const bool ext, const uint32_t data)
{
  bool ret;
  uint8_t buf[4];
  McpMode mode;
  const uint8_t rxf_addr[MCP_FILTER_MAX] = {0x00, 0x04, 0x08, 0x10, 0x14, 0x18};

  if (index >= MCP_FILTER_MAX) return false;


  mcp2515PrepareID(buf, ext, data);

  mode = mcp2515GetMode();
  mcp2515SetMode(MCP_MODE_CONFIG);

  ret = mcp2515WriteRegs(rxf_addr[index], buf, 4);

  mcp2515SetMode(mode);

  return ret;
}

bool mcp2515SendMsg(mcp_msg_t *p_msg)
{
  bool ret = false;
  uint8_t tx_i;
  uint8_t reg;

  for (int i=0; i<3; i++)
  {
    reg = mcp2515ReadReg(MCP_TXBCTRL(i));

    if ((reg & (1<<3)) == 0x00)
    {
      tx_i = i;
      ret = true;
      break;
    }
  }

  if (ret != true)
  {
    return false;
  }

  uint8_t data[13];

  mcp2515PrepareID(data, p_msg->ext, p_msg->id);
  data[MCP_DLC] = p_msg->dlc;

  memcpy(&data[MCP_DATA], p_msg->data, p_msg->dlc);

  ret = mcp2515WriteRegs(MCP_TXBSIDH(tx_i), data, 5 + p_msg->dlc);

  ret = mcp2515ModifyReg(MCP_TXBCTRL(tx_i), (1<<3), (1<<3));
  if (ret == true)
  {
    reg = mcp2515ReadReg(MCP_TXBCTRL(tx_i));

    if (reg & (0x70))
    {
      ret = false;
    }
  }

  return ret;
}

bool mcp2515ReadMsg(mcp_msg_t *p_msg)
{
  bool ret = false;
  uint8_t rx_i = 0;
  uint8_t reg;

  reg = mcp2515ReadStatus();

  if (reg & (1<<0))
  {
    rx_i = 0;
  }
  else if (reg & (1<<1))
  {
    rx_i = 1;
  }
  else
  {
    return false;
  }


  uint8_t tbufdata[13];

  mcp2515ReadRegs(MCP_RXBSIDH(rx_i), tbufdata, 13);

  uint32_t id;

  id = (tbufdata[MCP_SIDH]<<3) + (tbufdata[MCP_SIDL]>>5);
  p_msg->ext = false;

  if ( (tbufdata[MCP_SIDL] & TXB_EXIDE_MASK) ==  TXB_EXIDE_MASK )
  {
    id = (id<<2) + (tbufdata[MCP_SIDL] & 0x03);
    id = (id<<8) + tbufdata[MCP_EID8];
    id = (id<<8) + tbufdata[MCP_EID0];
    p_msg->ext = true;
  }

  uint8_t dlc = (tbufdata[MCP_DLC] & DLC_MASK);
  if (dlc > 8)
  {
    return false;
  }

  p_msg->id = id;
  p_msg->dlc = dlc;

  for (int i=0; i<dlc; i++)
  {
    p_msg->data[i] = tbufdata[5+i];
  }

  ret = mcp2515ModifyReg(MCP_CANINTF, 1<<rx_i, 0);

  return ret;
}

uint8_t mcp2515ReadReg(uint8_t addr)
{
  uint8_t ret = 0;
  uint8_t buf[3];


  buf[0] = MCP_INST_READ;
  buf[1] = addr;

  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);
  if (spiTransfer(spi_ch, buf, buf, 3, 10) == true)
  {
    ret = buf[2];
  }
  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);
  return ret;
}

bool mcp2515ReadRegs(uint8_t addr, uint8_t *p_data, uint16_t length)
{
  bool ret;
  uint8_t buf[2];


  buf[0] = MCP_INST_READ;
  buf[1] = addr;

  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);

  spiTransfer(spi_ch, buf, buf, 2, 10);

  ret = spiTransfer(spi_ch, p_data, p_data, length, 10);

  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);

  return ret;
}

bool mcp2515WriteReg(uint8_t addr, uint8_t data)
{
  bool ret = 0;
  uint8_t buf[3];


  buf[0] = MCP_INST_WRITE;
  buf[1] = addr;
  buf[2] = data;

  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);
  ret = spiTransfer(spi_ch, buf, buf, 3, 10);
  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);
  return ret;
}

bool mcp2515WriteRegs(uint8_t addr, uint8_t *p_data, uint16_t length)
{
  bool ret;
  uint8_t buf[2];


  buf[0] = MCP_INST_WRITE;
  buf[1] = addr;

  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);

  spiTransfer(spi_ch, buf, buf, 2, 10);

  ret = spiTransfer(spi_ch, p_data, NULL, length, 10);

  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);

  return ret;
}

bool mcp2515ModifyReg(uint8_t addr, uint8_t mask, uint8_t data)
{
  bool ret = 0;
  uint8_t buf[4];


  buf[0] = MCP_INST_BIT_MODIFY;
  buf[1] = addr;
  buf[2] = mask;
  buf[3] = data;

  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);
  ret = spiTransfer(spi_ch, buf, buf, 4, 10);
  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);
  return ret;
}

#ifdef _USE_HW_CLI
void mcp2515Info(void)
{
  uint8_t reg;
  uint8_t reg_bits;

  reg = mcp2515ReadReg(0x0E);
  reg_bits = (reg>>5) & 0x07;

  cliPrintf("is_init \t: %d\n", is_init);

  cliPrintf("Operation Mode \t: ");
  if (reg_bits == 0x00 ) cliPrintf("Nomal");
  if (reg_bits == 0x01 ) cliPrintf("Sleep");
  if (reg_bits == 0x02 ) cliPrintf("Loopback");
  if (reg_bits == 0x03 ) cliPrintf("Listen-Only");
  if (reg_bits == 0x04 ) cliPrintf("Configuration");
  cliPrintf("\n");

  uint32_t Fosc;
  uint32_t BRP;
  uint32_t Tq;
  uint32_t SJW;
  uint32_t SyncSeg = 1;
  uint32_t PropSeg;
  uint32_t PhaseSeg1;
  uint32_t PhaseSeg2;
  uint32_t Tbit;
  uint32_t NBR;

  Fosc = 8;
  BRP = ((mcp2515ReadReg(0x2A) >> 0) & 0x3F) + 1;
  Tq  = 2*BRP*1000 / 8;
  SJW = ((mcp2515ReadReg(0x2A) >> 6) & 0x03) + 1;

  cliPrintf("Fosc \t\t: %dMhz\n", Fosc);
  cliPrintf("BRP  \t\t: %d\n", BRP);
  cliPrintf("Tq   \t\t: %d ns, %d Mhz\n", Tq, 1000/Tq);
  cliPrintf("SJW  \t\t: %d Tq\n", SJW);


  PropSeg   = ((mcp2515ReadReg(0x29) >> 0) & 0x07) + 1;
  PhaseSeg1 = ((mcp2515ReadReg(0x29) >> 3) & 0x07) + 1;
  PhaseSeg2 = ((mcp2515ReadReg(0x28) >> 0) & 0x07) + 1;
  Tbit      = SyncSeg + PropSeg + PhaseSeg1 + PhaseSeg2;
  NBR       = 1000000 / (Tbit*Tq);

  cliPrintf("SyncSeg        \t: %d Tq\n", SyncSeg);
  cliPrintf("PropSeg        \t: %d Tq\n", PropSeg);
  cliPrintf("PhaseSeg1(PS1) \t: %d Tq\n", PhaseSeg1);
  cliPrintf("PhaseSeg2(PS2) \t: %d Tq\n", PhaseSeg2);
  cliPrintf("Tbit           \t: %d Tq, %d ns\n", Tbit, Tbit * Tq);
  cliPrintf("Sample Point   \t: %d%% \n", (SyncSeg+PropSeg+PhaseSeg1) * 100 / Tbit);
  cliPrintf("NBR            \t: %d Kbps\n", NBR);

}

void cliMCP2515(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    mcp2515Info();
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "reg_info") == true)
  {
    cliPrintf("BFPCTRL    0x%02X : 0x%02X\n", 0x0C, mcp2515ReadReg(0x0C));
    cliPrintf("TXRTSCTRL  0x%02X : 0x%02X\n", 0x0D, mcp2515ReadReg(0x0D));
    cliPrintf("CANSTAT    0x%02X : 0x%02X\n", 0x0E, mcp2515ReadReg(0x0E));
    cliPrintf("CANCTRL    0x%02X : 0x%02X\n", 0x0F, mcp2515ReadReg(0x0F));
    cliPrintf("TEC        0x%02X : 0x%02X\n", 0x1C, mcp2515ReadReg(0x1C));
    cliPrintf("REC        0x%02X : 0x%02X\n", 0x1D, mcp2515ReadReg(0x1D));
    cliPrintf("CNF3       0x%02X : 0x%02X\n", 0x28, mcp2515ReadReg(0x28));
    cliPrintf("CNF2       0x%02X : 0x%02X\n", 0x29, mcp2515ReadReg(0x29));
    cliPrintf("CNF1       0x%02X : 0x%02X\n", 0x2A, mcp2515ReadReg(0x2A));
    cliPrintf("CANINTE    0x%02X : 0x%02X\n", 0x2B, mcp2515ReadReg(0x2B));
    cliPrintf("CANINTF    0x%02X : 0x%02X\n", 0x2C, mcp2515ReadReg(0x2C));
    cliPrintf("EFLG       0x%02X : 0x%02X\n", 0x2D, mcp2515ReadReg(0x2D));
    cliPrintf("TXB0CTRL   0x%02X : 0x%02X\n", 0x30, mcp2515ReadReg(0x30));
    cliPrintf("TXB1CTRL   0x%02X : 0x%02X\n", 0x40, mcp2515ReadReg(0x40));
    cliPrintf("TXB2CTRL   0x%02X : 0x%02X\n", 0x50, mcp2515ReadReg(0x50));
    cliPrintf("RXB0CTRL   0x%02X : 0x%02X\n", 0x60, mcp2515ReadReg(0x60));
    cliPrintf("RXB1CTRL   0x%02X : 0x%02X\n", 0x70, mcp2515ReadReg(0x70));

    uint32_t pre_time;

    pre_time = millis();
    for (int i=0; i<1000; i++)
    {
      mcp2515ReadReg(0x2A);
    }
    cliPrintf("%d ms\n", millis()-pre_time);

    ret = true;
  }

  if (args->argc == 3 && args->isStr(0, "read_reg") == true)
  {
    uint8_t  addr;
    uint16_t length;
    uint8_t buf[2];

    addr   = (uint8_t)args->getData(1);
    length = (uint16_t)args->getData(2);

    for (int i=0; i<length; i++)
    {
      if (mcp2515ReadRegs(addr+i, buf, 1) == true)
      {
        cliPrintf("0x%02X : 0x%02X\n", addr+i, buf[0]);
      }
      else
      {
        cliPrintf("spi fail\n");
        break;
      }

      ret = true;
    }
  }

  if (args->argc == 2 && args->isStr(0, "set_baud") == true)
  {
    bool update = false;
    if (args->isStr(1, "125k"))
    {
      mcp2515SetBaud(MCP_BAUD_125K);
      update = true;
    }
    if (args->isStr(1, "250k"))
    {
      mcp2515SetBaud(MCP_BAUD_250K);
      update = true;
    }
    if (args->isStr(1, "500k"))
    {
      mcp2515SetBaud(MCP_BAUD_500K);
      update = true;
    }
    if (args->isStr(1, "1000k"))
    {
      mcp2515SetBaud(MCP_BAUD_1000K);
      update = true;
    }

    if (update == true)
    {
      cliPrintf("Baud %s OK\n", args->getStr(1));
    }
    else
    {
      cliPrintf("Wrong Baud\n");
    }

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "set_mode") == true)
  {
    bool update = false;
    if (args->isStr(1, "normal"))
    {
      mcp2515SetMode(MCP_MODE_NORMAL);
      update = true;
    }
    if (args->isStr(1, "loopback"))
    {
      mcp2515SetMode(MCP_MODE_LOOPBACK);
      update = true;
    }
    if (args->isStr(1, "listen"))
    {
      mcp2515SetMode(MCP_MODE_LISTEN);
      update = true;
    }
    if (args->isStr(1, "config"))
    {
      mcp2515SetMode(MCP_MODE_CONFIG);
      update = true;
    }
    if (update == true)
    {
      cliPrintf("Mode %s OK\n", args->getStr(1));
    }
    else
    {
      cliPrintf("Wrong Mode\n");
    }

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "test") == true)
  {
    uint8_t rx_data;
    mcp_msg_t rx_msg;
    uint8_t cnt = 0;

    while(1)
    {
      if (mcp2515ReadMsg(&rx_msg) == true)
      {
        cliPrintf("id : 0x%X, dlc : %d, ext : %d, ",
                  rx_msg.id,
                  rx_msg.dlc,
                  rx_msg.ext);

        for (int i=0; i<rx_msg.dlc; i++)
        {
          cliPrintf("0x%02X ", rx_msg.data[i]);
        }
        cliPrintf("\n");
      }

      if (cliAvailable() > 0)
      {
        rx_data = cliRead();

        if (rx_data <= 0x20)
        {
          break;
        }
        if (rx_data == '1')
        {
          mcp_msg_t msg;
          msg.id  = 0x123;
          msg.ext = true;
          msg.dlc = 8;
          msg.data[0] = 0;
          msg.data[1] = 1;
          msg.data[2] = 2;
          msg.data[3] = 3;
          msg.data[4] = 4;
          msg.data[5] = 5;
          msg.data[6] = 6;
          msg.data[7] = cnt++;

          if (mcp2515SendMsg(&msg) == true)
          {
            cliPrintf("SendMsg OK\n");
          }
          else
          {
            cliPrintf("SendMsg Fail\n");
          }
        }
        if (rx_data == 's')
        {
          uint8_t status;

          cliPrintf("Status : ");

          status = mcp2515ReadStatus();
          for (int i=0; i<8; i++)
          {
            if (status & 0x80)
            {
              cliPrintf("1");
            }
            else
            {
              cliPrintf("0");
            }
            status <<= 1;
          }
          cliPrintf("  ErrFlag : 0x%X", mcp2515ReadErrorFlags());
          cliPrintf("\n");
        }
      }
    }

    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("mcp2515 info\n");
    cliPrintf("mcp2515 reg_info\n");
    cliPrintf("mcp2515 read_reg addr length\n");
    cliPrintf("mcp2515 set_baud 125k:250k:500k:1000k\n");
    cliPrintf("mcp2515 set_mode normal:loopback:listen:config\n");
    cliPrintf("mcp2515 test\n");
  }
}
#endif

#endif
