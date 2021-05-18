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

static unsigned int handytech_keys;

BRLDRV_API const char *
brli_drvinfo(brli_term *term, brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      return "handytech";
    case BRL_TERMINAL:
      return term->name;
    default:
      return NULL;
    }
}

BRLDRV_API char
brli_drvinit(brli_term *term, char type, const char *pathname)
{
  struct termios tiodata;
  int i;

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
  tiodata.c_iflag = INPCK;
  tiodata.c_cflag = CLOCAL | CREAD | CS8 | PARENB | PARODD;
  tiodata.c_oflag = 0;
  tiodata.c_lflag = 0;

  /* Initial timeouts */
  tiodata.c_cc[VMIN] = 0;
  tiodata.c_cc[VTIME] = 2; /* we wait 200ms before getting an answer */

  if(brli_cfsetispeed(&tiodata, B19200) ||
     brli_cfsetospeed(&tiodata, B19200) ||
     brli_tcsetattr(term, TCSANOW, &tiodata))
    {
      brli_seterror("Port init failed: %s: %s", pathname, brli_geterror());
      brli_close(term);
      return 0;
    }
  brli_log(LOG_NOTICE, "Port init success: %s", pathname);

  /* Autodetect terminal and get size */
  for(i = 0; i < 5; i++)
  {
    unsigned char data[3];
    int size;

    if(brli_swrite(term, (unsigned char *)"\xFF", 1) < 1) /* Initialization string */
      {
	brli_seterror("Error writing to port");
	brli_drvclose(term);
	return 0;
      }

    size = brli_sread(term, data, 2);
    brli_log(LOG_DEBUG, "read %d bytes: 0x%x, 0x%x\n", size, data[0], data[1]);
    if(size == 2 && data[0] == 0xfe)
      {
	switch(data[1])
	  {
	  case 5:
	    term->name = "BrailleWave";
	    term->width = 40;
	    break;
	  case 0x72:
	    term->name = "Braillino";
	    term->width = 20;
	    term->status_width = 2;
	    break;
	  case 0x74:
	    term->name = "Braille Star";
	    term->width = 40;
	    term->status_width = 2;
	    break;
	  case 0x78:
	    term->name = "Braille Star";
	    term->width = 80;
	    term->status_width = 3;
	    break;
	  case 0x80:
	    term->name = "Modular";
	    term->width = 20;
	    term->status_width = 4;
	    break;
	  case 0x89:
	    term->name = "Modular";
	    term->width = 40;
	    term->status_width = 4;
	    break;
	  case 0x88:
	    term->name = "Modular";
	    term->width = 80;
	    term->status_width = 4;
	    break;
	  case 0x90:
	    term->name = "BookWorm";
	    term->width = 8;
	    term->status_width = 1;
	    break;
	  default:  
	    brli_log(LOG_INFO, "Unknown Handytech display with ID 0x%X", data[1]);
	  }
	break;
      }
  }
  if(term->width <= 0)
    {
      brli_seterror("No Handytech display detected");
      brli_drvclose(term);
      return 0;
    }
  brli_log(LOG_INFO, "Detected Handytech display");

  term->display = (unsigned char *)malloc(term->width);
  term->display_ascii = (unsigned char *)malloc(term->width);
  if(!term->display || !term->display_ascii)
    {
      brli_seterror("%s", strerror(errno));
      brli_drvclose(term);
      return 0;
    }
  
  handytech_keys = 0;
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
brli_drvwrite(brli_term *term)
{
  brli_swrite(term, (unsigned char *)"\x01", 1);
  brli_swrite(term, term->display, term->width);
  return 1;
}

BRLDRV_API char
brli_drvstatus(brli_term *term)
{
  return 1;
}

BRLDRV_API signed char
brli_drvread(brli_term *term, brl_key* key)
{
  unsigned char c;

  key->type = BRL_NONE;

  if(brli_timeout(term, term->cc_min, term->cc_time) == -1)
    {
      brli_seterror("Changing port timeouts failed");
      return -1;
    }

  if(brli_sread(term, (void *) &c, sizeof(c)) < 1)
    {
      return 0;
    }
  printf("read code: 0x%x\n", c);


//  Braille Wave codes:
//  when pressed (add 80 when released):
//
//  04   20 21 ................ 6F   08
//
//       03 07 0B 0F   13 17 1B 1F
//
//          0C  ====10====  14

  /* Cursor movement keys have codes in the range of 0x20-0x6f (left to right)
   * for press and 0xa0-0xef for release */
  if(c == 0x7e) // string display acknowledgement
    {
      return 0;
    }
  else if(c >= 0x20 && c <= 0x6F) /* cursor key pressed */
    { 
      key->type = BRL_CURSOR;
      key->code = c - 0x20;
      handytech_keys = 0;
      return 1;
    }
  else if(c >= 0xA0 && c <= 0xEF) /* cursor key released */
    {
      handytech_keys = 0;
      return 0;
    }
  else if(c < 0x20) /* Key pressed */
    {
      switch(c)
	{
	case 0x03: // dot 7 pressed
	  handytech_keys |= 1 << 6;
	  break;
	case 0x04: // Left navigation key pressed
	  key->type = BRL_CMD;
	  key->code = BRLK_BACKWARD;
	  handytech_keys = 0;
	  return 1;
	case 0x07: // dot 3 pressed
	  handytech_keys |= 1 << 2;
	  break;
	case 0x08: // Right navigation key pressed
	  key->type = BRL_CMD;
	  key->code = BRLK_FORWARD;
	  handytech_keys = 0;
	  return 1;
	case 0x0B: // dot 2 pressed
	  handytech_keys |= 1 << 1;
	  break;
	case 0x0C: // ESC key pressed
	  key->type = BRL_CMD;
	  key->code = BRLK_ESCAPE;
	  handytech_keys = 0;
	  return 1;
	case 0x0F: // dot 1 pressed
	  handytech_keys |= 1;
	  break;
	case 0x10: // Space key pressed
	  key->type = BRL_KEY;
	  key->code = BRLK_SPACE;
	  key->braille = 0;
	  handytech_keys = 0;
	  return 1;
	case 0x13: // dot 4 pressed
	  handytech_keys |= 1 << 3;
	  break;
	case 0x14: // Return key pressed
	  key->type = BRL_CMD;
	  key->code = BRLK_RETURN;
	  handytech_keys = 0;
	  return 1;
	case 0x17: // dot 5 pressed
	  handytech_keys |= 1 << 4;
	  break;
	case 0x1B: // dot 6 pressed
	  handytech_keys |= 1 << 5;
	  break;
	case 0x1F: // dot 8 pressed
	  handytech_keys |= 1 << 7;
	  break;
	default:
	  return 0;
	}
      return 1;
    }
  else if(c >= 0x83) /* key released */
    {
      switch(c)
	{
	case 0x83:
	case 0x87:
	case 0x8B:
	case 0x8F:
	case 0x93:
	case 0x97:
	case 0x9B:
	case 0x9F:
	  if(handytech_keys)
	    {
	      key->type = BRL_KEY;
	      key->braille = handytech_keys;
	      handytech_keys = 0;
	      return 1;
	    }
	  break;
	  //TODO: Handle Chord with Space
	default:
	  handytech_keys = 0;
	}
    }
  return 0;
}
