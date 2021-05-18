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

enum read_status {
  GOTERR,
  GOTNACK,
  TIMEOUT,
  GOTDATA,
  FRAMEREADY
};

enum model_num {
  NONE = 0,
  IRIS,
  SCRIBA,
  AZER,
  CLIO
};

static const char *model_names[] =
  {
    "None",
    "Iris",
    "Scriba",
    "CLIO azerBraille",
    "CLIO euroBraille"
  };

static int model = 0;

/* ASCII codes used by the terminal */
#define SOH  0x01
#define EOT  0x04
#define ACK  0x06
#define DLE  0x10
#define NACK 0x15

/* Error codes (defined in the documentation) */
#define PRT_E_PAR 0x01 /* parity error */
#define PRT_E_NUM 0x02 /* frame number error */
#define PRT_E_ING 0x03 /* length error */
#define PRT_E_COM 0x04 /* command error */
#define PRT_E_DON 0x05 /* data error */
#define PRT_E_SYN 0x06 /* syntax error */
#define PRT_E_VOC 0x80 /* vocal mode error */

#define FRAMESIZE 259 /* SOH + message <= 253 + [DLE] + framenum + [DLE] + parity + EOT */

static unsigned char frameinnum = 0;
static unsigned char frameoutnum = 1;
static unsigned char framein[FRAMESIZE];
static unsigned char frameout[FRAMESIZE];
static unsigned char msgout[FRAMESIZE];
static int frameinsize = 0;
static int frameoutsize = 0;
static char DLEflag = 0, NACKFlag = 0;

/* local functions prototypes */
static size_t send_byte(brli_term *term, unsigned char c);
static size_t send_frame(brli_term *term, const unsigned char *data, size_t len);
static signed char get_frame(brli_term *brl);

BRLDRV_API const char *
brli_drvinfo(brli_term *term, brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      return "eurobraille";
    case BRL_TERMINAL:
      return model_names[model];
    default:
      return NULL;
    }
}

BRLDRV_API char
brli_drvinit(brli_term *term, char type, const char *pathname)
{
  int i;
  struct termios tiodata;
  unsigned char initmsg[] = {2, 'S', 'I', /* Ask ID of terminal */
			     3, 'M', 'P', 0x3e}; /* Plays a melody */
  
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
  /* 8 data bits - parity - 1 stop bit at 9600 */
  tiodata.c_iflag = INPCK;
  tiodata.c_cflag = CLOCAL | CREAD | CS8 | PARENB;
  tiodata.c_oflag = 0;
  tiodata.c_lflag = 0;
  
  /* Initial timeouts */
  tiodata.c_cc[VMIN] = 0;
  tiodata.c_cc[VTIME] = 1; /* we wait 100ms before getting an answer */
  
  if(brli_cfsetispeed(&tiodata, B9600) ||
     brli_cfsetospeed(&tiodata, B9600) ||
     brli_tcsetattr(term, TCSANOW, &tiodata))
    {
      brli_seterror("Port init failed: %s: %s", pathname, brli_geterror());
      brli_close(term);
      return 0;
    }
  brli_log(LOG_NOTICE, "Port init success: %s", pathname);

  /* Send initialization message */
  send_frame(term, initmsg, sizeof(initmsg));

  for(i = 0; i < 100; i++)
    {
      signed char status;
      
      status = get_frame(term);
      if(status == TIMEOUT || status == GOTERR)
	{
	  break;
	}
      else if(status == GOTNACK || status == GOTDATA)
	{
	  continue;
	}
      else /* status == FRAMEREADY */
	{
	  int j;
	  
	  /* framein[j] represents the length of current sequence */
	  for(j = 0; j + framein[j] <= frameinsize; j += framein[j] + 1)
	    {
	      brli_log(LOG_NOTICE, "msg length = %d, frame length = %d", framein[j],
		       frameinsize);
	      if(framein[j] < 6 || framein[j + 1] != 'S' || framein[j + 2] != 'I')
		{
		  brli_log(LOG_NOTICE, "not an identification sequence");
		  continue;
		}
	      /* We had an identification sequence !*/
	      if(framein[j + 3] == 'I' && framein[j + 4] == 'R')
		{
		  model = IRIS;
		  break;
		}
	      else if(framein[j + 3] == 'N' && framein[j + 4] == 'B')
		{
		  // model = /* NoteBRAILLE */
		  break;
		}
	      else if(framein[j + 3] == 'C' && framein[j + 4] == 'N')
		{
		  // model = /* Clio-NoteBRAILLE */
		  break;
		}
	      else if(framein[j + 3] == 'S' && (framein[j + 4] == 'B'
			|| framein[j + 4] == 'C'))
		{
		  model = SCRIBA;
		  break;
		}
	      else if(framein[j + 3] == 'C' && framein[j + 4] == 'Z')
		{
		  model = AZER;
		  break;
		}
	      else if(framein[j + 3] == 'C' && framein[j + 4] == 'P')
		{
		  model = CLIO;
		  break;
		}
	      else
		{
		  model = 0;
		  brli_seterror("Unknown EuroBraille display detected '%c' '%c'",
				framein[j + 3], framein[j + 4]);
		  term->width = -1;
		  brli_drvclose(term);
		  return 0;
		}
	    }
	  if(model > 0)
	    {
	      term->width = (framein[j + 5] - '0') * 10;
	      break;
	    }
	}
    }
  if(term->width < 1)
    {
      brli_seterror("No EuroBraille display detected");
      brli_drvclose(term);
      return 0;
    }
  brli_log(LOG_INFO, "Detected EuroBraille display");

  /* Allocate space for buffers */
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
  if(term->display)
    free(term->display);
  if(term->display_ascii)
    free(term->display_ascii);
  brli_close(term);
  term->width = -1;
  return 1;
}

BRLDRV_API char
brli_drvwrite(brli_term *term)
{
  int i;
  unsigned char *p = msgout;

  /* display text on LCD */
  *p++ = term->width + 2; /* length of sequence */
  *p++ = 'D';
  *p++ = 'L';
  for(i = 0; i < term->width; i++)
    *p++ = term->display_ascii[i];

  /* display dots on display */
  *p++ = term->width + 2; /* length of sequence */
  *p++ = 'D';
  *p++ = 'P';
  for(i = 0; i < term->width; i++)
    *p++ = term->display[i];

  send_frame(term, msgout, p - msgout);

  /* TODO: add a read with a small timeout in order to avoid terminal going in
     'absent PC Braille' mode ????*/
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
  signed char status;

  key->type = BRL_NONE;

  if(brli_timeout(term, term->cc_min, term->cc_time) == -1)
    {
      brli_seterror("Changing port timeouts failed");
      return -1;
    }

  do
    {
      status = get_frame(term);
      if(status == GOTERR)
	{
	  return -1;
	}
      else if(status == TIMEOUT)
	{
	  return 0;
	}
      else if(status == GOTNACK)
	{
	  return 1; /* return BRL_NONE */
	}
      else if(status == GOTDATA)
	{
	  if(brli_timeout(term, 0, 0) == -1)
	    {
	      brli_seterror("Changing port timeouts failed");
	      return -1;
	    }
	  continue;
	}
    }
  while(status != FRAMEREADY);

  brli_log(LOG_NOTICE, "first sequence: %d, frame length: %d", framein[0], frameinsize);

  /* TODO: test for all sequence in a message if necessary */
  /* for(j = 0; j + frame[j] < frameinsize; j += frame[j] + 1) */

  if(framein[0] > frameinsize)
    {

      /* invalid packet */
      return 0;
    }

  switch(framein[1])
    {
    case 'K': /* a key from the braille terminal */
      switch(framein[2])
	{
	case 'B': /* Braille code */
	  if(!(framein[3] & 0xC0))
	    {
	      key->type = BRL_KEY;
	      key->braille = (framein[3] & 0x3F) | ((framein[4] & 0x03) << 6);
	    }
	  else /* accord with space */
	    {
	      key->type = BRL_ACC;
	      key->braille = (framein[3] & 0x3F) | ((framein[4] & 0x03) << 6);
	    }
	  brli_log(LOG_NOTICE, "braille code: %o or 0x%x", key->braille, key->braille);
	  break;
	case 'T': /* function keys or phone keys */
	  switch(framein[3])
	    {
	    case 'E':
	      key->type = BRL_CMD;
	      key->code = BRLK_BACKWARD;
	      break;
	    case 'F':
	      break;
	    case 'G':
	      break;
	    case 'H':
	      break;
	    case 'I':
	      break;
	    case 'J':
	      break;
	    case 'K':
	      break;
	    case 'L':
	      break;
	    case 'M':
	      key->type = BRL_CMD;
	      key->code = BRLK_FORWARD;
	      break;
//	    case 'A':
//	      break;
//	    case 'B':
//	      break;
//	    case 'C':
//	      break;
//	    case 'D':
//	      break;
//	    case 'Z': /* ABCD Keys */
//	      break;
//	    case '1':    /* arrow keys */
//	      /* key->type = BRL_CMD; */
//	      /* key->code = BRLK_???; */
//	      /* TOP_LEFT */
//	      break;
	    case '2': /* up */
	      key->type = BRL_CMD;
	      key->code = BRLK_UP;
	      break;
//	    case '3':
//	      break;
	    case '4': /* left */
	      key->type = BRL_CMD;
	      key->code = BRLK_LEFT;
	      break;
	    case '5':
	      key->type = BRL_CMD;
	      key->code = BRLK_HOME;
	      break;
	    case '6': /* right */
	      key->type = BRL_CMD;
	      key->code = BRLK_RIGHT;
	      break;
//	    case '7':
//	      /* BOT_LEFT; */
//	      break;
	    case '8': /* down */
	      key->type = BRL_CMD;
	      key->code = BRLK_DOWN;
	      break;
// 		    case '9':
// 		      /* res = CMD_NXDIFLN; */
// 		      break;
// 		    case '*':
// 		      /* res = CMD_DISPMD; */
// 		      break;
// 		    case '0':
// 		      /* res = CMD_CSRTRK; */
// 		      break;
// 		    case '#':
// 		      key->type = BRL_CMD;
// 		      key->code = BRLK_RETURN;
// 		      break;
	    }
	  break;
	case 'I': /* routing keys */
	  key->type = BRL_CURSOR;
	  key->code = framein[3] - 1;
	  break;
	default:
	  brli_log(LOG_NOTICE, "unknown key: '%c', '%c' or 0x%x, '%c' or 0x%x",
		   framein[2], framein[3], framein[3], framein[4], framein[4]);
	  return 0;
	}
      break;
    case 'R': /* mode change */
    case 'M': /* melodie */
    case 'S': /* system */
    case 'F': /* file */
    case 'A': /* alarm */
    case 'C': /* central */
    case 'T': /* line test */
    case 'U': /* uart */
    case 'N': /* numerotation */
      return 0;
    default:
      printf("unknown code: %c, %c, %c\n", framein[1], framein[2], framein[3]);
      return 0;
    }
  return 1;
}

static size_t
send_byte(brli_term *term, unsigned char c)
{
  return brli_swrite(term, &c, 1);
}

static size_t
send_frame(brli_term *term, const unsigned char *data, size_t len)
{
  unsigned char *p = frameout;
  unsigned char parity = 0;

  if(len < 1)
    return 0;

  *p++ = SOH;
  while(len)
    {
      switch(*data)
	{
	case SOH:
	case EOT:
	case ACK:
	case DLE:
	case NACK:
	  *p++ = DLE;
	default:
	  *p++ = *data;
	  parity ^= *data;
	  data++;
	}
      len--;
      if(p - frameout == FRAMESIZE - 5)
	{
	  brli_log(LOG_NOTICE, "Error too long message");
	  return 0;
	}
    }
  switch(frameoutnum)
    {
    case SOH:
    case EOT:
    case ACK:
    case DLE:
    case NACK:
      *p++ = DLE;
    default:
      *p++ = frameoutnum;
      parity ^= frameoutnum;
    }
  switch(parity)
    {
    case SOH:
    case EOT:
    case ACK:
    case DLE:
    case NACK:
      *p++ = DLE;
    default:
      *p++ = parity;
    }
  *p++ = EOT;
  if(frameoutnum == 0xFF)
    frameoutnum = 0x80;
  else
    frameoutnum++;
  frameoutsize = p - frameout;
  return brli_swrite(term, frameout, frameoutsize);
}

static signed char
get_frame(brli_term *term)
{
  unsigned char c;

  if(brli_sread(term, &c, 1) < 1)
    return TIMEOUT; /* timeout */

  if(DLEflag)
    {
      DLEflag = 0;
      if(frameinsize < FRAMESIZE)
	{
	  brli_log(LOG_NOTICE, "got char: 0x%x or '%c'", c, c);
	  framein[frameinsize] = c;
	  frameinsize++;
	}
      else
	{
	  brli_log(LOG_NOTICE, "invalid packet, length error");
	  send_byte(term, NACK);
	  send_byte(term, PRT_E_ING);
	  frameinsize = 0;
	  return GOTERR;
	}
    }
  else if(NACKFlag)
    {
      brli_log(LOG_NOTICE, "got err flag");
      NACKFlag = 0;
      frameinsize = 0;
      if(c == PRT_E_PAR)
	{
	  brli_log(LOG_NOTICE, "Parity error from eurobraille terminal");
	  if(frameoutsize > 0)
	    {
	      /* TODO: only resend 3 times */
	      brli_log(LOG_NOTICE, "Resend last frame");
	      send_frame(term, frameout, frameoutsize);
	    }
	}
      else
	{
	  brli_log(LOG_NOTICE, "Error %d from eurobraille terminal", c);
	  return GOTERR;
	}
    }
  else
    switch(c)
      {
      case NACK:
	brli_log(LOG_NOTICE, "got nack");
	NACKFlag = 1;
	return GOTNACK;
	break;
      case ACK:
      case SOH:
	brli_log(LOG_NOTICE, "got ack");
	frameinsize = 0;
	break;
      case DLE:
	brli_log(LOG_NOTICE, "got dle");
	DLEflag = 1;
	break;
      case EOT:
	{
	  /* frame completely received */
	  int i;
	  unsigned char parity = 0;

	  brli_log(LOG_NOTICE, "got eot\n");

	  for(i = 0; i < frameinsize - 1; i++)
	    parity ^= framein[i];
	  if(parity != framein[frameinsize - 1])
	    {
	      brli_log(LOG_NOTICE, "invalid packet, parity error");
	      send_byte(term, NACK);
	      send_byte(term, PRT_E_PAR);
	      frameinsize = 0;
	      return GOTERR;
	    }
	  else if(frameinsize < 4) /* a frame cannot be shorter than 4 */
	    {
	      brli_log(LOG_NOTICE, "invalid packet, length error");
	      send_byte(term, NACK);
	      send_byte(term, PRT_E_ING);
	      break;
	    }
	  else if(frameinnum != 0 && framein[frameinsize - 2] <= frameinnum)
	    {
	      brli_log(LOG_NOTICE, "invalid packet, frame number error");
	      send_byte(term, NACK);
	      send_byte(term, PRT_E_NUM);
	      frameinsize = 0;
	      return GOTERR;
	    }
	  else
	    {
	      /* packet is OK */
	      brli_log(LOG_NOTICE, "packet is ok");
	      send_byte(term, ACK);
	      frameinnum = framein[frameinsize - 2];
	      if(frameinnum == 255)
		frameinnum = 0x7F;
	      frameinsize -= 2;	/* we remove parity and frame number bytes */
	      return FRAMEREADY;
	    }
	  break;
	}
      default:
	if(frameinsize < FRAMESIZE)
	  {
	    brli_log(LOG_NOTICE, "got char: 0x%x or '%c'", c, c);
	    framein[frameinsize] = c;
	    frameinsize++;
	  }
	else
	  {
	    brli_log(LOG_NOTICE, "invalid packet, length error");
	    send_byte(term, NACK);
	    send_byte(term, PRT_E_ING);
	    frameinsize = 0;
	    return GOTERR;
	  }
	break;
      }

  return GOTDATA;
}
