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

BRLDRV_API const char *
brli_drvinfo(brli_term *term, brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      return "technibraille";
    case BRL_TERMINAL:
      return "Manager";
    default:
      return NULL;
    }
}

BRLDRV_API char
brli_drvinit(brli_term *term, char type, const char *pathname)
{
  struct termios tiodata;
  int autodetect;

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
   tiodata.c_iflag = IGNBRK | IGNPAR;
   tiodata.c_cflag = CLOCAL | CREAD | CS8 | PARENB;
   tiodata.c_oflag = 0;
   tiodata.c_lflag = 0;
  
  /* Initial timeouts */
  tiodata.c_cc[VMIN] = 0;
  tiodata.c_cc[VTIME] = 1; /* we wait 100ms before getting an answer */

  if(brli_cfsetispeed(&tiodata, B19200) ||
     brli_cfsetospeed(&tiodata, B19200) ||
     brli_tcsetattr(term, TCSANOW, &tiodata))
    {
      brli_seterror("Port init failed: %s: %s", pathname, brli_geterror());
      brli_close(term);
      return 0;
    }
  brli_log(LOG_NOTICE, "Port init success: %s", pathname);

  /* Autodetect TechniBraille */
  for(autodetect = 0; autodetect < 6; autodetect++)
    {
      unsigned char query[] = "\x00\x04\x00\x04";
      unsigned char answer[3];
      int read;
      
      if(brli_swrite(term, query, sizeof(query)) < sizeof(query))
 	{
 	  brli_seterror("Error sending identification query");
 	  brli_drvclose(term);
 	  return 0;
 	}
      
      read = brli_sread(term, answer, sizeof(answer));
      if(read == 3 && memcmp(answer, "\x00\x04", 2) == 0)
 	{
 	  term->width = answer[2];
 	  break;
 	}
      else
 	{
	  brli_log(LOG_DEBUG, "Invalid answer from display");
 	}
    }
  if(term->width <= 0)
    {
      brli_seterror("No TechniBraille display detected");
      brli_drvclose(term);
      return 0;
    }
  brli_log(LOG_INFO, "Detected TechniBraille display");
  
  // Clean braille line (does nothing on first try)
  {
    unsigned char query[] = "\x00\x01\x28\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x29";
    brli_swrite(term, query, 44);
  }

  // Clean LCD
  {
    unsigned char query[] = "\x00\x02\x28\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x2a";
    brli_swrite(term, query, 44);
  }

  // Clean braille line
  {
    unsigned char query[] = "\x00\x01\x28\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x29";
    brli_swrite(term, query, 44);
  }

  term->display = (unsigned char *)malloc(term->width);
  term->display_ascii = (unsigned char *)malloc(term->width);
  term->status = (unsigned char *)malloc(term->status_width);
  term->status_ascii = (unsigned char *)malloc(term->status_width);
  term->temp = (unsigned char *)malloc(term->width + 4);
  if(!term->display || !term->display_ascii || !term->status || !term->status_ascii
     || ! term->temp)
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
  if(term->temp)
    {
      free(term->temp);
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
  unsigned char *ptr;
  unsigned char checksum;
  int i;

  ptr = term->temp;
  *ptr = 0;
  ptr++;
  *ptr = 1; // update braille line
  ptr++;
  *ptr = (unsigned char)term->width;
  ptr++;
  memcpy(ptr, term->display, term->width);
  ptr += term->width;
  // calculate checksum
  checksum = 0;
  for(i = 0; i < term->width + 3; i++)
    {
      checksum ^= term->temp[i];
    }
  *ptr = checksum;
  brli_swrite(term, term->temp, term->width + 4);

  ptr = term->temp;
  *ptr = 0;
  ptr++;
  *ptr = 2; // update LCD
  ptr++;
  *ptr = (unsigned char)term->width;
  ptr++;
  memcpy(ptr, term->display_ascii, term->width);
  ptr += term->width;
  // calculate checksum
  checksum = 0;
  for(i = 0; i < term->width + 3; i++)
    {
      checksum ^= term->temp[i];
    }
  *ptr = checksum;
  brli_swrite(term, term->temp, term->width + 4);
  
  return 1;
}

BRLDRV_API signed char
brli_drvread(brli_term *term, brl_key *key)
{
  unsigned char buffer[3];
  
  key->type = BRL_NONE;
  
  if(brli_timeout(term, term->cc_min, term->cc_time) == -1)
    {
      brli_seterror("Changing port timeouts failed");
      return -1;
    }

  if(brli_sread(term, buffer, 3) < 3 || buffer[0] != 0)
    return 0;

  switch(buffer[1])
    {
    case 1: // braille code
      key->type = BRL_KEY;
      key->braille = buffer[2];
      return 1;
    case 2: // cursor routing
      key->type = BRL_CURSOR;
      key->code = buffer[2] - 1;
      return 1;
    case 3: // function key
      switch(buffer[2])
	{
	case 1: // M key
	  break;
	case 2: // ESC key
	  key->type = BRL_CMD;
	  key->code = BRLK_ESCAPE;
	  return 1;
	case 3:
	  key->type = BRL_CMD;
	  key->code = BRLK_BACKWARD;
	  return 1;
	case 4: // Down arrow
	  key->type = BRL_CMD;
	  key->code = BRLK_BELOW;
	  return 1;
	case 5:
	  key->type = BRL_CMD;
	  key->code = BRLK_FORWARD;
	  return 1;
	case 6: // Up arrow
	  key->type = BRL_CMD;
	  key->code = BRLK_ABOVE;
	  return 1;
	case 7: // Extra key - only 40s
	  break;
	case 8: // Extra key 2 - only 40s
	  break;
	case 9: // Inser key
	  key->type = BRL_CMD;
	  key->code = BRLK_HOME;
	  return 1;
	case 0xA: // E key
	  key->type = BRL_CMD;
	  key->code = BRLK_PAGEUP;
	  return 1;
	case 0xB: // Supp key
	  key->type = BRL_CMD;
	  key->code = BRLK_END;
	  return 1;
	case 0xC: // L key
	  key->type = BRL_CMD;
	  key->code = BRLK_PAGEDOWN;
	  return 1;
	case 0xE: // Left thumb key
	  key->type = BRL_CMD;
	  key->code = BRLK_BACKSPACE;
	  return 1;
	case 0xF: // Right thumb key
	  key->type = BRL_CMD;
	  key->code = BRLK_SPACE;
	  return 1;
	case 0x19: // Extra key 3
	  break;
	case 0x1A: // Extra key 4
	  break;
	case 0x1B: // Extra key 5
	  break;
	case 0x28: // Verr Num - only 40s
	  key->type = BRL_CMD;
	  key->code = BRLK_NUMLOCK;
	  return 1;
	case 0x29: // Key under dot 4
	  break;
	case 0x2A: // Key under dot 8
	  break;
	case 0x2B: // / key - only 40s
	  key->type = BRL_CMD;
	  key->code = BRLK_SLASH;
	  return 1;
	case 0x2C: // * key - only 40s
	  key->type = BRL_CMD;
	  key->code = BRLK_ASTERISK;
	  return 1;
	case 0x2D: // - key - only 40s
	  key->type = BRL_CMD;
	  key->code = BRLK_MINUS;
	  return 1;
	case 0x2E: // + key - only 40s
	  key->type = BRL_CMD;
	  key->code = BRLK_PLUS;
	  return 1;
	case 0x30: // 0 key - only 40s
	case 0x31: // 1 key - only 40s
	case 0x32: // 2 key - only 40s
	case 0x33: // 3 key - only 40s
	case 0x34: // 4 key - only 40s
	case 0x35: // 5 key - only 40s
	case 0x36: // 6 key - only 40s
	case 0x37: // 7 key - only 40s
	case 0x38: // 8 key - only 40s
	case 0x39: // 9 key - only 40s
	  key->type = BRL_CMD;
	  key->code = buffer[2] - 0x30 + BRLK_0;
	  return 1;
	case 0x3F: // 2 Thumb keys together
	  key->type = BRL_CMD;
	  key->code = BRLK_RETURN;
	  return 1;
	default:
	  printf("Unknown function key: 0x%X\n", buffer[2]);
	  break;
	}
      break;
    case 4:
      /* when some text is displayed, the terminal answers with bytes
	 \x0\x4\xX where X is the number of characters written */
      // We ignore it
      break;
    default:
      printf("Read 3 bytes Key: 0x%X 0x%X 0x%X\n", buffer[0], buffer[1], buffer[2]);
    }

  return 0;
}
