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

#ifdef HAVE_USB_H
#include <usb.h>
#endif
#include "driver.h"

typedef struct
{
  unsigned char id;
  const char *name;
  unsigned char width;
  unsigned char status_width;
} alva_model;

static alva_model models[] =
  {
    { 0x0E, "Alva Satellite 544", 40, 3},
    { 0x0F, "Alva Satellite 570 Pro", 66, 3},
    { 0x10, "Alva Satellite 584 Pro", 80, 3},
    { 0x11, "Alva Satellite 544 Traveller", 40, 3}
  };

static int model = -1;

BRLDRV_API const char *
brli_drvinfo(brli_term *term, brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      return "alvausb";
    case BRL_TERMINAL:
      if(model == -1)
	return "None";
      else
	return models[model].name;
    default:
      return NULL;
    }
}

BRLDRV_API char
brli_drvinit(brli_term *term, char type, const char *pathname)
{
  struct usb_bus *busses;
  struct usb_bus *bus;
  struct usb_device *dev = NULL;
  signed char id = -1;
  int i;

  /* Looking for an ALVA on the USB bus */
  busses = usb_get_busses();
  for(bus = busses; bus; bus = bus->next)
    {
      for(dev = bus->devices; dev; dev = dev->next)
	{
	  /* Check if this device is an Alva */
	  if(dev->descriptor.idVendor != 0x06b0)
	    {
	      continue;
	    }
	  else
	    {
	      switch(dev->descriptor.idProduct)
		{
		case 1:
		  break;
		default:
		  brli_log(LOG_NOTICE, "Unknown Alva USB display - testing anyway");
		}
	      break;
	    }
	}
      if(dev)
	break;
    }
  if(!dev)
    {
      brli_seterror("No Alva USB display detected");
      return 0;
    }

  /* Opening device */
  term->usbhandle = usb_open(dev);
  if(!term->usbhandle)
    {
      brli_seterror("Error opening device");
      return 0;
    }
  brli_log(LOG_NOTICE, "Device opened successfully");

  if(usb_reset(term->usbhandle) < 0)
    {
	printf("usb_reset error\n");
	return 0;
    }

  /* Set configuration */
  if(usb_set_configuration(term->usbhandle, 1) < 0)
    {
      brli_drvclose(term);
      brli_seterror("Error setting configuration: %s", strerror(errno));
      return 0;
    }

  /* TODO: is this really necessary ? */
  /* sleep(1); */

  /* Claiming the device */
  if(usb_claim_interface(term->usbhandle, 0) < 0)
    {
      brli_drvclose(term);
      brli_seterror("Error claiming interface: %s\nDo you have the correct rights on the usb device?", strerror(errno));
      return 0;
    }
  term->interface_claimed = 1;
  brli_log(LOG_NOTICE, "Interface claimed successfully");

  /* Invalid on Mac */
  /*
    if(usb_set_altinterface(term->usbhandle, 0) < 0)
    {
	brli_drvclose(term);
	brli_seterror("Error changing interface: %s", strerror(errno));
	return 0;
    }
  */

  {
    unsigned char initmsg[] = { 0x1B, 'F', 'U', 'N', 0x06, 0x0D, 0x00, 0x00 };

    if(usb_bulk_write(term->usbhandle, 0x2, initmsg, sizeof(initmsg), 255) < sizeof(initmsg))
      {
	printf("couldn't write on usb: %s\n", strerror(errno));
	//	  brli_drvclose(term);
	//	  brli_seterror("%s", strerror(errno));
	//	  return 0;
      }
  }

  /* Detect model of device */
  for(i = 0; i < 10; i++)
    {
      unsigned char initmsg[] = { 0x1B, 'F', 'U', 'N', 0x06, 0x0D, 0x00, 0x00 };
      unsigned char answer[] = { 0x1B, 'I', 'D', '='};
      unsigned char data[8];
      int size;
      
      if(usb_bulk_write(term->usbhandle, 0x2, initmsg, sizeof(initmsg), 255) < sizeof(initmsg))
	{
	  printf("couldn't write on usb: %s\n", strerror(errno));
//	  brli_drvclose(term);
//	  brli_seterror("%s", strerror(errno));
//	  return 0;
	}
      size = usb_interrupt_read(term->usbhandle, 0x81, data, 8, 0);
      if(size < 6 || memcmp(answer, data, sizeof(answer)) != 0)
	{
	  printf("read %d bytes - invalid ID packet\n", size);
	  continue;
	}
      else
	{
	  printf("read valid ID packet\n");
	  id = data[sizeof(answer)];
	  term->width = 0;
	  break;
	}
    }
  if(id == -1)
    {
      brli_drvclose(term);
      brli_seterror("No valid ID packet received");
      return 0;
    }
  for(i = 0; i < sizeof(models) / sizeof(alva_model); i++)
    {
      if(models[i].id == id)
	{
	  model = i;
	  break;
	}
      else
	{
	  continue;
	}
    }
  if(model == -1)
    {
      brli_drvclose(term);
      brli_seterror("Model with unknow ID please contact the libbraille team.");
      return 0;
    }

  /* Getting terminal configuration (size and status cells)*/
  for(i = 0; i < 20; i++)
    {
      unsigned char confmsg[] = { 0x1B, 'F', 'U', 'N', 0x07, 0x0D, 0x00, 0x00 };
      unsigned char answer[] = { 0x7F, 0x07, 0x7E };
      unsigned char data[16];
      int size;

      /* Send conf sequence */
      if(usb_bulk_write(term->usbhandle, 0x2, confmsg, sizeof(confmsg), 255) < sizeof(confmsg))
	{
	  brli_drvclose(term);
	  brli_seterror("%s", strerror(errno));
	  return 0;
	}
      //usleep(200000);

      /* Read answer */
      size = usb_interrupt_read(term->usbhandle, 0x81, data, 8, 0);
      /* Verify this is a valid answer */
      if(size < 8)
	{
	  printf("1\n");
	  continue;
	}
      else if(memcmp(answer, data, sizeof(answer)) != 0)
	{
	  printf("2\n");
	  continue;
	}
      else if(data[sizeof(answer)] < 4)
	{
	  printf("3\n");
	  continue;
	}
      size = usb_interrupt_read(term->usbhandle, 0x81, data + 8, 8, 0);
      if(size < 4 || data[4] != 0x7E || data[6] != 0x7E || data[8] != 0x7E || data[10] != 0x7E)
	{
	  printf("4\n");
	  continue;
	}
      term->width = data[11];
      term->status_width = data[9];
      break;
    }
  /* We couldn't retrieve the configuration */
  if(term->width == 0)
    {
      /* Use a default conf */
      term->width = models[i].width;
      term->status_width = models[i].status_width;
    }
  
  if(term->status_width > 0)
    {
      term->status = (unsigned char *)malloc(term->status_width);
      term->status_ascii = (unsigned char *)malloc(term->status_width);
      /* TODO: Remove later */
      memset(term->status, 0, term->status_width);
    }

  /* round size to a multiple of 8 */
  {
    int size;

    size = 5 + term->status_width + term->width;
    if (size % 8 > 0)
      {
	size = size / 8;
	size += 1;
	size = size * 8;
      }
    term->temp = (unsigned char *)malloc(size);
  }
  term->display = (unsigned char *)malloc(term->width);
  term->display_ascii = (unsigned char *)malloc(term->width);
  if(!term->display || !term->display_ascii || !term->temp  ||
     (term->status_width > 0 && (!term->status_ascii || !term->status)))
    {
      brli_drvclose(term);
      brli_seterror("%s", strerror(errno));
      return 0;
    }

  return 1;
}

BRLDRV_API char
brli_drvclose(brli_term *term)
{
  if(term->interface_claimed)
    {
      usb_release_interface(term->usbhandle, 0);
    }
  if(term->usbhandle)
    {
      usb_close(term->usbhandle);
    }
  if(term->temp)
    free(term->temp);
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
  unsigned char *pos;
  int size;

  pos = term->temp;
  *pos = 0x1B;
  pos++;
  *pos = 'B';
  pos++;
  /* first cell where to write */
  *pos = 0;
  pos++;
  /* number of cells to write */
  *pos = term->status_width;
  pos++;
  memcpy(pos, term->status, term->status_width);
  pos += term->status_width;
  *pos = 0x0D;
  pos++;

  // round size to a multiple of 8
  size = pos - term->temp;
  if (size % 8 > 0)
    {
      size = size / 8;
      size += 1;
      size = size * 8;
    }

  if(usb_bulk_write(term->usbhandle, 0x2, term->temp, size, 0) < pos - term->temp)
    {
	  brli_seterror("error writing data");
	  return 0;
    }

  return 1;
}

BRLDRV_API char
brli_drvwrite(brli_term *term)
{
  unsigned char *pos;
  int size;

  pos = term->temp;
  *pos = 0x1B;
  pos++;
  *pos = 'B';
  pos++;
  /* first cell where to write */
  *pos = term->status_width;
  pos++;
  /* number of cells to write */
  *pos = (unsigned char)term->width;
  pos++;
  memcpy(pos, term->display, term->width);
  pos += term->width;
  *pos = 0x0D;
  pos++;

  /* round size to a multiple of 8 */
  size = pos - term->temp;
  if (size % 8 > 0)
    {
      size = size / 8;
      size += 1;
      size = size * 8;
    }

  if(usb_bulk_write(term->usbhandle, 0x2, term->temp, size, 0) < pos - term->temp)
    {
	  brli_seterror("error writing data");
	  return 0;
    }

  return 1;
}

BRLDRV_API signed char
brli_drvread(brli_term *term, brl_key* key)
{
  int size;
  unsigned char data[255];

  size = usb_interrupt_read(term->usbhandle, 0x81, data, 8, term->timeout);
  if(size < 0)
    {
      return 0;
    }
  else if(size < 2)
    {
      return 0;
    }

  printf("brli_drvread: read %d bytes\n", size);

  if(data[1] & 0x80) /* a key is released */
    {
      return 0; /* ignore it */
    }

  switch(data[0])
    {
    case 0x71: /* front keys */
      switch(data[1])
	{
	case 0: /* PROG1 */
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 1: /* HOME1 */
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 2: /* CUR1 */
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 3: /* UP */
	  key->type = BRL_CMD;
	  key->code = BRLK_UP;
	  break;
	case 4: /* LEFT */
	  key->type = BRL_CMD;
	  key->code = BRLK_BACKWARD;
	  break;
	case 5: /* RIGHT */
	  key->type = BRL_CMD;
	  key->code = BRLK_FORWARD;
	  break;
	case 6: /* DOWN */
	  key->type = BRL_CMD;
	  key->code = BRLK_DOWN;
	  break;
	case 7: /* CUR2 */
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 8: /* HOME2 */
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 9: /* PROG2 */
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 0x0A: /* TK1 */
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 0x0B: /* TK2 */
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 0x0C: /* TK3 */
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 0x0D: /* TK4 */
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	default:
	  return 0;
	}
      return 1;
    case 0x72: /* Cursor - bottom line */
    case 0x75: /* Cursor - top line */
      key->type = BRL_CURSOR;
      key->code = data[1];
      return 1;
    case 0x77: /* Navigation keys */
      /* left navig :
	 1
      0 2 4 5    
	 3

	 rigth navig use the same codes + 0x20
      */
      switch(data[1])
	{
	case 0:
	case 0x20:
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 1:
	case 0x21:
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 2:
	case 0x22:
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 3:
	case 0x23:
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 4:
	case 0x24:
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	case 5:
	case 0x25:
	  key->type = BRL_CMD;
	  key->code = BRLK_UNKNOWN;
	  break;
	default:
	  return 0;
	}
      return 1;
    }

  return 0;
}
