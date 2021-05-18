/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#ifndef _SERIAL_H
#define _SERIAL_H 1

#include "config.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "braille.h"
#include "brl_error.h"
#include "brl_term.h"

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#else
#ifdef WIN32
#include <windows.h>
#include "brl_termios.h"
#else
#error Need termios or win32
#endif
#endif


/* old serial API */

BRAILLE_API void braillei_delay(int msec);
BRAILLE_API size_t braillei_safe_read(int fd, unsigned char *buffer, size_t length);
BRAILLE_API size_t braillei_safe_write(int fd, const unsigned char *buffer, size_t length);
BRAILLE_API int braillei_open_serial(const char *dev, char data_bits, char parity,
				     char stop_bits, unsigned int speed);
BRAILLE_API int braillei_close_serial(int fd);


/* new serial API */

BRAILLE_API void brli_delay(int msec);

/* Open a file */
BRAILLE_API int brli_open(brli_term *term, const char *pathname);

/* Close a file */
BRAILLE_API int brli_close(brli_term *term);

/* Read data safely */
BRAILLE_API size_t brli_sread(brli_term *term, unsigned char *buffer, size_t length);

/* Write data safely */
BRAILLE_API size_t brli_swrite(brli_term *term, const unsigned char *buffer, size_t length);

/* Return the output baud rate stored in *TERMIOS_P.  */
BRAILLE_API speed_t brli_cfgetospeed(const struct termios *termios_p);

/* Return the input baud rate stored in *TERMIOS_P.  */
BRAILLE_API speed_t brli_cfgetispeed(const struct termios *termios_p);

/* Set the output baud rate stored in *TERMIOS_P to SPEED.  */
BRAILLE_API int brli_cfsetospeed(struct termios *termios_p, speed_t speed);

/* Set the input baud rate stored in *TERMIOS_P to SPEED.  */
BRAILLE_API int brli_cfsetispeed(struct termios *termios_p, speed_t speed);

/* Put the state of FD into *TERMIOS_P.  */
BRAILLE_API int brli_tcgetattr(brli_term *term, struct termios *termios_p);

/* Set the state of FD to *TERMIOS_P.  */
BRAILLE_API int brli_tcsetattr(brli_term *term, int optional_actions,
			       const struct termios *termios_p);

BRAILLE_API int brli_timeout(brli_term *term, cc_t cc_min, cc_t cc_time);

/* Send zero bits on FD.  */
BRAILLE_API int brli_tcsendbreak(brli_term *term, int duration);

/* Wait for pending output to be written on FD.  */
BRAILLE_API int brli_tcdrain(brli_term *term);

/* Flush pending data on FD.  */
BRAILLE_API int brli_tcflush(brli_term *term, int queue_selector);

/* Suspend or restart transmission on FD.  */
BRAILLE_API int brli_tcflow(brli_term *term, int action);

#endif
