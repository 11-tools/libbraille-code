/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#include "config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "driver.h"
#include "serial.h" /* for brli_delay */

BRLDRV_API const char *
brli_drvinfo(brli_term *term, brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      return "fake";
    case BRL_TERMINAL:
      return "Fake";
    default:
      return NULL;
    }
}

BRLDRV_API char
brli_drvinit(brli_term *term, char type, const char *pathname)
{
  term->width = 40;

  term->display = (unsigned char *)malloc(term->width);
  term->display_ascii = (unsigned char *)malloc(term->width);
  if(!term->display || !term->display_ascii)
    {
      braillei_seterror("Error allocating buffers: %s", strerror(errno));
      brli_drvclose(term);
      return 0;
    }
  return 1;
}

BRLDRV_API char
brli_drvclose(brli_term *term)
{
  if(term->display)
    free(term->display);
  if(term->display_ascii)
    free(term->display_ascii);
  return 1;
}

BRLDRV_API char 
brli_drvwrite(brli_term *term)
{
  return 1;
}

BRLDRV_API char
brli_drvstatus(brli_term *term)
{
  return 1;
}

BRLDRV_API char
brli_drvread(brli_term *term, brl_key *key)
{
  if(term->cc_min)
    {
      brli_delay(10000);
    }
  else if(term->cc_time)
    {
      brli_delay(term->cc_time * 10);
    }

  return 0;
}
