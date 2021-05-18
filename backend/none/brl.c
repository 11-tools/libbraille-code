/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "driver.h"
#include "serial.h" /* for brli_delay */

BRLDRV_API const char *
brli_drvinfo(brli_term *term, brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      return "none";
    case BRL_TERMINAL:
      return "None";
    default:
      return NULL;
    }
}

BRLDRV_API char
brli_drvinit(brli_term *brl, char type, const char *dev)
{
  brl->width = 40;

  brl->display = (unsigned char *)malloc(brl->width);
  brl->display_ascii = (unsigned char *)malloc(brl->width);
  if(!brl->display || !brl->display_ascii)
    {
      brli_seterror("Error allocating buffers: %s", strerror(errno));
      brli_drvclose(brl);
      return 0;
    }
  return 1;
}

BRLDRV_API char
brli_drvclose(brli_term *brl)
{
  if(brl->display)
    free(brl->display);
  if(brl->display_ascii)
    free(brl->display_ascii);
  return 1;
}

BRLDRV_API char
brli_drvwrite(brli_term *brl)
{
  return 1;
}

BRLDRV_API char
brli_drvstatus(brli_term *term)
{
  return 1;
}

BRLDRV_API signed char
brli_drvread(brli_term *term, brl_key *key)
{
  if(term->cc_min)
    {
      brli_delay(10000);
    }
  else if(term->cc_time)
    {
      brli_delay(term->cc_time * 100);
    }

  return 0;
}
