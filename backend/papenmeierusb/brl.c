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

/* ring buffer for USB input */
#define RBSIZE 0x40
static unsigned char *rb_head;
static unsigned char *rb_tail;
static unsigned char rb_buffer[RBSIZE];

static unsigned char keys_status[0x40];

typedef struct
{
	unsigned char id;
	const char *name;
	unsigned char width;
	unsigned char status_width;
} papenmeier_model;

static papenmeier_model models[] =
{
	{ 0x44, "Braillex EL 40P", 40, 0},
	{ 0x45, "Braillex ELba", 32, 1},
	{ 0x55, "Braillex EL 40S", 40, 0}
};

static int model = -1;

#define SOT 0x02
#define EOT 0x03
#define CMD_INIT 0x42
#define TRAME_SIZE 0x40

#define BRL_TO_PAPENMEIER(a) ((((a) & 1) << 7) | (((a) & 2) << 5) | (((a) & 4) << 3) \
| (((a) & 8) << 1) | (((a) & 16) >> 1) | (((a) & 32) >> 3) | (((a) & 64) >> 5 ) \
| (((a) & 128) >> 7))

static int
rb_read_byte (usb_dev_handle *dev, int ep, int timeout, unsigned char *c)
{
	int size;

	if (rb_tail == rb_head)
	{
		/* Fill buffer */
		size = usb_bulk_read (dev, ep, rb_buffer, 0x40, timeout);
		if (size < 1)
		{
			brli_log (LOG_DEBUG, "Incomplete Trame");
			return 0;
		}
		/*
		else
		{
			if (size > 2)
			{
				int i;

				brli_log (LOG_DEBUG, "Read some data");
				printf ("read: ");
				for (i = 0; i < size; i++)
				{
					printf ("0x%x, ", rb_buffer[i]);
				}
				printf ("\n");

			}
		}
		*/
		rb_head = rb_buffer;
		rb_tail = rb_buffer + size;
	}
	*c = *rb_head;
	rb_head++;
	return 1;
}

static int
read_trame (usb_dev_handle *dev, int ep, char *bytes, int timeout)
{
	unsigned int in_offset, out_offset;
	unsigned char type;
	unsigned char value;
	unsigned char trame_size;
	unsigned char c;

	trame_size = 0;
	in_offset = 0;
	out_offset = 0;
	while (rb_read_byte (dev, ep, timeout, &c) > 0)
	{
		if (c == 0x1 || c == 0x60)
		{
			continue;
		}
		else if (c == SOT)
		{
			if (out_offset > 0)
			{
				brli_log (LOG_WARNING, "Discarding previous trame");
				out_offset = 0;
			}
			bytes[out_offset] = SOT;
			in_offset++;
			out_offset++;
			continue;
		}
		else if (c == EOT)
		{
			if (in_offset > 3 && ((bytes[1] == 0x0a) || (in_offset == trame_size * 2 + 4)))
			{
				bytes[out_offset] = EOT;
				out_offset++;
				brli_log (LOG_DEBUG, "Received valid trame");
				/*
				printf ("trame: ");
				for (j = 0; j < out_offset; j++)
				{
					printf ("0x%x, ", bytes[j]);
				}
				printf ("\n");
				*/
				return out_offset;
			}
			if (in_offset > 0 || trame_size > 0)
			{
				brli_log (LOG_WARNING, "Invalid end: in_offset = %d, trame_size = %d", in_offset, trame_size);
			}
			return 0;
		}
		else
		{
			type = (c & 0xF0) >> 4;
			value = c & 0x0F;

			switch (in_offset)
			{
			case 0: /* discard until SOT */
				break;
			case 1: /* type of trame */
				if (type != 4)
				{
					brli_log (LOG_WARNING, "Invalid Trame type 0x%x", c);
					return 0;
				}
				bytes[out_offset] = value;
				in_offset++;
				out_offset++;
				break;
			case 2:
				if (type != 5)
				{
					brli_log (LOG_WARNING, "Invalid Trame Size 0x%x", c);
					return 0;
				}
				trame_size = value << 4;
				in_offset++;
				break;
			case 3:
				if (type != 5)
				{
					brli_log (LOG_WARNING, "Invalid Trame size 0x%x", c);
					return 0;
				}
				trame_size |= value;
				bytes[out_offset] = trame_size;
				in_offset++;
				out_offset++;
				break;
			default:
				if (type != 3)
				{
					brli_log (LOG_WARNING, "Invalid Trame Data 0x%x at in_offset 0x%x", c, in_offset);
					return 0;
				}

				if (!(in_offset % 2))
				{
					bytes[out_offset] = value << 4;
					in_offset++;
				}
				else
				{
					bytes[out_offset] += value;
					in_offset++;
					out_offset++;
				}
				break;
			}
		}
	}
	brli_log (LOG_WARNING, "Invalid Trame");
	return 0;
}

BRLDRV_API const char *
brli_drvinfo (brli_term *term, brl_config code)
{
	switch (code)
	{
	case BRL_DRIVER:
		return "papenmeierusb";
	case BRL_TERMINAL:
		if (model == -1)
			return "None";
		else
			return models[model].name;
	default:
		return NULL;
	}
}

BRLDRV_API char
brli_drvinit (brli_term *term, char type, const char *pathname)
{
	struct usb_bus *busses;
	struct usb_bus *bus;
	struct usb_device *dev = NULL;
	signed char id = -1;
	int i;

	rb_head = rb_tail = rb_buffer;

	/* Looking for a Papenmeier on the USB bus */
	busses = usb_get_busses ();
	for (bus = busses; bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			/* Check if this device is a Papenmeier */
			/* The USB vendor ID corresponds to Future Technology Devices International Inc. */
			if (dev->descriptor.idVendor != 0x0403)
			{
				continue;
			}
			else
			{
				switch (dev->descriptor.idProduct)
				{
				case 0xf208:
					break;
				default:
					brli_log (LOG_NOTICE, "Unknown Papenmeier USB display; ID=0x%x", dev->descriptor.idProduct);
				}
				break;
			}
		}
		if (dev)
			break;
	}
	if (!dev)
	{
		brli_seterror ("No Papenmeier USB display detected");
		return 0;
	}

	/* Opening device */
	term->usbhandle = usb_open (dev);
	if (!term->usbhandle)
	{
		brli_seterror ("Error opening device");
		return 0;
	}

	/* Configuring USB-serial adapter correctly */
	if (usb_control_msg (term->usbhandle, 0x40, 0x3, 0x34, 0, NULL, 0, 1000) < 0)
	{
		brli_log (LOG_WARNING, "Error sending control message");
	}
	if (usb_control_msg (term->usbhandle, 0x40, 0x2, 0x0, 0, NULL, 0, 1000) < 0)
	{
		brli_log (LOG_WARNING, "Error sending control message");
	}
	if (usb_control_msg (term->usbhandle, 0x40, 0x4, 0x8, 0, NULL, 0, 1000) < 0)
	{
		brli_log (LOG_WARNING, "Error sending control message");
	}

	/* Opening device */
	term->usbhandle = usb_open (dev);
	if (!term->usbhandle)
	{
		brli_seterror ("Error reopening device");
		return 0;
	}
	brli_log (LOG_NOTICE, "Device reopened successfully");

	/* Invalid on Mac */
	/*
	if (usb_set_configuration (term->usbhandle, 1) < 0)
	{
		brli_drvclose (term);
		brli_seterror ("Error setting configuration: %s", strerror (errno));
		return 0;
	}
	*/

	/* Claiming the device */
	if (usb_claim_interface (term->usbhandle, 0) < 0)
	{
		brli_drvclose (term);
		brli_seterror ("Error claiming interface: %s\nDo you have the correct rights on the usb device?", strerror (errno));
		return 0;
	}
	term->interface_claimed = 1;
	brli_log (LOG_NOTICE, "Interface claimed successfully");

	if (usb_set_altinterface (term->usbhandle, 0) < 0)
	{
		brli_drvclose (term);
		brli_seterror ("Error changing interface: %s", strerror (errno));
		return 0;
	}

	usb_clear_halt (term->usbhandle, 0x02);
	usb_clear_halt (term->usbhandle, 0x81);

	/* Detect model of device */
	for (i = 0; i < 10; i++)
	{
		unsigned char trame_init[] = { SOT, CMD_INIT, 0x50, 0x50, EOT };
		unsigned char data[0x40];
		int size;
		
		if (usb_bulk_write (term->usbhandle, 0x2, trame_init, sizeof (trame_init), 0) < sizeof (trame_init))
		{
			brli_log (LOG_ERR, "couldn't write on usb: %s", strerror (errno));
			brli_drvclose (term);
			brli_seterror ("%s", strerror (errno));
			return 0;
		}

		size = read_trame (term->usbhandle, 0x81, data, 1000);
		/*
		printf ("read: ");
		for (j = 0; j < size; j++)
		{
			printf ("0x%x, ", data[j]);
		}
		printf ("\n");
		*/

		if (size > 4 && data[1] == 0x0a)
		{
			id = data[3];
			brli_log (LOG_NOTICE, "Papenmeier display detected with id=0x%x", id);
			break;
		}
		else
		{
			brli_log (LOG_WARNING, "Invalid answer");
		}
	}
	if (id == -1)
	{
		brli_drvclose (term);
		brli_seterror ("No valid ID packet received");
		return 0;
	}
	for (i = 0; i < sizeof (models) / sizeof (papenmeier_model); i++)
	{
		if (models[i].id == id)
		{
			model = i;
			term->width = models[i].width;
			term->status_width = models[i].status_width;
			break;
		}
	}
	if (model == -1)
	{
		brli_drvclose (term);
		brli_seterror ("Model with unknow ID=0x%x please contact the libbraille team.", id);
		return 0;
	}
	
	if (term->status_width > 0)
	{
		term->status = (unsigned char *)malloc (term->status_width);
		term->status_ascii = (unsigned char *)malloc (term->status_width);
		/* TODO: Remove later */
		memset (term->status, 0, term->status_width);
	}
	term->temp = (unsigned char *)malloc (5 + term->status_width + term->width);
	term->display = (unsigned char *)malloc (term->width);
	term->display_ascii = (unsigned char *)malloc (term->width);
	if (!term->display || !term->display_ascii || !term->temp  ||
	    (term->status_width > 0 && (!term->status_ascii || !term->status)))
	{
		brli_drvclose (term);
		brli_seterror ("%s", strerror (errno));
		return 0;
	}
	
	rb_head = rb_tail = rb_buffer;
	memset (keys_status, 0, sizeof (keys_status));

	return 1;
}

BRLDRV_API char
brli_drvclose (brli_term *term)
{
	if (term->interface_claimed)
	{
		usb_release_interface (term->usbhandle, 0);
	}
	if (term->usbhandle)
	{
		usb_close (term->usbhandle);
	}
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
	*pos = SOT;
	pos++;
	*pos = 0x43;
	pos++;

	/* number of cells to write */
	*pos = 0x50 | ((term->status_width + term->width + 4) >> 4);
	pos++;
	*pos = 0x50 | ((term->status_width + term->width + 4) & 0x0F);
	pos++;
	*pos = 0x30;
	pos++;
	*pos = 0x30;
	pos++;
	*pos = 0x30;
	pos++;
	*pos = 0x30;
	pos++;

	for (i = 0; i < term->width; i++)
	{
		*pos = 0x30 | (BRL_TO_PAPENMEIER(term->display[i]) >> 4);
		pos++;
		*pos = 0x30 | (BRL_TO_PAPENMEIER(term->display[i]) & 0x0F);
		pos++;
	}
	for (i = 0; i < term->status_width; i++)
	{
		*pos = 0x30 | (term->status[i] >> 4);
		pos++;
		*pos = 0x30 | (term->status[i] & 0x0F);
		pos++;
	}

	*pos = 0x30;
	pos++;
	*pos = 0x30;
	pos++;
	*pos = 0x30;
	pos++;
	*pos = 0x30;
	pos++;

	*pos = EOT;
	pos++;

	if (usb_bulk_write (term->usbhandle, 0x2, term->temp, pos - term->temp, 0) < pos - term->temp)
	{
		brli_seterror ("Error writing data");
		return 0;
	}


	return 1;
}

BRLDRV_API signed char
brli_drvread (brli_term *term, brl_key* key)
{
	int size, i, j;
	unsigned char data[0x40];
	
	size = read_trame (term->usbhandle, 0x81, data, term->timeout);
	if (size < 0)
	{
		brli_seterror ("%s", strerror (errno));
		return -1;
	}
	else if (size < 5)
	{
		return 0;
	}
	
	brli_log (LOG_DEBUG, "Read valid key trame of size 0x%x", size);

	key->type = BRL_NONE;
	
	for (i = 0; key->type == BRL_NONE && i < data[2]; i++)
	{
		if (data[i + 3] != keys_status[i + 3])
		{
			for (j = 0; key->type == BRL_NONE && j < 8; j++)
			{
				/* key released */
				/*
				if ((keys_status[i + 3] & (1 << j)) && !(data[i + 3] & (1 << j)))
				{
 					switch (i * 8 + j)
 					{
 					default:
 						key->type = BRL_CMD;
 						key->code = BRLK_UNKNOWN;
 						brli_log (LOG_DEBUG, "Unknown key released at byte 0x%x and bit 0x%x", i, j);
 					}
 				}
 				if (key->type != BRL_NONE)
 					break;
				*/

				/* key pressed */
				if (!(keys_status[i + 3] & (1 << j)) && (data[i + 3] & (1 << j)))
				{
					if ((i * 8 + j) >= 28 && (i * 8 + j) < 108)
					{
						/* cursor routing key */
						key->type = BRL_CURSOR;
						key->code = (i * 8 + j - 28) / 2;
					}
					else switch (i * 8 + j)
					{
					case 16: /* front bar further up */
						key->type = BRL_CMD;
						key->code = BRLK_UP;
						break;
					case 17: /* front bar up */
						key->type = BRL_CMD;
						key->code = BRLK_ABOVE;
						break;
					case 18: /* front bar down */
						key->type = BRL_CMD;
						key->code = BRLK_BELOW;
						break;
					case 19: /* front bar further down */
						key->type = BRL_CMD;
						key->code = BRLK_DOWN;
						break;
					case 20: /* front bar right */
						key->type = BRL_CMD;
						key->code = BRLK_FORWARD;
						break;
					case 21: /* front bar left */
						key->type = BRL_CMD;
						key->code = BRLK_BACKWARD;
						break;
					case 22: /* front bar further right */
						key->type = BRL_CMD;
						key->code = BRLK_END;
						break;
					case 23: /* front bar further left */
						key->type = BRL_CMD;
						key->code = BRLK_HOME;
						break;
					case 24: /* left scroll button up */
						key->type = BRL_CMD;
						key->code = BRLK_UP;
						break;
					case 25: /* left scroll button down */
						key->type = BRL_CMD;
						key->code = BRLK_DOWN;
						break;						
					case 108: /* right scroll button up */
						key->type = BRL_CMD;
						key->code = BRLK_ABOVE;
						break;
					case 109: /* right scroll button down */
						key->type = BRL_CMD;
						key->code = BRLK_BELOW;
						break;
					default:
						key->type = BRL_CMD;
						key->code = BRLK_UNKNOWN;
						brli_log (LOG_DEBUG, "Unknown key pressed at byte 0x%x and bit 0x%x", i, j);
					}
				}
			}
		}
	}

	memcpy (keys_status, data, size - 1);

	if (key->type != BRL_NONE)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
