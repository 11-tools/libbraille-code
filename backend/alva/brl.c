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
#include <errno.h>
#include <string.h>

#include "driver.h"
#include "serial.h"

typedef struct
{
  char *name;
  int devid;
  int width;
  int status_width;
} alva_model;

static alva_model models[] =
  {
    {"ABT320",                0, 20, 3},
    {"ABT340",                1, 40, 3},
    {"ABT340 Desktop",        2, 40, 5},
    {"ABT380",                3, 80, 5},
    {"ABT380 Twin Space",     4, 80, 5},
    /* 5 = reserved */
    /* 6 = reserved */
    /* 7 = Spacepad left */
    /* 8 = Keypad */
    /* 9 = Spacepad right */
    {"Delphi 420",         0x0A, 20, 3},
    {"Delphi 440",         0x0B, 40, 3},
    {"Delphi 440 Desktop", 0x0C, 40, 5},
    {"Delphi 480",         0x0D, 80, 5},
    {"Satellite 544",      0x0E, 40, 3},
    {"Satellite 570 Pro",  0x0F, 66, 3},
    {"Satellite 584 Pro",  0x10, 80, 3},
    {"Satellite 544 Traveller", 0x11, 40, 3 },
    { 0, }
  };

static alva_model *model = NULL; /* info about current model */

BRLDRV_API const char *
brli_drvinfo(brli_term *term, brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      return "alva";
    case BRL_TERMINAL:
	if(!model)
	    return "None";
	else
	    return model->name;
    default:
      return NULL;
    }
}

BRLDRV_API char
brli_drvinit (brli_term *term, char type, const char *pathname)
{
    struct termios tiodata;
    unsigned char buffer[13];
    const unsigned char init[6] = { 0x1B, 'F', 'U', 'N', 0x06, 0x0D};
    const unsigned char answer[4] = { 0x1B, 'I', 'D', '='}; 
    const unsigned char config[6] = { 0x1B, 'F', 'U', 'N', 0x07, 0x0D};
    unsigned char modelnum;
    char detected;
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
    tiodata.c_iflag = IGNPAR;
    tiodata.c_cflag = CLOCAL | CREAD | CS8;
    tiodata.c_oflag = 0;
    tiodata.c_lflag = 0;
    
    /* Initial timeouts */
    tiodata.c_cc[VMIN] = 0;
    tiodata.c_cc[VTIME] = 2; /* we wait 200ms before getting an answer */
    
    if(brli_cfsetispeed(&tiodata, B9600) ||
       brli_cfsetospeed(&tiodata, B9600) ||
       brli_tcsetattr(term, TCSANOW, &tiodata))
    {
	brli_seterror("Port init failed: %s: %s", pathname, brli_geterror());
	brli_close(term);
	return 0;
    }
    brli_log(LOG_NOTICE, "Port init success: %s", pathname);
    
    /* Detect device */
    detected = 0;
    for(i = 0; i < 3; i++)
    {
	if (brli_swrite(term, init, 6) < 6) /* init string */
	{
	    brli_seterror("Error writing to port");
	    brli_drvclose(term);
	    return 0;
	}
	
	if(brli_sread(term, (unsigned char *)&buffer, 6) == 6) /* Read ID answer from display */
	{
	    detected = 1;
	    break;
	}
    }
    if(!detected && strncmp(buffer, answer, 4))
    {
	brli_log(LOG_DEBUG, "Invalid answer from display");
	brli_seterror("No Alva display detected");
	brli_drvclose(term);
	return 0;
    }
    
    modelnum = buffer[4];
    for(i = 0; i < sizeof(models) / sizeof(alva_model); i++)
    {
	if(modelnum == models[i].devid)
	{
	    model = &models[i];
	    break;
	}
    }
    if(!model)
    {
	brli_log(LOG_NOTICE, "Unknow Alva Braille display - please contact libbraille team");
	brli_seterror("No Alva display detected");
	brli_drvclose(term);
	return 0;
    }
    brli_log(LOG_INFO, "Detected Alva display");

    /* The Satellites have 2 configurations: with or without status cells */
    /* Check configuration */
    if(model->devid >= 0x0E)
    {
	const unsigned char cfganswer[4] = { 0x7F, 0x07, 0x7E, 0x04};

	/* Send config request */
	if (brli_swrite(term, config, 6) < 6) /* config string */
	{
	    brli_seterror("Error writing to port");
	    brli_drvclose(term);
	    return 0;
	}

	/* Read config answer from display */
	if(brli_sread(term, (unsigned char *)&buffer, 12) < 12 || strncmp(buffer, cfganswer, 4))
	{
	    brli_log(LOG_WARNING, "Unable to get status cells configuration");
	    term->width = model->width;
	    term->status_width = model->status_width;
    	}
	else
	{
	    term->width = buffer[11];
	    term->status_width = buffer[9];
	}
    }
    else
    {
	term->width = model->width;
	term->status_width = model->status_width;
    }

    /* Allocate space for buffers */
    term->display = (unsigned char *)malloc(term->width);
    term->display_ascii = (unsigned char *)malloc(term->width);
    term->status = (unsigned char *)malloc(term->status_width);
    term->status_ascii = (unsigned char *)malloc(term->status_width);
    term->temp = (unsigned char *)malloc(5 + term->width);
    if(!term->display || !term->display_ascii || !term->status
       || !term->status_ascii || !term->temp)
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
  if(term->status)
    free(term->status);
  if(term->status_ascii)
    free(term->status_ascii);
  if(term->temp)
    free(term->temp);
  term->width = -1;
  return 1;
}

BRLDRV_API char
brli_drvwrite(brli_term *term)
{
    unsigned char *ptr = term->temp;

    /* Build request */
    *ptr = '\x1b';
    ptr++;
    *ptr = 'B';
    ptr++;
    *ptr = term->status_width;
    ptr++;
    *ptr = term->width;
    ptr++;
    memcpy(ptr, term->display, term->width);
    ptr += term->width;
    *ptr = '\x0D';

    /* Send request to display */
    if(brli_swrite(term, term->temp, 5 + term->width) < 5 + term->width)
    {
	brli_seterror("Error writing to port");
	brli_drvclose(term);
	return -1;
    }

    return term->width;
}

BRLDRV_API char
brli_drvstatus(brli_term *term)
{
    unsigned char *ptr = term->temp;

    /* Build request */
    *ptr = '\x1b';
    ptr++;
    *ptr = 'B';
    ptr++;
    *ptr = 0;
    ptr++;
    *ptr = term->status_width;
    ptr++;
    memcpy(ptr, term->status, term->status_width);
    ptr += term->status_width;
    *ptr = '\x0D';

    /* Send request to display */
    if(brli_swrite(term, term->temp, 5 + term->status_width)
       < 5 + term->status_width)
    {
	brli_seterror("Error writing to port");
	brli_drvclose(term);
	return -1;
    }

    return term->status_width;
}

BRLDRV_API signed char
brli_drvread(brli_term *term, brl_key *key)
{
  unsigned char c;
  
  key->type = BRL_NONE;

  if(brli_timeout(term, term->cc_min, term->cc_time) == -1)
  {
      brli_seterror("Changing port timeouts failed");
      return -1;
  }
  if(brli_sread(term, &c, 1) < 1)
  {
      return 0;
  }

  /* Interpret packet received from the display */
  if(c == 0x71) /* Front keys and touch cursor status */
  {
      if(brli_timeout(term, 0, 0) == -1)
      {
	  brli_seterror("Changing port timeouts failed");
	  return -1;
      }
      if(brli_sread(term, &c, 1) < 1)
      {
	  return 0;
      }
      if(c & 0x80) /* Key released */
      {
	  return 0;
      }
      switch(c)
      {
//      case 0x00: /* PROG1 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
      case 0x01: /* HOME1 */
	  key->type = BRL_CMD;
	  key->code = BRLK_HOME;
	  return 1;
      case 0x02: /* CUR1 */
	  key->type = BRL_CMD;
	  key->code = BRLK_END;
	  return 1;
      case 0x03: /* UP */
	  key->type = BRL_CMD;
	  key->code = BRLK_UP;
	  return 1;
      case 0x04: /* LEFT */
	  key->type = BRL_CMD;
	  key->code = BRLK_BACKWARD;
	  return 1;
      case 0x05: /* RIGHT */
	  key->type = BRL_CMD;
	  key->code = BRLK_FORWARD;
	  return 1;
      case 0x06: /* DOWN */
	  key->type = BRL_CMD;
	  key->code = BRLK_DOWN;
	  return 1;
//      case 0x07: /* CUR2 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
      case 0x08: /* HOME2 */
	  key->type = BRL_CMD;
	  key->code = BRLK_HOME;
	  return 1;
//      case 0x09: /* PROG2 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x0A: /* TK1 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x0B: /* TK2 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x0C: /* TK3 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x0D: /* TK4 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x20: /* TCS1 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x21: /* TCS2 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x22: /* TCS3 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x23: /* TCS4 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x24: /* TCS5 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x25: /* TCS6 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x30: /* DTCS1 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x31: /* DTCS2 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x32: /* DTCS3 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x33: /* DTCS4 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x34: /* DTCS5 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
//      case 0x35: /* DTCS6 */
//	  key->type = BRL_CMD;
//	  key->code = BRLK_;
//	  return 1;
      default:
	  brli_log(LOG_DEBUG, "Unknown front key code 0x%X", c);
      }
  }
  else if(c == 0x72 || c == 0x75) /* Routing cursor */
  {
      if(brli_timeout(term, 0, 0) == -1)
      {
	  brli_seterror("Changing port timeouts failed");
	  return -1;
      }
      if(brli_sread(term, &c, 1) < 1)
      {
	  return 0;
      }
      if(c & 0x80) /* Key released */
      {
	  return 0;
      }
      key->type = BRL_CURSOR;
      key->code = c;
      return 1;
  }
  else if(c == 0x77) /* Navigation keys */
  {
      if(brli_timeout(term, 0, 0) == -1)
      {
	  brli_seterror("Changing port timeouts failed");
	  return -1;
      }
      if(brli_sread(term, &c, 1) < 1)
      {
	  return 0;
      }
      if(c & 0x80) /* Key released */
      {
	  return 0;
      }
      switch(c)
      {
      case 0x00: /* Left navigation */
	  key->type = BRL_CMD;
	  key->code = BRLK_BACKWARD;
	  return 1;
      case 0x01:
	  key->type = BRL_CMD;
	  key->code = BRLK_UP;
	  return 1;
      case 0x02:
	  key->type = BRL_CMD;
	  key->code = BRLK_LEFT;
	  return 1;
      case 0x03:
	  key->type = BRL_CMD;
	  key->code = BRLK_DOWN;
	  return 1;
      case 0x04:
	  key->type = BRL_CMD;
	  key->code = BRLK_RIGHT;
	  return 1;
      case 0x05:
	  key->type = BRL_CMD;
	  key->code = BRLK_FORWARD;
	  return 1;
      case 0x20: /* Right navigation */
	  key->type = BRL_CMD;
	  key->code = BRLK_BACKWARD;
	  return 1;
      case 0x21:
	  key->type = BRL_CMD;
	  key->code = BRLK_UP;
	  return 1;
      case 0x22:
	  key->type = BRL_CMD;
	  key->code = BRLK_LEFT;
	  return 1;
      case 0x23:
	  key->type = BRL_CMD;
	  key->code = BRLK_DOWN;
	  return 1;
      case 0x24:
	  key->type = BRL_CMD;
	  key->code = BRLK_RIGHT;
	  return 1;
      case 0x25:
	  key->type = BRL_CMD;
	  key->code = BRLK_FORWARD;
	  return 1;
      default:
	  brli_log(LOG_DEBUG, "Unknown navigation code 0x%X", c);
      }
  }
  else if(c == 0x7F) /* Information block - we disregard it */
  {
      int i, max;

      if(brli_timeout(term, 0, 0) == -1)
      {
	  brli_seterror("Changing port timeouts failed");
	  return -1;
      }
      if(brli_sread(term, &c, 1) < 1 || c != 0x7E || brli_sread(term, &c, 1) < 1)
      {
	  brli_log(LOG_WARNING, "Received invalid block");
	  return 0;
      }
      max = c;
      for(i = 0; i < max; i++)
      {
	  if(brli_sread(term, &c, 1) < 1 || c != 0x7E || brli_sread(term, &c, 1) < 1)
	  {
	      brli_log(LOG_WARNING, "Received invalid block");
	      return 0;
	  }
      }
      return 0;
  }
  else
  {
      brli_log(LOG_DEBUG, "Received unknown code 0x%X", c);
  }

  return 0;
}
