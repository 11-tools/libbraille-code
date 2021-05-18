/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#include "serial.h"

static char (*drvinit_func) (brli_term *term, const char *pathname);
static char (*drvclose_func) (brli_term *term);
static int (*write_func) (brli_term *term, char *bytes, int size);
static int (*read_byte_func) (brli_term *term, char *byte, int timeout);
static int (*read_bytes_func) (brli_term *term, char *bytes, int size, int timeout);

#define ESCAPE 0x1B
#define TSP_DATA_SIZE 5

enum
{
	RQT_DISPLAY_DATA   = 0x01,
	RQT_VERSION_NUMBER = 0x05,
	RQT_REPEAT_ALL     = 0x08,
	RQT_MODE_FLAGS     = 0X12,
	RQT_PROTOCOL       = 0x15,
	RQT_COMM_CHANNEL   = 0X16,
	RQT_INFO_BLOCK     = 0X42, /* for test only */
	RQT_READ_EEPROM    = 0x70,
	RQT_WRITE_EEPROM   = 0x71,
	RQT_TEST_HARDWARE  = 0x80, /* for test only */
	RQT_DEVICE_ID      = 0x84,
	RQT_SERIAL_NUMBER  = 0x8A,
	RQT_PROG_SERIALNUM = 0x8B
} request_code;

enum
{
	ANS_CELLS_NUMBER   = 0x01,
	ANS_VERSION_NUMBER = 0x05,
	ANS_TSP_DATA       = 0x22,
	ANS_BUTTON_DATA    = 0x24,
	ANS_ERROR_CODE     = 0x40,
	ANS_INFO_BLOCK     = 0x42,
	ANS_DEVICE_ID      = 0x84,
	ANS_SERIAL_NUM     = 0x8A
} answer_code;

enum
{
	KEY_TL1 = 0x01,
	KEY_TL2 = 0x02,
	KEY_TL3 = 0x04,
	KEY_TR1 = 0x08,
	KEY_TR2 = 0x10,
	KEY_TR3 = 0x20
} key_code;

static unsigned char version = 0;
static unsigned char TSP_keys_status[TSP_DATA_SIZE];
static unsigned char keys_status;


#ifdef HAVE_USB_H

/* buffer for USB input */
static int maxpacketsize = 0;
static unsigned char *rb_head = NULL;
static unsigned char *rb_tail = NULL;
static unsigned char *rb_buffer = NULL;
 
static char
usb_drvinit (brli_term *term, const char *pathname)
{
	struct usb_bus *busses;
	struct usb_bus *bus;
	struct usb_device *dev = NULL;

	/* Looking for a Baum on the USB bus */
	busses = usb_get_busses ();
	for (bus = busses; bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			/* Check if this device is a Baum */
			/* The USB vendor ID corresponds to Future Technology Devices International Inc. */
			if (dev->descriptor.idVendor != 0x0403)
			{
				continue;
			}
			else
			{
				switch (dev->descriptor.idProduct)
				{
				case 0xfe71:  /* Pocket Vario 24 */
				case 0xfe72:  /* Super Vario 40 */
				case 0xfe73:  /* Super Vario 32 */
				case 0xfe74:  /* Super Vario 64 */
				case 0xfe75:  /* Super Vario 80 */
					break;

				default:
					brli_log (LOG_NOTICE, "Unknown Baum USB display or other FTDI device; ID=0x%x", dev->descriptor.idProduct);
				}
				break;
			}
		}
		if (dev)
			break;

	}
	if (!dev)
	{
		brli_seterror ("No Baum USB display detected");
		return 0;
	}

 	/* Opening device */
 	term->usbhandle = usb_open (dev);
 	if (!term->usbhandle)
 	{
 		brli_seterror ("Error opening device");
 		return 0;
 	}
 
 	/* Claiming the device */
 	if (usb_claim_interface (term->usbhandle, 0) < 0)
 	{
 		brli_seterror ("Error claiming interface: %s\nDo you have the correct rights on the usb device?", strerror (errno));
 		return 0;
 	}
 	term->interface_claimed = 1;
 	brli_log (LOG_NOTICE, "Interface claimed successfully");
 
// 	/* Invalid on Mac */
// 	/*
// 	if (usb_set_configuration (term->usbhandle, 1) < 0)
// 	{
// 		brli_seterror ("Error setting configuration: %s", strerror (errno));
// 		return 0;
// 	}
// 	*/

 	if (usb_set_altinterface (term->usbhandle, 0) < 0)
 	{
 		brli_seterror ("Error changing interface: %s", strerror (errno));
 		return 0;
 	}

 	/* Configuring USB-serial adapter correctly */
	/* Set baud rate to 19200bps */
 	if (usb_control_msg (term->usbhandle, 0x40, 0x3, 0x809C, 0, NULL, 0, 1000) < 0)
 	{
 		brli_log (LOG_WARNING, "Error sending control message");
 	}

	/* no flow control */
 	if (usb_control_msg (term->usbhandle, 0x40, 0x2, 0x0, 0, NULL, 0, 1000) < 0)
 	{
 		brli_log (LOG_WARNING, "Error sending control message");
 	}

	/* 8 data bits - parity: None - 1 stop bit */
 	if (usb_control_msg (term->usbhandle, 0x40, 0x4, 0x8, 0, NULL, 0, 1000) < 0)
 	{
 		brli_log (LOG_WARNING, "Error sending control message");
 	}
 
	maxpacketsize = dev->config->interface[0].altsetting[0].endpoint[1].wMaxPacketSize;

	rb_buffer = malloc (maxpacketsize);
	if (!rb_buffer)
	{
		brli_seterror ("%s", strerror (errno));
		return 0;
	}

 	rb_head = rb_tail = rb_buffer;

	return 1;
}

BRLDRV_API char
usb_drvclose (brli_term *term)
{
	if (term->interface_claimed)
		usb_release_interface (term->usbhandle, 0);
	if (rb_buffer)
		free (rb_buffer);
	if (term->usbhandle)
		usb_close (term->usbhandle);
	return 1;
}

static int
usb_read_byte (brli_term *term, unsigned char *c, int timeout)
{
	int size;

	if (rb_tail == rb_head)
	{
		/* Fill buffer */
		size = usb_bulk_read (term->usbhandle, 0x81, rb_buffer, maxpacketsize, timeout);
		if (size < 0)
		{
			brli_log (LOG_ERR, "couldn't read on usb: %s", strerror (errno));
			brli_seterror ("%s", strerror (errno));
			return size;
		}
		else if (size < 3)
		{
			return 0;
		}
		else
		{
			int i;
			
			brli_log (LOG_DEBUG, "Read some data");
#ifdef DEBUG
			printf ("read: ");
			for (i = 0; i < size; i++)
			{
				printf ("0x%x, ", rb_buffer[i]);
			}
			printf ("\n");
#endif		    
		}
		/* Skip two first bytes: garbage from the USB - serial adapter */
		rb_head = rb_buffer + 2;
		rb_tail = rb_buffer + size;
	}
	*c = *rb_head;
	rb_head++;
	return 1;
}

static int
usb_read_bytes (brli_term *term, unsigned char *data, int length, int timeout)
{
	int size;
	unsigned char *pos = data;
	unsigned char *end = data + length;

	while (pos < end)
	{
		if (rb_tail == rb_head)
		{
			/* Fill buffer */
			size = usb_bulk_read (term->usbhandle, 0x81, rb_buffer, maxpacketsize, timeout);
			if (size < 0)
			{
				brli_log (LOG_ERR, "couldn't read on usb: %s", strerror (errno));
				brli_seterror ("%s", strerror (errno));
				return size;
			}
			else if (size < 3)
			{
				return 0;
			}
			else
			{
				int i;
				
				brli_log (LOG_DEBUG, "Read some data");
#ifdef DEBUG
				printf ("read: ");
				for (i = 0; i < size; i++)
				{
					printf ("0x%x, ", rb_buffer[i]);
				}
				printf ("\n");
#endif
			}
			rb_head = rb_buffer + 2; /* Skip two first bytes: garbage from the USB - serial adapter */
			rb_tail = rb_buffer + size;
		}

		size = ((end - pos > rb_tail - rb_head)? rb_tail - rb_head : end - pos);
		memcpy (pos, rb_head, size);
		rb_head += size;
		pos += size;
	}
	return length;
}

static int
usb_write (brli_term *term, char *bytes, int size)
{
	return usb_bulk_write (term->usbhandle, 0x2, bytes, size, 0);
}

#endif /* HAVE_USB_H */


static char
serial_drvinit (brli_term *term, const char *pathname)
{
	struct termios tiodata;

	if(brli_open(term, pathname) == -1)
	{
		brli_seterror("Port open failed: %s: %s", pathname, brli_geterror());
		return 0;
	}

	/* If we got it open, get the attributes of the port */
	if(brli_tcgetattr(term, &tiodata))
	{
		brli_seterror("brli_tcgetattr failed on %s: %s", pathname, brli_geterror());
		return 0;
	}

	/* Serial port parameters */
	/* baud rate: 19200bps */
	/* no flow control */
	/* 8 data bits - parity: None - 1 stop bit */
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
		return 0;
	}
	brli_log(LOG_NOTICE, "Port init success: %s", pathname);

	return 1;
}

BRLDRV_API char
serial_drvclose (brli_term *term)
{

	brli_close(term);
	return 1;
}

static int
serial_read_byte (brli_term *term, unsigned char *c, int timeout)
{
	if (brli_timeout(term, 0, timeout / 100) == -1)
	{
		brli_seterror("Changing port timeouts failed");
		return -1;
	}
	return brli_sread(term, c, 1);
}

static int
serial_read_bytes (brli_term *term, unsigned char *data, int length, int timeout)
{
	if (brli_timeout(term, 0, timeout / 100) == -1)
	{
		brli_seterror("Changing port timeouts failed");
		return -1;
	}
	return brli_sread(term, data, length);
}

static int
serial_write (brli_term *term, char *bytes, int size)
{
	return brli_swrite(term, bytes, size);
}


BRLDRV_API const char *
brli_drvinfo (brli_term *term, brl_config code)
{
	switch (code)
	{
	case BRL_DRIVER:
		return "baumusb";
	case BRL_TERMINAL:
		if (!term->name)
			return "None";
		else
			return term->name;
	default:
		return NULL;
	}
}



BRLDRV_API char
brli_drvinit (brli_term *term, char type, const char *pathname)
{
	int i;
	
	/* TODO: if type is BRL_TYPE_UNKNOWN, try both USB then serial */

	if (type == BRL_TYPE_USB)
#ifdef HAVE_USB_H
 	{
 		drvinit_func = usb_drvinit;
 		drvclose_func = usb_drvclose;
 		write_func = usb_write;
 		read_byte_func = usb_read_byte;
 		read_bytes_func = usb_read_bytes;
 	}
#else
	{
		brli_seterror ("USB support not activated");
		return 0;
	}
#endif /* HAVE_USB_H */
	else
	{
		drvinit_func = serial_drvinit;
		drvclose_func = serial_drvclose;
		write_func = serial_write;
		read_byte_func = serial_read_byte;
		read_bytes_func = serial_read_bytes;
	}

	if ((*drvinit_func) (term, pathname) == 0)
		return 0;

	{
		/* Start Baum Escape protocol */
		const unsigned char start_protocol[] = { ESCAPE, RQT_PROTOCOL , 0x1 }; 
		
		if ((*write_func) (term, start_protocol, sizeof (start_protocol))
		    < sizeof (start_protocol))
		{
			brli_log (LOG_ERR, "couldn't write on usb: %s", strerror (errno));
			brli_seterror ("%s", strerror (errno));
			return 0;
		}
	}

 	/* Detect model of device */
 	for (i = 0; i < 10; i++)
 	{
		const unsigned char request_version[] = { ESCAPE, RQT_VERSION_NUMBER };
		const unsigned char request_id[] = { ESCAPE, RQT_DEVICE_ID };		
 		const unsigned char request_size[] = { ESCAPE, RQT_DISPLAY_DATA, 0x0 };
 		unsigned char data[64];
		int pos;
 		int size;
		int j;

		unsigned char c;
		unsigned char started;

 		if ((*write_func) (term, request_version, sizeof (request_version))
		    < sizeof (request_version))
 		{
 			brli_log (LOG_ERR, "couldn't write on usb: %s", strerror (errno));
 			brli_seterror ("%s", strerror (errno));
 			return 0;
 		}
 
 		if ((*write_func) (term, request_id, sizeof (request_id)) < sizeof (request_id))
 		{
 			brli_log (LOG_ERR, "couldn't write on usb: %s", strerror (errno));
 			brli_seterror ("%s", strerror (errno));
 			return 0;
 		}

 		if ((*write_func) (term, request_size, sizeof (request_size)) < sizeof (request_size))
 		{
 			brli_log (LOG_ERR, "couldn't write on usb: %s", strerror (errno));
 			brli_seterror ("%s", strerror (errno));
 			return 0;
 		}

		started = 0;
		while (size = (*read_byte_func) (term, &c, 1000)
		       && (term->width < 1 || term->name == NULL || version == 0))
		{
			if (size < 0)
			{
				return 0;
			}

			if (!started)
			{
				if (c != ESCAPE)
				{
					brli_log (LOG_DEBUG, "Discarding byte 0x%x", c);
					continue;
				}
				else
				{
					started = 1;
					continue;
				}
			}

			switch (c)
			{
			case ESCAPE:
				break;
			case ANS_CELLS_NUMBER:
				size = (*read_byte_func) (term, &c, 1000);
				if (size < 1)
				{
					return 0;
				}
				term->width = c;
				brli_log (LOG_NOTICE, "Found display size: %d", term->width);
				break;
			case ANS_VERSION_NUMBER:
				size = (*read_byte_func) (term, &c, 1000);
				if (size < 1)
				{
					brli_log (LOG_ERR, "couldn't read on usb: %s", strerror (errno));
					brli_seterror ("%s", strerror (errno));
					return 0;
				}
				version = c;
				brli_log (LOG_NOTICE, "Found display version: %d", version);
				break;
			case ANS_DEVICE_ID:
				term->name = malloc(17);
				term->name[16] = '\0';
				size = (*read_bytes_func) (term, term->name, 16, 1000);
				if (size < 16)
				{
					brli_log (LOG_ERR, "couldn't read on usb: %s", strerror (errno));
					brli_seterror ("%s", strerror (errno));
					return 0;
				}
				brli_log (LOG_NOTICE, "Found Baum display with Id: %s", term->name);
				break;
			default:
				brli_log (LOG_DEBUG, "Unknown infotype 0x%x", c);
			}
			started = 0;
		}
		if (term->width > 0 && term->name != NULL && version > 0)
			break;
	}
	if (term->width < 1)
	{
		brli_seterror ("Unable to determine size of Baum Braille display; is the display on?");
		return 0;
	}
	else
	{
		brli_log (LOG_NOTICE, "Found Baum Display with Id: %s, size: %d and version: %d",
			  term->name, term->width, version);
	}

 	term->display = (unsigned char *)malloc (term->width);
 	term->display_ascii = (unsigned char *)malloc (term->width);
	term->temp = (unsigned char *)malloc (2 + 2 * term->width);
	if (!term->display || !term->display_ascii || !term->temp)
 	{
 		brli_seterror ("%s", strerror (errno));
 		return 0;
 	}
 	
 	memset (TSP_keys_status, 0, sizeof (TSP_keys_status));
	keys_status = 0;

 	return 1;
}

BRLDRV_API char
brli_drvclose (brli_term *term)
{
	(*drvclose_func) (term);

	if (term->temp)
		free (term->temp);
	if (term->display)
		free (term->display);
	if (term->display_ascii)
		free (term->display_ascii);
	if (term->status)
		free (term->status);
	if (term->status_ascii)
		free (term->status_ascii);
	term->width = -1;
	return 1;
}

BRLDRV_API char
brli_drvstatus (brli_term *term)
{
	return 1;
}

BRLDRV_API char
brli_drvwrite (brli_term *term)
{
	unsigned char *pos;
	int i;

	pos = term->temp;
	*pos = ESCAPE;
	pos++;
	*pos = RQT_DISPLAY_DATA;
	pos++;

	for (i = 0; i < term->width; i++)
	{
		*pos = term->display[i];
		pos++;
		if (term->display[i] == ESCAPE)
		{
			*pos = term->display[i];
			pos++;
		}
	}

	if ((*write_func) (term, term->temp, pos - term->temp) < pos - term->temp)
	{
		brli_seterror ("Error writing data");
		return 0;
	}

	return 1;
}

BRLDRV_API signed char
brli_drvread (brli_term *term, brl_key* key)
{
	unsigned char data[TSP_DATA_SIZE];
	unsigned char c, skip;
	int size;
	int i, j;

	key->type = BRL_NONE;

	/* Find packet start */
	do
	{
		size = (*read_byte_func) (term, &c, term->timeout);
		if (size < 1)
		{
			return size;
		}
	}
	while (c != ESCAPE);

	/* Read infotype */
	size = (*read_byte_func) (term, &c, term->timeout);
	if (size < 1)
	{
		return size;
	}

	if (c == ANS_TSP_DATA) /* Cursor routing keys */
	{
		skip = 0;
		for (i = 0; i < TSP_DATA_SIZE; i++)
		{
			size = (*read_byte_func) (term, &c, term->timeout);
			if (size < 1)
			{
				return size;
			}
			if (c == ESCAPE)
			{
				if (!skip)
				{
					i--;
					skip = 1;
					continue;
				}
				else
				{
					skip = 0;
				}

			}
			data[i] = c;
		}
		for (i = 0; key->type == BRL_NONE && i < TSP_DATA_SIZE; i++)
		{
			if (data[i] != TSP_keys_status[i])
			{
				for (j = 0; key->type == BRL_NONE && j < 8; j++)
				{
					/* key pressed */
					if (!(TSP_keys_status[i] & (1 << j)) && (data[i] & (1 << j)))
					{
						/* cursor routing key */
						key->type = BRL_CURSOR;
						key->code = (i * 8 + j);
					}
				}
			}
		}
		memcpy (TSP_keys_status, data, TSP_DATA_SIZE);
	}
	else if (c == ANS_BUTTON_DATA)
	{
		skip = 0;
		for (i = 0; i < 1; i++)
		{
			size = (*read_byte_func) (term, &c, term->timeout);
			if (size < 1)
			{
				return size;
			}
			if (c == ESCAPE)
			{
				if (!skip)
				{
					i--;
					skip = 1;
					continue;
				}
				else
				{
					skip = 0;
				}
			}
		}

		if (c == keys_status)
		{
			return 0;
		}

		switch (keys_status & ~c)
		{
		case 0: /* all key release */
			break;
		case KEY_TL1:
			key->type = BRL_CMD;
			key->code = BRLK_UP;
			break;
		case KEY_TL2:
			key->type = BRL_CMD;
			key->code = BRLK_ABOVE;
			break;
		case KEY_TL3:
			key->type = BRL_CMD;
			key->code = BRLK_BACKWARD;
			break;
		case KEY_TR1:
			key->type = BRL_CMD;
			key->code = BRLK_DOWN;
			break;
		case KEY_TR2:
			key->type = BRL_CMD;
			key->code = BRLK_BELOW;
			break;
		case KEY_TR3:
			key->type = BRL_CMD;
			key->code = BRLK_FORWARD;
			break;
		default:
			key->type = BRL_CMD;
			key->code = BRLK_UNKNOWN;
			brli_log (LOG_DEBUG, "Unknown key combination released", keys_status & ~c);
		}
		
		keys_status = c;
	}
	
	if (key->type != BRL_NONE)
	{
		return 1;
	}

	return 0;
}
