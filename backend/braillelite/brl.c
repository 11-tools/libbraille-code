/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#include "config.h"

#include <stdio.h>  /* temp for printf */
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <fcntl.h>
#include <errno.h>

#include "driver.h"
#include "serial.h"

/* A queue for key pressed */
#define KQSIZE 16
static unsigned char first_key;
static unsigned char key_queue_len;
static brl_key key_queue[KQSIZE];

static char wait_ok; /* indicates if we are waiting for an ok answer from the terminal */

/* Local prototypes */
static char fill_key_queue(brli_term *brl);


BRLDRV_API const char *
brli_drvinfo(brli_term *term, brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      return "braillelite";
    case BRL_TERMINAL:
      return "BrailleLite (Blazie Engineering)";
    default:
      return NULL;
    }
}

BRLDRV_API char
brli_drvinit(brli_term *term, char type, const char *pathname)
{
  struct termios tiodata;

  first_key = 0;
  key_queue_len = 0;

  if(brli_open(term, pathname) == -1)
    {
      brli_seterror("Port open failed: %s: %s", pathname, brli_geterror());
      return 0;
    }

  /* If we got it open, get the old attributes of the port */ 
  if(brli_tcgetattr(term, &tiodata))
    {
      brli_seterror("tcgetattr failed: %s: %s", pathname, brli_geterror());
      brli_close(term);
      return 0;
    }

  /* Serial port parameters: B9600, 8 data bits, no parity, 1 stop bit */
  tiodata.c_iflag = IGNPAR;
  tiodata.c_cflag = CLOCAL | CREAD | CS8; // | CRTSCTS;
  tiodata.c_oflag = 0;
  tiodata.c_lflag = 0;

  /* Initial timeouts */
  tiodata.c_cc[VMIN] = 0;
  tiodata.c_cc[VTIME] = 2; /* we wait 200ms before getting an answer */

  if(brli_cfsetispeed(&tiodata, B9600) ||
     brli_cfsetospeed(&tiodata, B9600) ||
     brli_tcsetattr(term, TCSANOW, &tiodata))
    {
      brli_seterror("Port init failed: %s: %s", pathname, strerror(errno));
      brli_drvclose(term);
      return 0;
    }
  brli_log(LOG_NOTICE, "Port init success: %s", pathname);

  /* Detecting Braille Lite */
  {
    unsigned char c;

    brli_swrite(term, (const unsigned char *)"\x05\x44", 2);
    if(brli_sread(term, &c, 1) < 1 || c != 0x5)
      {
	brli_seterror("No Braille Lite display detected");
	brli_drvclose(term);
	return 0;
      }
    brli_log(LOG_INFO, "Detected Braille Lite display");
  }

  /* Detecting size of terminal */
  {
    unsigned char detect[18];
    unsigned char c;

    memset(detect, 0, sizeof(detect));
    brli_swrite(term, detect, sizeof(detect));
    if(brli_sread(term, &c, 1) == 1 && c == 0x5)
      {
	term->width = 18;
      }
    else
      {
	term->width = 40;
      }
    brli_log(LOG_NOTICE, "Detected Braille Lite %d", term->width);
  }

  /* The terminal may return an identification string */
  /* does nothing on our Braille Lite 40 */
  // tiodata.c_cc[VTIME] = 2; /* we wait 200ms before getting an answer from the terminal */
  // tcsetattr(devfd, TCSANOW, &tiodata);
  // brli_swrite(devfd, (const unsigned char *)"\x05\x57", 2);
  // read(devfd, buffer, 1);

  /* Allocating buffers */
  term->display = (unsigned char *)malloc(term->width);
  term->display_ascii = (unsigned char *)malloc(term->width);
  if(!term->display || !term->display_ascii)
    {
      brli_seterror("%s", strerror(errno));
      brli_drvclose(term);
      return 0;
    }

  wait_ok = 0;

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
  struct termios tiodata;

  /* Get the attributes of the port */
  if(brli_tcgetattr(term, &tiodata))
    {
      brli_seterror("brli_tcgetattr failed: %s", brli_geterror());
      return 0;
    }

  tiodata.c_cc[VMIN] = 0; /* we wait 100ms before getting an answer from the terminal */
  tiodata.c_cc[VTIME] = 1;
  if(brli_tcsetattr(term, TCSANOW, &tiodata))
    {
      brli_seterror("Changing port parameters failed");
      return 0;
    }

  /* Waiting for a previous OK answer from the terminal */
  while(wait_ok && fill_key_queue(term));

  /* Sending write request */
  brli_swrite(term, (const unsigned char *)"\x05\x44", 2);
  wait_ok = 1;

  tiodata.c_cc[VMIN] = 0; /* we wait 100ms before getting an answer from the terminal */
  tiodata.c_cc[VTIME] = 1;
  if(brli_tcsetattr(term, TCSANOW, &tiodata))
    {
      brli_seterror("Changing port parameters failed");
      return 0;
    }

  /* Waiting for a OK answer from the terminal */
  while(wait_ok && fill_key_queue(term));

  /* Sending display data */
  brli_swrite(term, term->display, term->width);
  wait_ok = 1;
  return 1;
}

static char
fill_key_queue(brli_term *term)
{
  struct termios tiodata;
  unsigned char data[2];
  brl_key key;

  key.type = BRL_NONE;

  /* Get the attributes of the port */
  if(brli_tcgetattr(term, &tiodata))
    {
      brli_seterror("brli_tcgetattr failed: %s", brli_geterror());
      return 0;
    }

  if(brli_sread(term, data, 1) < 1)
    return 0;

  if(wait_ok && data[0] == 0x5) /* OK answer from terminal */
    {
      printf("OK answer\n");
      wait_ok = 0;
      return 1;
    }
  else if(data[0] == 0) /* extended keys of Braille Lite 40 */
    {
      tiodata.c_cc[VMIN] = 0;
      tiodata.c_cc[VTIME] = 1; /* we wait only 100ms, chars should already be there */
      brli_tcsetattr(term, TCSANOW, &tiodata);
      if(brli_sread(term, data, 2) < 2)
	{
#ifdef DEBUG
	  printf("Error: extended key data where not ready!\n");
#endif
	  return 0;
	}
      if(data[0] == 0) /* Braille Lite 40 extra keys */
	{
	  if(data[1] & 0x80) /* Navigation bar */
	    {
	      switch(data[1])
		{
		case 0x88: /* Navigation bar button 1 */
		  key.type = BRL_CMD;
		  key.code = BRLK_BACKWARD;
		  break;
		case 0x84: /* Navigation bar button 2 */
		  key.type = BRL_CMD;
		  key.code = BRLK_ESCAPE;
		  break;
		case 0x82: /* Navigation bar button 3 */
		  key.type = BRL_CMD;
		  key.code = BRLK_RETURN;
		  break;
		case 0x81: /* Navigation bar button 4 */
		  key.type = BRL_CMD;
		  key.code = BRLK_FORWARD;
		  break;
		}
	    }
	  else if(data[1] > 0 && data[1] <= term->width) /* Cursor routing */
	    {
	      key.type = BRL_CURSOR;
	      key.code = data[1] - 1;
	    }
	}
      else
	{
	  /* Using dot 7 or 8 */
	  if(data[1] & 0x40) /* Using space key */
	    {
	      key.type = BRL_ACC;
	    }
	  else
	    {
	      key.type = BRL_KEY;
	    }
	  key.braille = data[0];
	}
    }
  else if(data[0] == 0x80 && term->width != 18) /* Braille Millenium extra keys */
    {
      printf("Millenium keys unhandled yet\n");
    }
  else /* Basic keys (Braille Lite 18) */
    {
      if(data[0] >= 0x80)
	{
	  if(data[0] == 0x80) /* Advance bar */
	    {
	      key.type = BRL_CMD;
	      key.code = BRLK_FORWARD;
	    }
	  else if(data[0] == 0x83)
	    {
	      key.type = BRL_CMD;
	      key.code = BRLK_BACKWARD;
	    }
	  else
	    {
	      printf("Unknown key pressed\n");
	    }
	}
      else
 	{
	  if(data[0] & 0x40) /* Accord with Space */
	    {
	      key.type = BRL_ACC;
	    }
	  else
	    {
	      key.type = BRL_KEY;
	    }
	  key.braille = data[0] & 0x3f;
 	}
    }

  if(key_queue_len == KQSIZE)
    {
      printf("Error: Key queue overflow\n");
      return 0;
    }
  else
    {
      memcpy(&key_queue[(first_key + key_queue_len) % KQSIZE], &key, sizeof(brl_key));
      key_queue_len++;
    }
  return 1;
}

BRLDRV_API signed char
brli_drvread(brli_term *term, brl_key *key)
{
  struct termios tiodata;
  key->type = BRL_NONE;
  
  while(key_queue_len == 0)
    {
      /* Get the attributes of the port */
      if(brli_tcgetattr(term, &tiodata))
	{
	  brli_seterror("brli_tcgetattr failed: %s", brli_geterror());
	  return -1;
	}

      tiodata.c_cc[VMIN] = term->cc_min;
      tiodata.c_cc[VTIME] = term->cc_time;
      if(brli_tcsetattr(term, TCSANOW, &tiodata))
	{
	  brli_seterror("Changing port parameters failed");
	  return -1;
	}
      if(fill_key_queue(term) == 0)
	{
	  return 0;
	}
    }
  
  memcpy(key, &key_queue[first_key], sizeof(brl_key));
  first_key = (first_key + 1) % KQSIZE;
  key_queue_len -= 1;
  return 1;
}
