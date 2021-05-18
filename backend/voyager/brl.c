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

/* A queue for key pressed */
#define KQSIZE 16
static unsigned char first_key;
static unsigned char key_queue_len;
static brl_key key_queue[KQSIZE];

BRLDRV_API const char *
brli_drvinfo(brli_term *term, brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      return "voyager";
    case BRL_TERMINAL:
      return "Tieman Voyager"; /* TODO: use string returned by display */
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

  /* Looking for a Voyager on the USB bus */
  busses = usb_get_busses();
  for(bus = busses; bus; bus = bus->next)
    {
      for(dev = bus->devices; dev; dev = dev->next)
	{
	  /* Check if this device is made by Tieman */
	  if(dev->descriptor.idVendor != 0x0798)
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
		  brli_log(LOG_NOTICE, "Unknown Tieman USB display - testing anyway");
		}
	      break;
	    }
	}
      if(dev)
	break;
    }
  if(!dev)
    {
      brli_seterror("No Tieman USB Voyager display detected");
      return 0;
    }
  brli_log(LOG_INFO, "Detected Tieman USB Voyager display");

  /* Opening device */
  term->usbhandle = usb_open(dev);
  if(!term->usbhandle)
    {
      brli_seterror("Error opening device");
      return 0;
    }
  brli_log(LOG_NOTICE, "Device opened successfully");


  /* Invalid on Mac OS X */
  /*
    if(usb_set_configuration(term->usbhandle, 1) < 0)
      {
        brli_drvclose(term);
        brli_seterror("Error setting configuration: %s", strerror(errno));
        return 0;
      }
  */
 
  /* Claiming the device */
  if(usb_claim_interface(term->usbhandle, 0) < 0)
    {
      brli_drvclose(term);
      brli_seterror("Error claiming interface: %s\nDo you have the correct rights on the usb device?", strerror(errno));
      return 0;
    }
  term->interface_claimed = 1;
  brli_log(LOG_NOTICE, "Interface claimed successfully");
 
  /* Invalid on Mac OS X */
  /*
    if(usb_set_altinterface(term->usbhandle, 0) < 0)
      {
        brli_drvclose(term);
        brli_seterror("Error changing interface: %s", strerror(errno));
        return 0;
      }
  */

  {
    int n, i;
    unsigned char buf[100];
    
    n = usb_control_msg(term->usbhandle, 0x80, 6, 0x300 | dev->descriptor.iManufacturer, 0,
			(char *)buf, sizeof(buf), 200);
      if(n > 0)
	{
	  printf("Manufacturer  : ");
	  for(i = 2; i < n; i += 2)
	    printf("%c", buf[i]);
	  printf("\n");
	}
      
      n = usb_control_msg(term->usbhandle, 0x80, 6, 0x300 | dev->descriptor.iProduct, 0,
			  (char *)buf, sizeof(buf), 200);
      if(n > 0)
	{
	  printf("Product       : ");
	  for(i = 2; i < n; i += 2)
	    printf("%c", buf[i]);
	  printf("\n");
	}

      n = usb_control_msg(term->usbhandle, 0x80, 6, 0x300 | dev->descriptor.iSerialNumber, 0,
			  (char *)buf, sizeof(buf), 200);
      if(n > 0)
	{
	  printf("Serial number : ");
	  for(i = 2; i < n; i += 2)
	    printf("%c", buf[i]);
	  printf("\n");
	}

      // buf[0] = 0; buf[1] = 0;

      /* Asking type and size of the display */
      n = usb_control_msg(term->usbhandle, 0xC2, 6, 0, 0, (char *)buf, 2, 200);
      if(n > 1)
	{
	  if(buf[1] == 48)
	    {
	      term->width = 44;
	    }
	  else if(buf[1] == 72)
	    {
	      term->width = 70;
	      }
	  else
	    {
	      brli_log(LOG_ERR, "unknown terminal size. Using 44");
	      term->width = 44;
	    }
	}
      else
	{
	  brli_drvclose(term);
	  brli_seterror("error reading data from USB");
	  return 0;
	}

      /* Changing voltage of the display */
      n = usb_control_msg(term->usbhandle, 0x42, 1, 0x60, 0, (char *)buf, 0, 200);
      if(n < 0)
	{
	  brli_drvclose(term);
	  brli_seterror("error setting display voltage");
	  return 0;
	}

      /* Display makes a beep */
      n = usb_control_msg(term->usbhandle, 0x42, 9, 0x64, 0, NULL, 0, 200);
      if(n < 0)
	{
	  brli_drvclose(term);
	  brli_seterror("error asking for a beep");
	  return 0;
	}

      /* Set display ON */
      n = usb_control_msg(term->usbhandle, 0x42, 0, 0x1, 0, NULL, 0, 200);
      if(n < 0)
	{
	  brli_drvclose(term);
	  brli_seterror("error setting display ON");
	  return 0;
	}
    }

  term->display = (unsigned char *)malloc(term->width);
  term->display_ascii = (unsigned char *)malloc(term->width);
  term->temp = (unsigned char *)malloc(term->width + 4); /* data to send to terminal */
  term->temp1 = (unsigned char *)malloc(8); /* state of keys on the terminal */
  term->temp2 = (unsigned char *)malloc(1); /* state of braille keys on the terminal */
  if(!term->display || !term->display_ascii || !term->temp || !term->temp1 || !term->temp2)
    {
      brli_drvclose(term);
      brli_seterror("%s", strerror(errno));
      return 0;
    }
  memset(term->temp1, 0, 8); /* clean storage for keys state */
  memset(term->temp2, 0, 1); /* clean storage for keys state */

  first_key = 0;
  key_queue_len = 0;
  
  return 1;
}

BRLDRV_API char
brli_drvclose(brli_term *term)
{
  if(term->interface_claimed)
    {
      int n;
      
      /* Set display OFF */
      n = usb_control_msg(term->usbhandle, 0x42, 0, 0, 0, NULL, 0, 200);
      if(n < 0)
 	{
	  brli_log(LOG_ERR, "error setting display OFF");
 	}
      
      usb_release_interface(term->usbhandle, 0);
    }
  if(term->usbhandle)
    {
      usb_close(term->usbhandle);
    }
  if(term->temp1)
    free(term->temp1);
  if(term->temp2)
    free(term->temp2);
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
  return 1;
}

BRLDRV_API char
brli_drvwrite(brli_term *term)
{
  int size;

  if(term->width == 44)
    {
      term->temp[0] = 0; // unused
      term->temp[1] = 0; // unused
      memcpy(term->temp + 2, term->display, 6);

      term->temp[8] = 0; // unused
      term->temp[9] = 0; // unused
      
      memcpy(term->temp + 10, term->display + 6, term->width - 6);
  
      /* Displaying some Braille */
      size = usb_control_msg(term->usbhandle, 0x42, 7, 0, 0, (char *)term->temp, term->width + 4, 100);
      if(size < term->width + 4)
	{
	  brli_seterror("error displaying Braille");
	  return 0;
	}
    }
  else /* size == 70 */
    {
      term->temp[0] = 0; // unused
      term->temp[1] = 0; // unused
      memcpy(term->temp + 2, term->display, term->width);
  
      /* Displaying some Braille */
      size = usb_control_msg(term->usbhandle, 0x42, 7, 0, 0, (char *)term->temp, term->width + 2, 100);
      if(size < term->width + 2)
	{
	  brli_seterror("error displaying Braille");
	  return 0;
	}
    }

  return 1;
}

BRLDRV_API signed char
brli_drvread(brli_term *term, brl_key* key)
{
  int size;
  unsigned char data[8];

  key->type = BRL_NONE;

  if(key_queue_len == 0) /* No more keys - read some more */
    {
      /* Reading keys state from the display */
      size = usb_interrupt_read(term->usbhandle, 0x81, data, 8, term->timeout);
      if(size == -1)
	{
	  brli_log(LOG_ERR, "Error in usb subsystem while reading");
	  brli_seterror("Error in usb subsystem while reading");
	  return -1;
	}
      else if(size < 8)
	{
	  return 0;
	}

      /* braille key pressed */
      if(data[0] != term->temp2[0])
	{
	  if(term->temp2[0] == 0) /* no old key pressed */
	    {
	      term->temp2[0] = data[0]; /* store key combinaison */
	    }
	  else if(data[0] == 0 && term->temp2[0]) /* all keys are released */
	    {
	      unsigned char offset = (first_key + key_queue_len) % KQSIZE;
	      key_queue[offset].type = BRL_KEY;
	      key_queue[offset].braille = term->temp2[0];
	      key_queue_len++;
	      if(key_queue_len == KQSIZE)
		{
		  brli_log(LOG_ERR, "Key queue overflow");
		  first_key = (first_key + 1) % KQSIZE;
		  key_queue_len--;
		}
	      term->temp2[0] = 0;
	    }
	  else if((term->temp2[0] ^ data[0]) & data[0]) /* new key pressed */
	    {
	      term->temp2[0] = data[0]; /* store new key combinaison */
	    }
	  /* else some key were released. we do nothing to wait for
	     all key released */
	}

      /* function key pressed */
      if(term->temp1[1] != data[1] && data[1])
	{
	  char i;
	  
	  for(i = 0; i < 8; i++)
	    {
	      if((data[1] & (1 << i)) > (term->temp1[1] & (1 << i)))
		{
		  unsigned char offset = (first_key + key_queue_len) % KQSIZE;

		  key_queue[offset].type = BRL_CMD;
		  switch(i)
		    {
		    case 0:
		      key_queue[offset].code = BRLK_BACKWARD;
		    break;
		    case 1:
		      key_queue[offset].code = BRLK_ESCAPE;
		    break;
		    case 2:
		      key_queue[offset].code = BRLK_LEFT;
		    break;
		    case 3:
		      key_queue[offset].code = BRLK_UP;
		    break;
		    case 4:
		      key_queue[offset].code = BRLK_DOWN;
		    break;
		    case 5:
		      key_queue[offset].code = BRLK_RIGHT;
		    break;
		    case 6:
		      key_queue[offset].code = BRLK_RETURN;
		    break;
		    case 7:
		      key_queue[offset].code = BRLK_FORWARD;
		    break;
		    }

		  key_queue_len++;
		  if(key_queue_len == KQSIZE)
		    {
		      brli_log(LOG_ERR, "Key queue overflow");
		      first_key = (first_key + 1) % KQSIZE;
		      key_queue_len--;
		    }
		}
	    }
	}

      /* cursor key pressed */
      if(data[2] != term->temp1[2] && data[2] > 0 && data[2] <= term->width)
	{
	  key_queue[(first_key + key_queue_len) % KQSIZE].type = BRL_CURSOR;
	  key_queue[(first_key + key_queue_len) % KQSIZE].code = data[2] - 1;

	  key_queue_len++;
	  if(key_queue_len == KQSIZE)
	    {
	      brli_log(LOG_ERR, "Key queue overflow");
	      first_key = (first_key + 1) % KQSIZE;
	      key_queue_len--;
	    }
	}

      memcpy(term->temp1, data, 8); /* save old state */
    }

  /* There are unread keys in the queue */
  if(key_queue_len > 0)
    {
      memcpy(key, &key_queue[first_key], sizeof(brl_key));
      first_key = (first_key + 1) % KQSIZE;
      key_queue_len--;
      return 1;
    }

  return 0;
}
