/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#include "config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef DEBUG
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
static struct timeval oldtime;
static struct timeval newtime;
#endif
#endif

#include "serial.h"

#ifdef HAVE_TERMIOS_H

BRAILLE_API void
brli_delay(int msec)
{
#ifdef HAVE_UNISTD_H
  unsigned int usec;

  usec = msec * 1000;
  usleep(usec);
#else
#ifdef HAVE_SYS_SELECT_H
  struct timeval del;
  
  del.tv_sec = 0;
  del.tv_usec = msec * 1000;
  select(0, NULL, NULL, NULL, &del);
#else
#ifdef WIN32
  Sleep(msec);
#else
#error Need usleep or select or win32
#endif
#endif
#endif
}

/* Open communication port */
BRAILLE_API int
brli_open(brli_term *term, const char *pathname)
{
  term->old_term_ok = 0;

  term->fd = open(pathname, O_RDWR | O_NOCTTY);
  if(term->fd == -1)
    {
      brli_seterror("%s", strerror(errno));
      return -1;
    }

  if(!isatty(term->fd))
    {
      brli_seterror("Opened device %s is not a tty", pathname);
      close(term->fd);
      return -1;
    }

  /* If we got it open, get the old attributes of the port */
  if(tcgetattr(term->fd, &term->oldtio))
    {
      brli_seterror("tcgetattr failed: %s", brli_geterror());
      brli_close(term);
      return -1;
    }
  memcpy(&term->tiodata, &term->oldtio, sizeof(struct termios));
  term->old_term_ok = 1;

  /* cleaning port state */
  if(cfsetispeed(&term->tiodata, B0))
    {
      brli_seterror("cfsetispeed failed: %s", strerror(errno));
      brli_close(term);
      return -1;
    }  
  /* TODO: this does not work on MacOSX */
  /*  if(cfsetospeed(&term->tiodata, B0))
      {
        brli_seterror("cfsetospeed failed: %s: %s", pathname, strerror(errno));
        brli_close(term);
        return -1;
      }
  */
  if(tcsetattr(term->fd, TCSANOW, &term->tiodata))
    {
      brli_seterror("tcsetattr failed: %s", strerror(errno));
      brli_close(term);
      return -1;
    }
  if(tcflush(term->fd, TCIOFLUSH))
    {
      brli_seterror("tcflush failed: %s", strerror(errno));
      brli_close(term);
      return -1;
    }
  /* TODO: Check if this works with all terminals */
  if(tcsendbreak(term->fd, 0))
    {
      brli_seterror("tcsendbreak failed: %s", strerror(errno));
      brli_close(term);
      return -1;
    }

  return term->fd;
}

/* Close communication port */
BRAILLE_API int
brli_close(brli_term *term)
{
  int error;

  if(term->fd == -1)
    {
      brli_seterror("Invalid file descriptor");
      return -1;
    }

  /* Flush all pending output and then close the port */
  tcflush(term->fd, TCIOFLUSH);
  if(term->old_term_ok == 1)
    tcsetattr(term->fd, TCSANOW, &term->oldtio); /* restore port status */

  error = close(term->fd);
  if(error == -1)
    {
      brli_seterror("%s", strerror(errno));
    }
  return error;
}

/* Read data safely by continually retrying the read system call until
   all of the requested data has been transferred or an end-of-file is
   encountered.  This routine is a wrapper for the read system call
   which is fully compatible with it except that the caller does not
   need to handle the various scenarios which can occur when a signal
   interrupts the system call. */
BRAILLE_API size_t
brli_sread(brli_term *term, unsigned char *buffer, size_t length)
{
  unsigned char *address = buffer;

#ifdef DEBUG
#ifdef HAVE_SYS_TIME_H
  gettimeofday(&oldtime, NULL);
#endif
#endif

  /* Keep looping while there's still some data to be read */
  while(length > 0)
    {
      /* Read the rest of the data */
      size_t count = read(term->fd, address, length);

      /* Handle errors */
      if(count == -1)
        {
	  /* If the system call was interrupted by a signal, then restart it */
	  if(errno == EINTR)
	    {
	      continue;
	    }

	  /* Return all other errors to the caller */
	  return -1;
	}

      /* Stop looping if the end of the file has been reached */
      if(count == 0)
        break;

      /* In case the system call was interrupted by a signal
	 after some, but not all, of the data was read,
	 point to the remainder of the buffer and try again */
      address += count;
      length -= count;
    }

#ifdef DEBUG
#ifdef HAVE_SYS_TIME_H
  gettimeofday(&newtime, NULL);
  brli_log(LOG_DEBUG, "Read time: %dms\n",
	   (int)((newtime.tv_sec - oldtime.tv_sec) * 1000000
		 + newtime.tv_usec - oldtime.tv_usec) / 1000);
#endif
#endif

  /* Return the number of bytes which were actually written */
  return address - buffer;
}

/* Write data safely by continually retrying the write system call
   until all of the requested data has been transferred.  This routine
   is a wrapper for the write system call which is fully compatible
   with it except that the caller does not need to handle the various
   scenarios which can occur when a signal interrupts the system
   call. */
BRAILLE_API size_t
brli_swrite(brli_term *term, const unsigned char *buffer, size_t length)
{
  const unsigned char *address = buffer;

  /* Keep on looping while there's still some data to be written */
  while(length > 0)
    {
      /* Write the rest of the data */
      size_t count = write(term->fd, address, length);

      /* Handle errors */
      if(count == -1)
        {
	  /* If the system call was interrupted by a signal, then restart it */
          if(errno == EINTR)
	    {
	      continue;
	    }

	  /* Return all other errors to the caller */
          return -1;
        }

      /* In case the system call was interrupted by a signal
	 after some, but not all, of the data was written,
	 point to the remainder of the buffer and try again. */
      address += count;
      length -= count;
    }
  /* Return the number of bytes which were actually written */
  return address - buffer;
}

/* Return the output baud rate stored in *TERMIOS_P.  */
BRAILLE_API speed_t
brli_cfgetospeed(const struct termios *termios_p)
{
  return cfgetospeed(termios_p);
}

/* Return the input baud rate stored in *TERMIOS_P.  */
BRAILLE_API speed_t
brli_cfgetispeed(const struct termios *termios_p)
{
  return cfgetispeed(termios_p);
}

/* Set the output baud rate stored in *TERMIOS_P to SPEED.  */
BRAILLE_API int
brli_cfsetospeed(struct termios *termios_p, speed_t speed)
{
  speed_t error;

  error = cfsetospeed(termios_p, speed);
  if(error == -1)
    {
      brli_seterror("%s", strerror(errno));
    }
  return error;
}

/* Set the input baud rate stored in *TERMIOS_P to SPEED.  */
BRAILLE_API int
brli_cfsetispeed(struct termios *termios_p, speed_t speed)
{
  speed_t error;

  error = cfsetispeed(termios_p, speed);
  if(error == -1)
    {
      brli_seterror("%s", strerror(errno));
    }
  return error;
}

/* Put the state of FD into *TERMIOS_P.  */
BRAILLE_API int
brli_tcgetattr(brli_term *term, struct termios *termios_p)
{
  int error;

  error = tcgetattr(term->fd, termios_p);
  if(error == -1)
    {
      brli_seterror("%s", strerror(errno));
    }
  return error;
}

/* Set the state of FD to *TERMIOS_P.  */
BRAILLE_API int
brli_tcsetattr(brli_term *term, int optional_actions,
	       const struct termios *termios_p)
{
  int error;

  error = tcsetattr(term->fd, optional_actions, termios_p);
  if(error == -1)
    {
      brli_seterror("%s", strerror(errno));
    }
  else
    {
      memcpy(&term->tiodata, termios_p, sizeof(struct termios));
    }
  return error;
}

BRAILLE_API int
brli_timeout(brli_term *term, cc_t cc_min, cc_t cc_time)
{
  struct termios tiodata;
  
  if(term->tiodata.c_cc[VMIN] == cc_min
     && term->tiodata.c_cc[VTIME] == cc_time)
    return 0;

  memcpy(&tiodata, &term->tiodata, sizeof(struct termios));
  tiodata.c_cc[VMIN] = cc_min;
  tiodata.c_cc[VTIME] = cc_time;
  return brli_tcsetattr(term, TCSANOW, &tiodata);
}

/* Send zero bits on FD.  */
BRAILLE_API int
brli_tcsendbreak(brli_term *term, int duration)
{
  int error;

  error = tcsendbreak(term->fd, duration);
  if(error == -1)
    {
      brli_seterror("%s", strerror(errno));
    }
  return error;
}

/* Wait for pending output to be written on FD.  */
BRAILLE_API int
brli_tcdrain(brli_term *term)
{
  int error;

  error = tcdrain(term->fd);
  if(error == -1)
    {
      brli_seterror("%s", strerror(errno));
    }
  return error;
}

/* Flush pending data on FD.  */
BRAILLE_API int
brli_tcflush(brli_term *term, int queue_selector)
{
  int error;

  error = tcflush(term->fd, queue_selector);
  if(error == -1)
    {
      brli_seterror("%s", strerror(errno));
    }
  return error;
}

/* Suspend or restart transmission on FD.  */
BRAILLE_API int
brli_tcflow(brli_term *term, int action)
{
  int error;

  error = tcflow(term->fd, action);
  if(error == -1)
    {
      brli_seterror("%s", strerror(errno));
    }
  return error;
}

#endif // HAVE_TERMIOS_H
