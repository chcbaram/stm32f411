/*
 * cdc.c
 *
 *  Created on: 2020. 12. 11.
 *      Author: baram
 */


#include "cdc.h"


#ifdef _USE_HW_CDC


static bool is_init = false;

bool cdcInit(void)
{
  bool ret = true;


  is_init = true;

  return ret;
}

bool cdcIsInit(void)
{
  return is_init;
}
#endif
