/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */
/* Braille information structure */

#ifndef	_BRL_TERM_H
#define	_BRL_TERM_H	1

#include "config.h"

/* Make sure the correct platform symbols are defined */
#if !defined(WIN32) && defined(_WIN32)
#define WIN32
#endif /* Windows */

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#else
#ifdef WIN32
#include <windows.h>
#include <brl_termios.h>
#else
#error Need termios or win32
#endif
#endif

#ifdef HAVE_USB_H
#include <usb.h>
#endif

typedef struct
{
  unsigned char *temp;           /* for terminals that may require this */
  unsigned char *temp1;
  unsigned char *temp2;
  unsigned char *temp3;
  unsigned char *display;        /* contents of the display in braille representation */
  unsigned char *display_ascii;  /* contents of the display in ascii representation */
  unsigned char *status;
  unsigned char *status_ascii;
  signed short width;            /* size of the display */
  signed char status_width;      /* number of status cells */

  char old_term_ok;              /* Indicates wether the old parameters of the
			            communication port has been saved */

  char *name;                    /* Name of display */

#ifdef HAVE_TERMIOS_H
  int fd;
  struct termios oldtio;

#else
#ifdef WIN32
  HANDLE handle;

  DCB olddcb;                  /* Old parameters for the port */
  COMMTIMEOUTS oldtimeouts ;   /* Old timeouts for the port */
  DCB dcb;

#else
#error Need termios or win32
#endif
#endif

  struct termios tiodata;

  cc_t cc_min;
  cc_t cc_time;

#ifdef HAVE_USB_H
  struct usb_device *usbdev;
  usb_dev_handle *usbhandle;
  char interface_claimed;
#endif

  int timeout;

} brli_term;                     /* contains terminal infos */

#endif /* brl_term.h  */
