/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "driver.h"
#include "serial.h"

static unsigned char *rawdata;

BRLDRV_API const char *
brli_drvinfo(brli_term *term, brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      return "once";
    case BRL_TERMINAL:
      return "Eco (ONCE)";
    default:
      return NULL;
    }
}

BRLDRV_API char
brli_drvinit(brli_term *term, char type, const char *pathname)
{
  struct termios tiodata;
  int autodetect;
  char detected = 0;
  char winmode = 0;

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
  tiodata.c_cflag = CLOCAL | CREAD | CS8;
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

  /* Autodetect ECO */
  for(autodetect = 0; autodetect < 10; autodetect++)
    {
      unsigned char query[] = "\x10\x02\xF1\x57\x57\x57\x10\x03";
      unsigned char answer[9];
      int read;
      
      read = brli_sread(term, answer, sizeof(answer));
      
      if(autodetect > 0 && read < sizeof(answer))
	{
	}
      else if(memcmp(answer, "\x10\x02\xF1", 3) == 0 && memcmp(answer + 7, "\x10\x03", 2) == 0)
	{
	  detected = 1;
	  switch(answer[3])
	    {
	    case 0x20:
	      term->width = 20;
	      term->status_width = 2;
	      break;
	    case 0x40:
	      term->width = 40;
	      term->status_width = 4;
	      break;
	    case 0x80:
	      term->width = 80;
	      term->status_width = 4;
	      break;
	    default:
	      brli_seterror("Unknown Eco model");
	      brli_drvclose(term);
	      return 0;
	    }
	}
      else if(detected &&
	      memcmp(answer, "\x10\x02\x2e\x2e\x2e\x2e\x2e\x10\x03", sizeof(answer)) == 0)
	{
	  winmode = 1;
	}
      
      if(detected && winmode)
	{
	  break;
	}
      
      if(brli_swrite(term, query, sizeof(query)) < sizeof(query))
	{
	  brli_seterror("Error sending identification query");
	  brli_drvclose(term);
	  return 0;
	}
    }
  if(!detected || !winmode)
    {
      brli_seterror("No ONCE display detected");
      brli_drvclose(term);
      return 0;
    }
  brli_log(LOG_INFO, "Detected ONCE display");

  term->display = (unsigned char *)malloc(term->width);
  term->display_ascii = (unsigned char *)malloc(term->width);
  term->status = (unsigned char *)malloc(term->status_width);
  term->status_ascii = (unsigned char *)malloc(term->status_width);
  rawdata = (unsigned char *)malloc(term->width + term->status_width + 7);
  if(!term->display || !term->display_ascii || !term->status || !term->status_ascii
     || !rawdata)
    {
      brli_seterror("%s", strerror(errno));
      brli_drvclose(term);
      return 0;
    } 
  
  memset(rawdata, 0, term->width + term->status_width + 7);
  
  return 1;
}

BRLDRV_API char
brli_drvclose(brli_term *term)
{
  brli_close(term);
  if(term->display)
    {
      free(term->display);
    }
  if(term->display_ascii)
    {
      free(term->display_ascii);
    }
  if(term->status)
    {
      free(term->status);
    }
  if(term->status_ascii)
    {
      free(term->status_ascii);
    }
  if(rawdata)
    {
      free(rawdata);
    }
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
  unsigned char *ptr;

#define ECODOTS(a) (((a) & 1) << 4) | (((a) & 2) << 4) | (((a) & 4) << 4) | \
   (((a) & 8) >> 3) | (((a) & 16) >> 3) | (((a) & 32) >> 3) | (((a) & 64) << 1) | (((a) & 128) >> 4);

  /* Write prefix */
  memcpy(rawdata, "\x61\x10\x02\xBC", 4);
  ptr = rawdata + 4;
  /* Write status cells */
  for(i = 0; i < term->status_width; i++)
    {
      *ptr = ECODOTS(term->status[i]);
      ptr++;
    }
  /* Skip empty cell */
  *ptr = 0;
  ptr++;
  /* Write display cells */
  for(i = 0; i < term->width; i++)
    {
      *ptr = ECODOTS(term->display[i]);
      ptr++;
    }
  /* Write end */
  memcpy(ptr, "\x10\x03", 2);
 
  if(brli_swrite(term, rawdata, term->width + term->status_width + 7) <
     (unsigned char)(term->width + term->status_width + 7))
    {
      brli_seterror("Error writing to port");
      return 0;
    }
  
  return 1;
}

BRLDRV_API signed char
brli_drvread(brli_term *term, brl_key *key)
{
  unsigned char buffer[9];
  
  key->type = BRL_NONE;
  
  if(brli_timeout(term, term->cc_min, term->cc_time) == -1)
    {
      brli_seterror("Changing port timeouts failed");
      return -1;
    }

  if(brli_sread(term, buffer, 9) < 9)
    return 0;

  if(memcmp(buffer, "\x10\x02\x88", 3) || memcmp(buffer + 7, "\x10\x03", 2))
    return 0;

  /* we have a key */
  if(buffer[3]) /* Cursors */
    {
      switch(buffer[3])
	{
	case 0xD5: /* status cursor 0 */
	case 0xD6: /* status cursor 1 */
	case 0xD0: /* status cursor 2 */
	case 0xD1: /* status cursor 3 */
	  return 0;
	default:
	  if(buffer[3] < 0x80 || buffer[3] >= 0x80 + term->width)
	    {
	      return 0;
	    }
	  key->type = BRL_CURSOR;
	  key->code = buffer[3] - 0x80;
	}
      return 1;
    }
  else if(buffer[4]) /* Frontal keyboard */
    {
      switch(buffer[4])
	{
	case 0x01:  /* down */
	  key->type = BRL_CMD;
	  key->code = BRLK_DOWN;
	  break;
	case 0x02:  /* right */
	  key->type = BRL_CMD;
	  key->code = BRLK_FORWARD;
	  break;
	case 0x04:  /* middle */
	  key->type = BRL_CMD;
	  key->code = BRLK_RETURN;
	  break;
	case 0x08:  /* left */
	  key->type = BRL_CMD;
	  key->code = BRLK_BACKWARD;
	  break;
	case 0x10:  /* up */
	  key->type = BRL_CMD;
	  key->code = BRLK_UP;
	  break;
	  /* it is possible to make combinations of keys, in which
	     case the return code is the sum of the code of each
	     key */
	default:
	  return 0;
	}
      return 1;
    }
  else if(buffer[5])
    {
      switch(buffer[5])
	{
	case 0x01: /* F9 */
	case 0x02: /* ALT */
	case 0x04: /* F0 */
	case 0x40: /* shift */
	default:
	  return 0;
	}
      /* it is possible to make combinations of keys, in which
	 case the return code is the sum of the code of each
	 key */
    }
  else if(buffer[6])
    {
      switch(buffer[6])
	{
	case 0x01: /* F1 */
	case 0x02: /* F2 */
	case 0x04: /* F3 */
	case 0x08: /* F4 */
	case 0x10: /* F5 */
	case 0x20: /* F6 */
	case 0x40: /* F7 */
	case 0x80: /* F8 */
	default:
	  return 0;
	}
    }
  return 1;
}
