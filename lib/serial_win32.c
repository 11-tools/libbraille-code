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

#ifdef WIN32

BRAILLE_API void
brli_delay(int msec)
{
#ifdef HAVE_SYS_SELECT_H
  struct timeval del;
  
  del.tv_sec = 0;
  del.tv_usec = msec * 1000;
  select(0, NULL, NULL, NULL, &del);
#else
#ifdef WIN32
  Sleep(msec);
#else
#error Need select or win32
#endif
#endif
}

/* Open communication port */
BRAILLE_API int
brli_open(brli_term *term, const char *pathname)
{
  term->old_term_ok = 0;

  term->handle = CreateFile(pathname,
			    GENERIC_READ | GENERIC_WRITE,
			    0,    /* comm devices must be opened w/exclusive-access */
			    NULL, /* no security attrs */
			    OPEN_EXISTING, /* comm devices must use OPEN_EXISTING */
			    0,    /* not overlapped I/O */
			    NULL  /* hTemplate must be NULL for comm devices */
			    );
  if(term->handle == INVALID_HANDLE_VALUE)
    { 
      LPVOID lpMsgBuf;
      if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL))
	brli_seterror("Error in FormatMessage");
      else
	brli_seterror("Error opening port %s: %s", pathname, (LPCTSTR)lpMsgBuf);
      /* Free the buffer. */
      LocalFree(lpMsgBuf);
      return -1;
    }

  /* Build on the current configuration, and skip setting the size */
  /* of the input and output buffers with SetupComm. */
  if(!GetCommState(term->handle, &term->olddcb))
   {
      LPVOID lpMsgBuf;
      if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL))
	{
	  brli_seterror("Error in FormatMessage");
	}
      else
	{
	  brli_seterror("Error getting port state: %s", (LPCTSTR)lpMsgBuf);
	}
      /* Free the buffer. */
      LocalFree(lpMsgBuf);
      brli_close(term);
      return -1;
   }
  memcpy(&term->dcb, &term->olddcb, sizeof(DCB));

  if(!GetCommTimeouts(term->handle, &term->oldtimeouts))
   {
      LPVOID lpMsgBuf;
      if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL))
	{
	  brli_seterror("Error in FormatMessage");
	}
      else
	{
	  brli_seterror("Error getting port timeouts: %s", (LPCTSTR)lpMsgBuf);
	}
      /* Free the buffer. */
      LocalFree(lpMsgBuf);
      brli_close(term);
      return -1;
   }

  term->old_term_ok = 1;

  /* Stop all transmission */
  if(!SetCommBreak(term->handle))
   {
      LPVOID lpMsgBuf;
      if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL))
	{
	  brli_seterror("Error in FormatMessage");
	}
      else
	{
	  brli_seterror("Error stopping transmission: %s", (LPCTSTR)lpMsgBuf);
	}
      /* Free the buffer. */
      LocalFree(lpMsgBuf);
      brli_close(term);
      return -1;
   }

  /* Purge all transmissions */
  if(!PurgeComm(term->handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR))
    {
      LPVOID lpMsgBuf;
      if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL))
	{
	  brli_seterror("Error in FormatMessage");
	}
      else
	{
	  brli_seterror("Error purging transmissions: %s", (LPCTSTR)lpMsgBuf);
	}
      /* Free the buffer. */
      LocalFree(lpMsgBuf);
      brli_close(term);
      return -1;
    }

  /* Restart all transmission */
  if(!ClearCommBreak(term->handle))
   {
      LPVOID lpMsgBuf;
      if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL))		
	{
	  brli_seterror("Error in FormatMessage");
	}
      else
	{
	  brli_seterror("Error restarting transmission: %s", (LPCTSTR)lpMsgBuf);
	}
      /* Free the buffer. */
      LocalFree(lpMsgBuf);
      brli_close(term);
      return -1;
   }

  /* Transform the current dcb in termios format */
  switch(term->dcb.BaudRate)
    {
    case CBR_110:
      term->tiodata.c_ispeed = B110;
      term->tiodata.c_ospeed = B110;
      break;
    case CBR_300:
      term->tiodata.c_ispeed = B300;
      term->tiodata.c_ospeed = B300;
      break;
    case CBR_600:
      term->tiodata.c_ispeed = B600;
      term->tiodata.c_ospeed = B600;
      break;
    case CBR_1200:
      term->tiodata.c_ispeed = B1200;
      term->tiodata.c_ospeed = B1200;
      break;
    case CBR_2400:
      term->tiodata.c_ispeed = B2400;
      term->tiodata.c_ospeed = B2400;
      break;
    case CBR_4800:
      term->tiodata.c_ispeed = B4800;
      term->tiodata.c_ospeed = B4800;
      break;
    case CBR_9600:
      term->tiodata.c_ispeed = B9600;
      term->tiodata.c_ospeed = B9600;
      break;
#ifdef B14400
    case CBR_14400:
      term->tiodata.c_ispeed = B14400;
      term->tiodata.c_ospeed = B14400;
      break;
#endif
    case CBR_19200:
      term->tiodata.c_ispeed = B19200;
      term->tiodata.c_ospeed = B19200;
      break;
    case CBR_38400:
      term->tiodata.c_ispeed = B38400;
      term->tiodata.c_ospeed = B38400;
      break;
#ifdef B56000
    case CBR_56000:
      term->tiodata.c_ispeed = B56000;
      term->tiodata.c_ospeed = B56000;
      break;
#endif
    case CBR_57600:
      term->tiodata.c_ispeed = B57600;
      term->tiodata.c_ospeed = B57600;
      break;
    case CBR_115200:
      term->tiodata.c_ispeed = B115200;
      term->tiodata.c_ospeed = B115200;
      break;
#ifdef B128000
    case CBR_128000:
      term->tiodata.c_ispeed = B128000;
      term->tiodata.c_ospeed = B128000;
      break;
#endif
#ifdef B256000
    case CBR_256000:
      term->tiodata.c_ispeed = B256000;
      term->tiodata.c_ospeed = B256000;
      break;
#endif
    default:
      brli_seterror("Unknown speed specified; %d", term->dcb.BaudRate);
      return -1;
    }

  term->tiodata.c_cflag = CLOCAL | CREAD;
  term->tiodata.c_iflag = 0;
  term->tiodata.c_oflag = 0; /* raw output */
  term->tiodata.c_lflag = 0; /* don't echo or generate signals */

  switch(term->dcb.ByteSize)
    {
    case 5:
      term->tiodata.c_cflag |= CS5;
      break;
    case 6:
      term->tiodata.c_cflag |= CS6;
      break;
    case 7:
      term->tiodata.c_cflag |= CS7;
      break;
    case 8:
      term->tiodata.c_cflag |= CS8;
      break;
    default:
      brli_seterror("Unknown data size specified; %d", term->dcb.ByteSize);
      return -1;
    }

  if(term->dcb.fParity)
    {
      term->tiodata.c_iflag = INPCK;
      term->tiodata.c_cflag |= PARENB;
    }
  else
    {
      term->tiodata.c_iflag = IGNPAR;
    }
  switch(term->dcb.Parity)
    {
    case NOPARITY:
    case EVENPARITY:
    case MARKPARITY:
    case SPACEPARITY:
      break;
    case ODDPARITY:
      term->tiodata.c_cflag |= PARODD;
      break;
      //default:
    }

  switch(term->dcb.StopBits)
    {
    case ONESTOPBIT:
      break;
    case ONE5STOPBITS:
      break;
    case TWOSTOPBITS:
      term->tiodata.c_cflag |= CSTOPB;
    }

/* by default wait input forever */
  {
    COMMTIMEOUTS cto;

    cto.ReadIntervalTimeout = 0;
    cto.ReadTotalTimeoutMultiplier = 0;
    cto.ReadTotalTimeoutConstant = 0;
    cto.WriteTotalTimeoutMultiplier = 0;
    cto.WriteTotalTimeoutConstant = 1000;

    if(!SetCommTimeouts(term->handle, &cto))
      {
	// Handle the error.
	LPVOID lpMsgBuf;
	if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			  FORMAT_MESSAGE_FROM_SYSTEM | 
			  FORMAT_MESSAGE_IGNORE_INSERTS,
			  NULL,
			  GetLastError(),
			  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			  (LPTSTR) &lpMsgBuf,
			  0,
			  NULL))		
	  {
	    brli_seterror("Error in FormatMessage");
	  }
	else
	  {
	    brli_seterror("Error in SetCommTimeouts: %s", (LPCTSTR)lpMsgBuf);
	  }
	/* Free the buffer. */
	LocalFree(lpMsgBuf);
	return -1;
      }
    else
      {
	term->tiodata.c_cc[VMIN] = 1;
	term->tiodata.c_cc[VTIME] = 0;
      }
  }

  return 0;
}

/* Close communication port */
BRAILLE_API int
brli_close(brli_term *term)
{
  if(term->handle == INVALID_HANDLE_VALUE)
    {
      brli_seterror("Invalid file descriptor");
      return -1;
    }

  /* Flush all pending output and then close the port */
  /* Purge all transmissions */
  PurgeComm(term->handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
  if(term->old_term_ok == 1)
    {
      SetCommState(term->handle, &term->olddcb);
      SetCommTimeouts(term->handle, &term->oldtimeouts);
    }
  if(!CloseHandle(term->handle))
    {
      brli_seterror("Error closing handle");
      return -1;
    }
  term->handle = INVALID_HANDLE_VALUE;
  return 0;
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
  const unsigned char *address = (const unsigned char *)buffer;

  /* Keep on looping while there's still some data to be written */
  while(length > 0)
    {
      DWORD count;

      if(!ReadFile(term->handle, (void *)address, (unsigned long)length, &count, NULL))
	{
	  brli_seterror("Error in ReadFile");
	  return -1;
	}

      /* Stop looping if the end of the file has been reached */
      if(count == 0)
        break;

      /* In case the system call was interrupted by a signal
	 after some, but not all, of the data was written,
	 point to the remainder of the buffer and try again. */
      address += count;
      length -= count;
    }

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
      DWORD count;
      
      if(!WriteFile(term->handle, address, length, &count, NULL))
	{
	  brli_seterror("Error in WriteFile");
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
  return termios_p->c_ospeed;
}

/* Return the input baud rate stored in *TERMIOS_P.  */
BRAILLE_API speed_t
brli_cfgetispeed(const struct termios *termios_p)
{
  return termios_p->c_ispeed;
}

/* Set the output baud rate stored in *TERMIOS_P to SPEED.  */
BRAILLE_API int
brli_cfsetospeed(struct termios *termios_p, speed_t speed)
{
  termios_p->c_ospeed = speed;
  return 0;
}

/* Set the input baud rate stored in *TERMIOS_P to SPEED.  */
BRAILLE_API int
brli_cfsetispeed(struct termios *termios_p, speed_t speed)
{
  termios_p->c_ispeed = speed;
  return 0;
}

/* Put the state of FD into *TERMIOS_P.  */
BRAILLE_API int
brli_tcgetattr(brli_term *term, struct termios *termios_p)
{
  memcpy(termios_p, &term->tiodata, sizeof(struct termios));
  return 0;
}

/* Set the state of FD to *TERMIOS_P.  */
BRAILLE_API int
brli_tcsetattr(brli_term *term, int optional_actions,
	       const struct termios *termios_p)
{
  /*
    TCSANOW
    TCSADRAIN
    TCSAFLUSH
  */

  /* Transform termios in dcb format */
  switch(termios_p->c_ospeed)
    {
    case B110:
      term->dcb.BaudRate = CBR_110;
      break;
    case B300:
      term->dcb.BaudRate = CBR_300;
      break;
    case B600:
      term->dcb.BaudRate = CBR_600;
      break;
    case B1200:
      term->dcb.BaudRate = CBR_1200;
      break;
    case B2400:
      term->dcb.BaudRate = CBR_2400;
      break;
    case B4800:
      term->dcb.BaudRate = CBR_4800;
      break;
    case B9600:
      term->dcb.BaudRate = CBR_9600;
      break;
#ifdef B14400
    case B14400:
      term->dcb.BaudRate = CBR_14400;
      break;
#endif
    case B19200:
      term->dcb.BaudRate = CBR_19200;
      break;
    case B38400:
      term->dcb.BaudRate = CBR_38400;
      break;
#ifdef B56000
    case B56000:
      term->dcb.BaudRate = CBR_56000;
      break;
#endif
    case B57600:
      term->dcb.BaudRate = CBR_57600;
      break;
    case B115200:
      term->dcb.BaudRate = CBR_115200;
      break;
#ifdef B128000
    case B128000:
      term->dcb.BaudRate = CBR_128000;
      break;
#endif
#ifdef B256000
    case B256000:
      term->dcb.BaudRate = CBR_256000;
      break;
#endif
    default:
      brli_seterror("Unknown speed specified; %d", term->dcb.BaudRate);
      return -1;
    }

  switch(termios_p->c_cflag & CSIZE)
    {
    case CS5:
      term->dcb.ByteSize = 5;
      break;
    case CS6:
      term->dcb.ByteSize = 6;
      break;
    case CS7:
      term->dcb.ByteSize = 7;
      break;
    case CS8:
      term->dcb.ByteSize = 8;
      break;
    default:
      brli_seterror("Unknown data size specified");
      return -1;
    }

  if(termios_p->c_cflag & PARENB)
    {
      term->dcb.fParity = 1;
      term->dcb.Parity = EVENPARITY;
    }
  else
    {
      term->dcb.fParity = 0;
      term->dcb.Parity = NOPARITY;
    }

  if(termios_p->c_cflag & PARODD)
    {
      term->dcb.Parity = ODDPARITY;
    }

  if(termios_p->c_cflag & CSTOPB)
    {
      term->dcb.StopBits = TWOSTOPBITS;
    }
  else
    {
      term->dcb.StopBits = ONESTOPBIT;
    }

   if(!SetCommState(term->handle, &term->dcb))
     {
       // Handle the error.
      LPVOID lpMsgBuf;
      if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL))		
	{
	  brli_seterror("Error in FormatMessage");
	}
      else
	{
	  brli_seterror("Error changing port stat: %s", (LPCTSTR)lpMsgBuf);
	}
      /* Free the buffer. */
      LocalFree(lpMsgBuf);
      return -1;
     }

   if(brli_timeout(term, termios_p->c_cc[VMIN], termios_p->c_cc[VTIME]) < 0)
     {
       brli_seterror("Error changing timeout: %s", brli_geterror());
       return -1;
     }

   memcpy(&term->tiodata, termios_p, sizeof(struct termios));

   return 0;
}


BRAILLE_API int
brli_timeout(brli_term *term, cc_t cc_min, cc_t cc_time)
{
  COMMTIMEOUTS cto;

  if(cc_min == term->tiodata.c_cc[VMIN] && cc_time == term->tiodata.c_cc[VTIME])
    return 0;

  cto.WriteTotalTimeoutMultiplier = 0;
  cto.WriteTotalTimeoutConstant = 1000;

  if(cc_min)
    {
      if(cc_time)
	{
	  /* une lecture attendra au moins un caractère, et reviendra
	     dès que MIN caractères auront été reçus, ou si le temps TIME
	     est écoulé depuis la réception du dernier caractère */
	  
	  cto.ReadIntervalTimeout = cc_time * 100;
	  cto.ReadTotalTimeoutMultiplier = 0;
	  cto.ReadTotalTimeoutConstant = 0;
	}
      else
	{
	  /* la lecture ne reviendra pas avant d¡Çavoir recu MIN
	     caractères */
	  cto.ReadIntervalTimeout = 0;
	  cto.ReadTotalTimeoutMultiplier = 0;
	  cto.ReadTotalTimeoutConstant = 0;
	}
    }
  else if(cc_time)
    {
      /* la lecture reviendra dès quun caractere arrivera, ou des
	 que le delai sera écoule */

      cto.ReadIntervalTimeout = MAXDWORD;
      cto.ReadTotalTimeoutMultiplier = MAXDWORD;
      cto.ReadTotalTimeoutConstant = cc_time * 100;
    }
  else
    {
      /* la lecture reviendra immediatement, ne lisant que les
	 caracteres immediatement disponibles */
      cto.ReadIntervalTimeout = MAXDWORD;
      cto.ReadTotalTimeoutMultiplier = 0;
      cto.ReadTotalTimeoutConstant = 0;
    }

  if(!SetCommTimeouts(term->handle, &cto))
    {
      // Handle the error.
      LPVOID lpMsgBuf;
      if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL))		
	{
	  brli_seterror("Error in FormatMessage");
	}
      else
	{
	  brli_seterror("Error in SetCommTimeouts: %s", (LPCTSTR)lpMsgBuf);
	}
      /* Free the buffer. */
      LocalFree(lpMsgBuf);
      return -1;
    }
  else
    {
      term->tiodata.c_cc[VMIN] = cc_min;
      term->tiodata.c_cc[VTIME] = cc_time;
    }

  return 0;
}

/* Send zero bits on FD.  */
BRAILLE_API int
brli_tcsendbreak(brli_term *term, int duration)
{
  brli_log(LOG_ERR, "brli_tcsendbreak not implemented yet");
  return 0;
}

/* Wait for pending output to be written on FD.  */
BRAILLE_API int
brli_tcdrain(brli_term *term)
{
  brli_log(LOG_ERR, "brli_tcdrain not implemented yet");
  return 0;
}

/* Flush pending data on FD.  */
BRAILLE_API int
brli_tcflush(brli_term *term, int queue_selector)
{
  brli_log(LOG_ERR, "brli_tcflush not implemented yet");
  return 0;
}

/* Suspend or restart transmission on FD.  */
BRAILLE_API int
brli_tcflow(brli_term *term, int action)
{
  brli_log(LOG_ERR, "brli_tcflow not implemented yet");
  return 0;
}

#endif //WIN32
