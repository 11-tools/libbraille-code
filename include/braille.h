/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#ifndef _BRAILLE_H
#define _BRAILLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Make sure the correct platform symbols are defined */
#if !defined(WIN32) && defined(_WIN32)
#define WIN32
#endif /* Windows */

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LIBBRAILLE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LIBBRAILLE_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
#ifdef LIBBRAILLE_EXPORTS
#define BRAILLE_API __declspec(dllexport)
#else
#define BRAILLE_API __declspec(dllimport)
#endif
#else
#define BRAILLE_API
#endif

/* A macro that returns the right binary value to activate the given dots */
#define BRAILLE(a, b, c, d, e, f, g, h)  (a + b * 2 + c * 4 + d * 8 + e * 16 + f * 32 + g * 64 + h * 128)

#include "brl_keycode.h"

typedef enum {
  BRL_NONE   = 0,
  BRL_KEY    = 1,
  BRL_CURSOR = 2,
  BRL_CMD    = 3,
  BRL_ACC    = 4
} brl_keytype;

typedef struct
{
  brl_keytype type;
  unsigned char braille; /* corresponding braille keys */
  brl_keycode code;
  unsigned short unicode;
} brl_key;

typedef enum {
  BRL_DEVICE,
  BRL_DRIVER,
  BRL_TERMINAL,
  BRL_TABLE,
  BRL_PATH,
  BRL_VERSION,
  BRL_PATHDRV,
  BRL_PATHTBL,
#ifndef WIN32
  BRL_PATHCONF
#endif
} brl_config;

typedef enum {
  BRL_TYPE_UNKNOWN = 0,
  BRL_TYPE_SPECIAL = 1,
  BRL_TYPE_SERIAL  = 2,
  BRL_TYPE_USB     = 4
} brl_drivertype;

  /**** Configuration related functions ****/

/* Configure some values */
extern BRAILLE_API char braille_config(brl_config, char *);
/* Returns the number of available drivers */
extern BRAILLE_API int braille_drivernum(void);
/* Returns the name of the driver for which the number is provided */
extern BRAILLE_API const char *braille_drivername(int num);
/* Returns the type of the driver (usb or serial) for which the number
   is provided */
extern BRAILLE_API brl_drivertype braille_drivertype(int num);
/* Returns a string describing models supported by the driver for
which the number is provided */
extern BRAILLE_API const char *braille_drivermodels(int num);
/* Returns whether a driver can autodetect its terminal */
extern BRAILLE_API char braille_driverauto(int num);
/* Returns whether a driver supports a specific connection type */
extern BRAILLE_API char braille_driver_is_type(int num, char type);
/* Returns the number of available tables */
extern BRAILLE_API int braille_tablenum(void);
/* Returns the name of the table for which the number is provided */
extern BRAILLE_API const char *braille_tablename(int num);
/* Returns the description of the table for which the number is provided */
extern BRAILLE_API const char *braille_tabledesc(int num);


  /**** Usage related functions ****/

/* Initialize the library */
extern BRAILLE_API char braille_init();
/* Stop the library and the braille display */
extern BRAILLE_API char braille_close(void);
/* Info about the running terminal */
extern BRAILLE_API const char * braille_info(brl_config code);
/* Returns the size of the braille display */
extern BRAILLE_API int braille_size();
/* Display a simple string (terminated by \0) on the braille display */
extern BRAILLE_API char braille_display(const char *s);
/* Write characters on the braille display */
extern BRAILLE_API char braille_write(const char *s, int n);
/* Apply braille points filter to what is to be displayed */
extern BRAILLE_API char braille_filter(unsigned char s, int n);
/* Returns the size of the braille status cells */
extern BRAILLE_API int braille_statussize();
/* Display a simple string (terminated by \0) on the braille status cells */
extern BRAILLE_API char braille_statusdisplay(const char *s);
/* Write characters on the braille status cells */
extern BRAILLE_API char braille_statuswrite(const char *s, int n);
/* Apply braille points filter to the status cells */
extern BRAILLE_API char braille_statusfilter(unsigned char s, int n);
/* Display everything when using braille_status_write or braille_status_filter */
extern BRAILLE_API char braille_statusrender();
/* Function to specify how much time to wait for a key press */
extern BRAILLE_API char braille_timeout(int time);
/* Get a key from the device */
extern BRAILLE_API signed char braille_read(brl_key *key);
/* Display everything when using braille_write or braille_filter */
extern BRAILLE_API char braille_render();
/* Change the current character translation table */
extern BRAILLE_API char braille_usetable(const char *tablefile);
/* Translate from ascii to braille representation */
extern BRAILLE_API unsigned char braille_ascii2braille(unsigned char ascii);
/* Translate from braille to ascii representation */
extern BRAILLE_API unsigned char braille_braille2ascii(unsigned char braille);

/* Configure some values */
extern BRAILLE_API void braille_debug(unsigned char);
								
extern BRAILLE_API const char * braille_geterror();

#ifdef __cplusplus
}
#endif

#endif
