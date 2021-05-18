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
#include <errno.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "driver.h"
#include "serial.h"

BRLDRV_API const char *
brli_drvinfo(brli_term *term, brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      return "braillenote";
    case BRL_TERMINAL:
      return "BrailleNote (PDI)";
    default:
      return NULL;
    }
}

BRLDRV_API char
brli_drvinit(brli_term *term, char type, const char *pathname)
{
  struct termios tiodata;
  unsigned char data[3];
  int testnum;

  if(brli_open(term, pathname) == -1)
    {
      brli_seterror("Port open failed: %s: %s", pathname, brli_geterror());
      return 0;
    }

  /* If we got it open, get the attributes of the port */
  if(brli_tcgetattr(term, &tiodata))
    {
      brli_seterror("brli_tcgetattr failed on %s: %s", pathname, brli_geterror());
      brli_close(term);
      return 0;
    }

  /* Serial port parameters */
  tiodata.c_iflag = IGNPAR;
  tiodata.c_cflag = CLOCAL | CREAD | CS8 | CSTOPB;
  tiodata.c_oflag = 0;
  tiodata.c_lflag = 0;

  /* Initial timeouts */
  tiodata.c_cc[VMIN] = 0;
  tiodata.c_cc[VTIME] = 3; /* we wait 300ms before getting an answer */

  if(brli_cfsetispeed(&tiodata, B38400) ||
     brli_cfsetospeed(&tiodata, B38400) ||
     brli_tcsetattr(term, TCSANOW, &tiodata))
    {
      brli_seterror("Port init failed: %s: %s", pathname, brli_geterror());
      brli_close(term);
      return 0;
    }
  brli_log(LOG_NOTICE, "Port init success: %s", pathname);

  testnum = 0;
  while(1)
    {
      int size;

      /* Autodetect BrailleNote and get size */
      if(brli_swrite(term, (unsigned char *)"\x1b?", 2) < 2) /* Initialization string */
	{
	  brli_seterror("Error writing to port");
	  brli_drvclose(term);
	  return 0;
	}

      size = brli_sread(term, data, 1);
      if(size < 1)
	{
	  brli_log(LOG_DEBUG, "No answer from display");
	  brli_seterror("No BrailleNote display detected");
	  brli_drvclose(term);
	  return 0;
	}
      else if(data[0] == 0xff && testnum == 0) /* BrailleNote may be in suspend mode */
	{
	  testnum++;
	  brli_log(LOG_INFO, "BrailleNote in suspend mode, retrying");
	  
	  if(brli_timeout(term, 0, 10) == -1) /* we wait 1s before getting an answer */
	    {
	      brli_seterror("Changing port timeouts failed");
	      return 0;
	    }
	  continue;
	}
      else if(data[0] == 0x86)
	{
	  size = brli_sread(term, data + 1, 2);
	  if(size < 2)
	    {
	      brli_seterror("Error reading size from display");
	      brli_drvclose(term);
	      return 0;
	    }
	  // status_cell = data[1]; /* number of status cells */
	  term->width = data[1] + data[2]; /* number of cells */
	  if(term->width > 40)
	    {
	      brli_seterror("Terminal returned invalid size");
	      term->width = 0;
	      brli_drvclose(term);
	      return 0;
	    }
	  brli_log(LOG_INFO, "Detected BrailleNote display");
	  break;
	}
      else
	{
	  brli_log(LOG_NOTICE, "Bad display type 0x%x", data[0]);
	  brli_seterror("No BrailleNote display detected");
	  brli_drvclose(term);
	  return 0;
	}
    }

  term->display = (unsigned char *)malloc(term->width);
  term->display_ascii = (unsigned char *)malloc(term->width);
  if(!term->display || !term->display_ascii)
    {
      brli_seterror("%s", strerror(errno));
      brli_drvclose(term);
      return 0;
    }
  return 1;
}

BRLDRV_API char
brli_drvclose(brli_term *term)
{
  brli_close(term);
  if(term->display)
    free(term->display);
  if(term->display_ascii)
    free(term->display_ascii);
  term->width = -1;
  return 1;
}

BRLDRV_API char
brli_drvstatus(brli_term *term)
{
  return 1;
}

BRLDRV_API char
brli_drvwrite(brli_term *term)
{
  int i;

  if(brli_swrite(term, (unsigned char *)"\x1b""B", 2) < 2) /* Initialization string */
    {
      brli_seterror("Error writing to port");
      return 0;
    }
  for(i = 0; i < term->width; i++)
    {
      if(term->display[i] != 0x1b)
	{
	  if(brli_swrite(term, &term->display[i], 1) < 1)
	    {
	      brli_seterror("Error writing to port");
	      return 0;
	    }
	}
      else
	{
	  if(brli_swrite(term, &term->display[i], 1) < 1)
	    {
	      brli_seterror("Error writing to port");
	      return 0;
	    }
	  if(brli_swrite(term, &term->display[i], 1) < 1)
	    {
	      brli_seterror("Error writing to port");
	      return 0;
	    }
	}
    }
  return 1;
}

BRLDRV_API signed char
brli_drvread(brli_term *term, brl_key* key)
{
  unsigned char data[2];

  if(brli_timeout(term, term->cc_min, term->cc_time) == -1)
    {
      brli_seterror("Changing port timeouts failed");
      return -1;
    }

  if(brli_sread(term, data, 1) < 1)
    {
      return 0;
    }

  if(data[0] == 0xff) /* terminal was in suspend mode */
    {
      brli_log(LOG_INFO, "BrailleNote in suspend mode, refreshing display");
      brli_delay(100);
      brli_drvwrite(term); /* refresh screen with last string displayed */
      return 0;
    }

  if(brli_timeout(term, 0, 0) == -1)
    {
      brli_seterror("Changing port timeouts failed");
      return -1;
    }
  if(brli_sread(term, data + 1, 1) < 1)
    {
      return 0;
    }

  switch(data[0])
    {
    case 0x80: /* combination of dots 1-6 */
      if(data[1] == 0)
	return 0;
      key->type = BRL_KEY;
      key->braille = data[1]; 
      key->code = BRLK_UNKNOWN;
      return 1;
      break;
    case 0x81: /* combination of the SPACEBAR and dots 1-6 */
      if(data[1] == 0)
	{
	  key->type = BRL_KEY;
	  key->code = BRLK_SPACE;
	  key->braille = 0;
	} else {
          key->type = BRL_ACC;
	  key->code = BRLK_UNKNOWN;
	  key->braille = data[1];
	}
      return 1;
      break;
    case 0x82: /* combination of dot 7 and dots 1-6 */
      /* TODO: Change that */
      /* Pour l'instant on ne traite que le dot 7 tout seul */
      if(data[1] == 64) 
	{
	  key->type = BRL_CMD;
	  key->code = BRLK_DOT7;
	  return 1;
	}
      break;
    case 0x83: /* combination of dot 8 and dots 1-6 */
      /* TODO: Change that */
      /* Pour l'instant on ne traite que le dot 8 tout seul */
      if(data[1] == 0) 
	{
	  key->type = BRL_CMD;
	  key->code = BRLK_DOT8;
	  return 1;
	}
      break;
    case 0x84: /* thumb-key combination */
      switch(data[1])
	{
	case 1:
	  key->type = BRL_CMD;
	  key->code = BRLK_BACKWARD;
	  return 1;
	case 2:
	  key->type = BRL_CMD;
	  key->code = BRLK_RETURN;
	  return 1;
	case 4:
	  key->type = BRL_CMD;
	  key->code = BRLK_ESCAPE;
	  return 1;
	case 8:
	  key->type = BRL_CMD;
	  key->code = BRLK_FORWARD;
	  return 1;
	default:
	  return 0;
	}
      break;
    case 0x85: /* touch cursor button combination */
      key->type = BRL_CURSOR;
      key->code = data[1];
      return 1;
      break;
    default:
      brli_log(LOG_INFO, "unknown command 0x%x\n", data[0]);
    }
  return 0;
}
