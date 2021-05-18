/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 * 
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#include "config.h"
#include "brl_config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32
#include <direct.h> /* for getcwd */
#define getcwd _getcwd
#define strcasecmp stricmp
#endif

#ifdef HAVE_USB_H
#include <usb.h>
#endif

#include "braille.h"
#include "brl_error.h"
#include "brl_term.h"
#include "ltdl.h"


/* Output braille translation table - default is us */
static unsigned char brailletrans[256] =
  {
    0xc8, 0xc1, 0xc3, 0xc9, 0xd9, 0xd1, 0xcb, 0xdb,
    0xd3, 0xca, 0xda, 0xc5, 0xc7, 0xcd, 0xdd, 0xd5,
    0xcf, 0xdf, 0xd7, 0xce, 0xde, 0xe5, 0xe7, 0xfa,
    0xed, 0xfd, 0xf5, 0xea, 0xf3, 0xfb, 0xd8, 0xf8,
    0x00, 0x2e, 0x10, 0x3c, 0x2b, 0x29, 0x2f, 0x04,
    0x37, 0x3e, 0x21, 0x2c, 0x20, 0x24, 0x28, 0x0c,
    0x34, 0x02, 0x06, 0x12, 0x32, 0x22, 0x16, 0x36,
    0x26, 0x14, 0x31, 0x30, 0x23, 0x3f, 0x1c, 0x39,
    0x48, 0x41, 0x43, 0x49, 0x59, 0x51, 0x4b, 0x5b,
    0x53, 0x4a, 0x5a, 0x45, 0x47, 0x4d, 0x5d, 0x55,
    0x4f, 0x5f, 0x57, 0x4e, 0x5e, 0x65, 0x67, 0x7a,
    0x6d, 0x7d, 0x75, 0x6a, 0x73, 0x7b, 0x58, 0x38,
    0x08, 0x01, 0x03, 0x09, 0x19, 0x11, 0x0b, 0x1b,
    0x13, 0x0a, 0x1a, 0x05, 0x07, 0x0d, 0x1d, 0x15,
    0x0f, 0x1f, 0x17, 0x0e, 0x1e, 0x25, 0x27, 0x3a,
    0x2d, 0x3d, 0x35, 0x2a, 0x33, 0x3b, 0x18, 0x78,
    0x62, 0xad, 0xb2, 0x92, 0x54, 0x81, 0x8e, 0x9e,
    0xb4, 0x99, 0x96, 0xd0, 0x91, 0xee, 0x60, 0x6b,
    0x76, 0xa2, 0xa0, 0x4c, 0xd4, 0x98, 0x69, 0x88,
    0x7e, 0x95, 0x63, 0xb0, 0xa4, 0x6c, 0x56, 0xeb,
    0x40, 0x64, 0x90, 0x68, 0xe4, 0xa8, 0xe2, 0xff,
    0xc2, 0x79, 0x93, 0xf7, 0xf2, 0x6e, 0xe8, 0x9b,
    0xb8, 0xd6, 0x83, 0x46, 0xb5, 0x8d, 0xff, 0x44,
    0x89, 0x77, 0x9a, 0xfe, 0xa5, 0xa7, 0xec, 0x84,
    0xc4, 0x42, 0x85, 0x66, 0x70, 0x7c, 0x5c, 0x6f,
    0xc6, 0x86, 0x80, 0x9f, 0x8b, 0xd2, 0xf0, 0x61,
    0x50, 0x72, 0x82, 0x87, 0x8a, 0xaf, 0x94, 0x97,
    0x74, 0xe0, 0x52, 0xff, 0xa6, 0xe6, 0xf4, 0xbc,
    0xb7, 0xa1, 0xe1, 0x8f, 0x9c, 0xfc, 0xdc, 0xef,
    0xae, 0xa3, 0xe3, 0xab, 0x8c, 0xa9, 0xe9, 0xbb,
    0xb6, 0x9d, 0xac, 0xb9, 0xf9, 0x71, 0xaa, 0xcc,
    0xc0, 0xbe, 0xb1, 0xf1, 0xb3, 0xf6, 0x7f, 0xbf
  };
static unsigned char asciitrans[256];

/* prototypes of local functions */
static char brli_load_driver(const char *driver, const char *path);
static char brli_init_driver(char type);

char *driver = NULL; /* name of library */
char *device = NULL; /* device on which the terminal is connected */
char *table = NULL;  /* translation table */
char *path = NULL;   /* path where libbraille as been installed */
char *pathcfg = NULL; /* path where to find the configuration file (unix only)*/
char *pathdrv = NULL; /* extra path where to find drivers*/
char *pathtbl= NULL; /* extra path where to find tables */

static lt_dlhandle library = NULL;          /* handle to driver */
static brli_term term;

static const char *(*brli_drvinfo)(brli_term *, brl_config code);
static char (*brli_drvinit)(brli_term *, char type, const char *);
static char (*brli_drvclose)(brli_term *);
static char (*brli_drvwrite)(brli_term *);
static char (*brli_drvread)(brli_term *, brl_key *);
static char (*brli_drvstatus)(brli_term *);


/** Initialize the library
    RETURN:
    0 if an error occured
    1 if everything is ok */
BRAILLE_API char
braille_init()
{
  char *oldpath; /* save the current location before moving to data dir */
  size_t size;

  brli_log(LOG_INFO, "libbraille " PACKAGE_VERSION);

  if(lt_dlinit())
    {
      brli_seterror("Error initialising libltdl : %s", lt_dlerror());
      return 0;
    }

  size = 128;
  oldpath = (char *)malloc(size);
  if(!oldpath)
    {
      brli_seterror("Cannot allocate memory for old path");
      return 0;
    }
  while(!getcwd(oldpath, size))
    {
      if(errno != ERANGE)
	{
	  brli_seterror("Cannot save old path: %s", strerror(errno));
	  return 0;
	}

      size += 128;
      oldpath = realloc(oldpath, size);
      if(!oldpath)
	{
	  brli_seterror("Cannot allocate memory for old path: %s", strerror(errno));
	  return 0;
	}
    }
  if(!getcwd(oldpath, size))
    {
      brli_seterror("Cannot save old path: %s", strerror(errno));
      return 0;
    }

  if(!brli_get_configuration())
    return 0;
  chdir(oldpath);

  brli_log(LOG_INFO, "Braille device: %s", device);

  //TODO: Initialise USB only if we are using a USB terminal

#ifdef HAVE_USB_H
#ifdef DEBUG
  usb_set_debug(1);
#endif
  usb_init();
  usb_find_busses();
  usb_find_devices();
#endif

  if(strcmp(driver, "auto") != 0)
    {
      if(!brli_load_driver(driver, pathdrv) || !brli_init_driver(BRL_TYPE_UNKNOWN))
        {
          brli_log(LOG_ERR, "Error loading driver: %s", brli_geterror());
          return 0;
        }
    }
  else /* driver autodetection */
    {
      int i;
      char done = 0;

#ifdef HAVE_USB_H
      /* first detect USB devices */
      for(i = 0; i < braille_drivernum(); i++)
	{
	  if(braille_driverauto(i) && braille_driver_is_type(i, BRL_TYPE_USB))
	    {
	      brli_log(LOG_INFO, "testing driver %s", braille_drivername(i));
	      if(brli_load_driver(braille_drivername(i), pathdrv) && brli_init_driver(BRL_TYPE_USB))
		{
		  done = 1;
		  break;
		}
	    }
	}
#endif

      /* then detect serial devices */
      if(!done)
          for(i = 0; i < braille_drivernum(); i++)
            {
	      if(braille_driverauto(i) && braille_driver_is_type(i, BRL_TYPE_SERIAL))
                {
        	  brli_log(LOG_INFO, "testing driver %s", braille_drivername(i));
        	  if(brli_load_driver(braille_drivername(i), pathdrv) && brli_init_driver(BRL_TYPE_SERIAL))
        	    {
        	      done = 1;
        	      break;
        	    }
        	}
            }
      if(!done)
	{
	  brli_seterror("No terminal detected: %s", brli_geterror());
	  return 0;
	}
    }

  brli_log(LOG_NOTICE, "Driver %s loaded with terminal %s"
	   " with %d cells, %d status cells on device %s",
	   brli_drvinfo(&term, BRL_DRIVER), brli_drvinfo(&term, BRL_TERMINAL),
	   term.width, term.status_width, device);

  if(pathtbl && chdir(pathtbl) < 0)
    {
      brli_seterror("Cannot move to table directory %s", pathtbl);
      return 0;
    }

  /* Load text translation table */
  braille_usetable(table);

  chdir(oldpath);
  free(oldpath);

  braille_statusdisplay("    ");   /* clear status cells */
  braille_display("libbraille " PACKAGE_VERSION);	/* display initialisation message */
  return 1;
}

/** Stop the library and the braille display */
BRAILLE_API char
braille_close(void)
{
  if(!brli_drvclose(&term))
    return 0;
  if(lt_dlclose(library))
    {
      brli_seterror("Cannot unload Braille driver, still in use");
      return 0;
    }
  brli_drvinfo = NULL;
  brli_drvinit = NULL;
  brli_drvclose = NULL;
  brli_drvwrite = NULL;
  brli_drvread = NULL;
  brli_drvstatus = NULL;
  if(lt_dlexit())
    {
      brli_log(LOG_INFO, "Warning: libltdl still in use");
    }
  brli_log(LOG_INFO, "Closed libbraille");
  return 1;
}

/** Info about the running terminal */
BRAILLE_API const char *
braille_info(brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      if(!term.width)
	{
	  brli_seterror("No terminal initialised");
	  return NULL;
	}
      else
	return brli_drvinfo(&term, BRL_DRIVER);
    case BRL_TERMINAL:
      if(!term.width)
	{
	  brli_seterror("No terminal initialised");
	  return NULL;
	}
      else
	return brli_drvinfo(&term, BRL_TERMINAL);
    case BRL_DEVICE:
      return device;
    case BRL_VERSION:
      return PACKAGE_VERSION;
    case BRL_TABLE:
      return table;
    case BRL_PATH:
      return path;
#ifndef WIN32
    case BRL_PATHCONF:
      return pathcfg;
#endif
    case BRL_PATHDRV:
      return pathdrv;
    case BRL_PATHTBL:
      return pathtbl;
    default:
      return NULL;
    }
}

/** Get a key from the device */
BRAILLE_API signed char
braille_read(brl_key *key)
{
  signed char error;

  key->type = BRL_NONE;

  do
    {
      error = brli_drvread(&term, key);
      if(error == -1)
	{
	  return -1; /* an error occured while reading */
	}
    }
  /* in blocking mode, loop until a valid key is returned */
  while(term.cc_min == 1 && term.cc_time == 0 && (error == 0 || key->type == BRL_NONE));

  if(error == 0)
    {
      return 0; /* no key pressed */
    }

  if(key->type == BRL_KEY)
    {
      key->code = asciitrans[key->braille];
    }
  else if(key->type == BRL_ACC)
    {
      key->type = BRL_CMD;
      key->code = asciitrans[key->braille] + BRLK_ACCORD_a - BRLK_a;
    }
  return error;
}

/** Display a simple string on the braille display
    ARGS:
    s: the string to display (must end with '\0') */
BRAILLE_API char
braille_display(const char *s)
{
  int l;

  l = strlen(s);
  if(!braille_write(s, l))
    return 0;
  if(!braille_render())
    return 0;
  return 1;
}

/** Write characters on the braille display
    need to call braille_render() after that
    ARGS:
    s: a table of char to be displayed
    n: number of char to display */
BRAILLE_API char
braille_write(const char *s, int n)
{
  int i;

  memset(term.display_ascii, ' ', term.width);
  memset(term.display, 0, term.width);
  
  if(n > term.width)
    {
      n = term.width;
    }
  for(i = 0; i < n; i++)
	{
	  unsigned char cc = s[i];
      term.display_ascii[i] = (cc >= 32 ? cc : 32);
	}
  /* Do Braille translation using text table. */
  for(i = 0; i < n; i++)
	{
	  term.display[i] = brailletrans[term.display_ascii[i]];
	}
  return 1;
}

/** Apply braille points filter to what is to be displayed
    ARGS:
    s: a table of unsigned char corresponding to braille dots
    n: number of char to display */
BRAILLE_API char
braille_filter(unsigned char s, int n)
{
  if(n > term.width)
    {
      brli_seterror("Invalid position");
      return 0;
    }
  term.display[n] |= s;
  return 1;
}

/** Display everything when using braille_write or braille_filter */
BRAILLE_API char
braille_render()
{
  return brli_drvwrite(&term);
}

/** Returns the size of the braille display
    RETS:
    size of braille display */
BRAILLE_API int
braille_size()
{
  return term.width;
}

/** Display a simple string on the braille status cells
    ARGS:
    s: the string to display (must end with '\0') */
BRAILLE_API char
braille_statusdisplay(const char *s)
{
  int l;

  l = strlen(s);
  if(!braille_statuswrite(s, l))
    return 0;
  if(!braille_statusrender())
    return 0;
  return 1;
}

/** Write characters on the braille status cells
    need to call braille_render() after that
    ARGS:
    s: a table of char to be displayed
    n: number of char to display */
BRAILLE_API char
braille_statuswrite(const char *s, int n)
{
  int i;

  memset(term.status_ascii, ' ', term.status_width);
  memset(term.status, 0, term.status_width);
  
  if(n > term.status_width)
    {
      n = term.status_width;
    }
  for(i = 0; i < n; i++)
    term.status_ascii[i] = (unsigned char)(s[i] >= 32 ? s[i] : 32);

  /* Do Braille translation using text table. */
  for(i = 0; i < n; i++)
    {
      term.status[i] = brailletrans[term.status_ascii[i]];
    }
  return 1;
}

/** Apply braille points filter to status cells
    ARGS:
    s: a table of unsigned char corresponding to braille dots
    n: number of char to display */
BRAILLE_API char
braille_statusfilter(unsigned char s, int n)
{
  if(n > term.status_width)
    {
      brli_seterror("Invalid position");
      return 0;
    }
  term.status[n] |= s;
  return 1;
}

/** Display everything when using braille_write or braille_filter */
BRAILLE_API char
braille_statusrender()
{
  return brli_drvstatus(&term);
}

/** Returns the size of the braille status cells
    RETS:
    size of braille display */
BRAILLE_API int
braille_statussize()
{
  return term.status_width;
}

/** Change the current character translation table
    ARGS:
    tablefile: a string indicating the table file */
BRAILLE_API char
braille_usetable(const char *tablefile)
{
  unsigned int i;
  
  if(tablefile == NULL)
    {
      brli_log(LOG_INFO, "Dot Translation Table: built-in");
    }
  else
    {
      FILE* tbl_fd; /* Translation table filedescriptor */
      unsigned char *loadtbl = NULL;
      
      if(!(tbl_fd = fopen(tablefile, "rb")))
	{
	  brli_seterror("Cannot read dot translation table '%s': using built-in", tablefile);
	  return 0;
	}
      else
	{
	  if(!(loadtbl = (unsigned char *)malloc(256)))
	    {
	      brli_seterror( "Cannot allocate translation table memory");
	      return 0;
	    }
	  else
	    {
	      size_t count = fread(loadtbl, 1, 256, tbl_fd);
	      if(count < 256)
		{
		  brli_seterror("Bad format for tanslation table file %s length = %d", tablefile, count);
		  return 0;
		}
	      else
		{
		  memcpy(brailletrans, loadtbl, 256);
		  brli_log(LOG_INFO, "Dot Translation Table: %s", tablefile);
		}
	      
	      if(loadtbl)
		{
		  free(loadtbl);
		  loadtbl = NULL;
		}
	    }

	  if(tbl_fd)
	    {
	      fclose(tbl_fd);
	    }
	}
    }

  for(i = 0; i < 256; i++)
    {
      asciitrans[(unsigned char)brailletrans[i]] = (unsigned char)i;
    }
  return 1;
}

/** Translate from ascii to braille representation */
BRAILLE_API unsigned char
braille_ascii2braille(unsigned char ascii)
{
  return brailletrans[ascii];
}

/** Translate from braille to ascii representation */
BRAILLE_API unsigned char
braille_braille2ascii(unsigned char braille)
{
  return asciitrans[braille];
}

BRAILLE_API const char *
braille_geterror()
{
  return brli_geterror();
}

BRAILLE_API char
braille_timeout(int time)
{
  if(time < 0) /* blocking until some data arrives */
    {
      term.cc_min = 1;
      term.cc_time = 0;

      /* Waits 10s - braille_read loops in blocking mode while there is no data */ 
      term.timeout = 10000;
    }
  else /* waiting at most 'time' milliseconds before returning */
    {
      term.timeout = time;
      term.cc_min = 0;
      if(time > 25500)
	{
	  term.cc_time = 255;
	}
      else
	{
	  term.cc_time = (unsigned char)(time / 100);
	}
    }
  return 1;
}

/* ------------------------------------------------------------ */
/** Load driver from library
    RETS:
    true (nonzero) on success */
static char
brli_load_driver(const char *driver, const char *path)
{
  if(!driver)
    {
      brli_seterror("Error no driver specified");
      return 0;
    }

  if(lt_dladdsearchdir(path))
    {
      brli_seterror("Cannot change search path : %s", lt_dlerror());
      return 0;
    }

  library = lt_dlopenext(driver);
  if(!library)
    {
      brli_seterror("Cannot open braille driver library %s : %s", driver, lt_dlerror());
      return 0;
    }

  brli_drvinfo = (const char *(*)(brli_term *, brl_config))lt_dlsym(library, "brli_drvinfo"); /* locate function from the shared library */
  if(!brli_drvinfo)
    {
      brli_seterror("Driver symbol not found brli_drvinfo : %s", lt_dlerror());
      return 0;
    }

  brli_drvinit = (char (*)(brli_term *, char, const char *))lt_dlsym(library, "brli_drvinit");
  if(!brli_drvinit)
    {
      brli_seterror("Driver symbol not found brli_drvinit : %s", lt_dlerror());
      return 0;
    }

  brli_drvclose = (char (*)(brli_term *))lt_dlsym(library, "brli_drvclose");
  if(!brli_drvclose)
    {
      brli_seterror("Driver symbol not found brli_drvclose : %s", lt_dlerror());
      return 0;
    }

  brli_drvwrite = (char (*)(brli_term *))lt_dlsym(library, "brli_drvwrite");
  if(!brli_drvwrite)
    {
      brli_seterror("Driver symbol not found brli_drvwrite : %s", lt_dlerror());
      return 0;
    }

  brli_drvread = (char (*)(brli_term *, brl_key *))lt_dlsym(library, "brli_drvread");
  if(!brli_drvread)
    {
      brli_seterror("Driver symbol not found brli_drvread : %s", lt_dlerror());
      return 0;
    }

  brli_drvstatus = (char (*)(brli_term *))lt_dlsym(library, "brli_drvstatus");
  if(!brli_drvstatus)
    {
      brli_seterror("Driver symbol not found brli_drvstatus : %s", lt_dlerror());
      return 0;
    }

  return 1;
}

/* Initialise Braille and set text display */
static char
brli_init_driver(char type)
{
#ifdef HAVE_TERMIOS_H
  term.fd = -1;
#else
#ifdef WIN32
  term.handle = INVALID_HANDLE_VALUE;
#else
#error Need termios or win32
#endif
#endif

#ifdef HAVE_USB_H
  term.usbhandle = NULL;
  term.interface_claimed = 0;
#endif

  term.cc_min = 1;     /* By default brli_drvread functions are
			  blocking until some data arrives */
  term.cc_time = 0;
  term.width = 0;
  term.status_width = 0;
  term.display = term.display_ascii = NULL;
  term.status = term.status_ascii = NULL;
  term.name = NULL;
  if(!brli_drvinit(&term, type, device) || !term.width
     || !term.display || !term.display_ascii)
    {
      brli_drvclose(&term);
      brli_seterror("Braille driver initialization failed: %s", braille_geterror());
      return 0;
    }
  return 1;
}
