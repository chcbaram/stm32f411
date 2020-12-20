/*
 * ap.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "ap.h"




void apInit(void)
{
}

void apMain(void)
{
  while(1)
  {
    ledToggle(_DEF_LED1);
    delay(500);
  }
}

