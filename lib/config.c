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
#include "brl_error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

extern brl_table table_list[];

static brl_driver driver_list[] =
  {
    {"auto",          0, BRL_TYPE_SPECIAL,
     "Autodetect the type of terminal"},
    {"alva",          1, BRL_TYPE_SERIAL,
     "Alva ABT320, ABT340, ABT340 Desktop, ABT380, ABT380 Twin Space, "
     "Delphi 420, 440, 440 Desktop, 480, Satellite 544, 570 Pro, 584 Pro and 544 Traveller"},
    {"alvausb",       1, BRL_TYPE_USB,
     "Alva Satellite 544, 570 Pro, 584 Pro and 544 Traveller"},
    {"baum",          1, BRL_TYPE_USB | BRL_TYPE_SERIAL,
     "Baum Pocket and Super Vario 24, 32, 40, 64 and 80"},
    {"braillelite",   1, BRL_TYPE_SERIAL,
     "BrailleLite 18, 40, Millenium 20 and Millenium 40"},
    {"braillenote",   1, BRL_TYPE_SERIAL,
     "Pulse Data BrailleNote 18 and 32"},
    {"eurobraille",   1, BRL_TYPE_SERIAL,
     "NoteBRAILLE, Clio-NoteBRAILLE, SCRIBA, AzerBRAILLE, CLIO-euroBraille and Iris"},
    {"fake",          0, BRL_TYPE_SPECIAL,
     "Fake"},
    {"handytech",     1, BRL_TYPE_SERIAL,
     "HandyTech 40, 80, Braille Wave, Baum Vario 40 via emulation"},
    {"none",          0, BRL_TYPE_SPECIAL, "None"},
    {"once",          1, BRL_TYPE_SERIAL,
     "ONCE Eco 20, 40 and 80"},
    {"papenmeierusb", 1, BRL_TYPE_USB, "Papenmeier Braillex EL 40S"},
    {"technibraille", 1, BRL_TYPE_SERIAL, "Manager 40i and 40s"},
    {"text",          0, BRL_TYPE_SPECIAL, "Text"},
    {"voyager",       1, BRL_TYPE_USB, "Tieman Voyager 44, 44+, 70 and 70+"},
  };

/* Returns the number of available drivers */
BRAILLE_API int
braille_drivernum(void)
{
  return sizeof(driver_list)/sizeof(brl_driver);
}

/* Returns the name of the driver for which the number is provided */
BRAILLE_API const char *
braille_drivername(int num)
{
  if(num < 0 || num > sizeof(driver_list)/sizeof(brl_driver))
    return NULL;

  return driver_list[num].name;
}

/* Returns the type of the driver (usb or serial) for which the number
   is provided */
BRAILLE_API brl_drivertype
braille_drivertype(int num)
{
  if(num < 0 || num > sizeof(driver_list)/sizeof(brl_driver))
    return 0;

  return driver_list[num].type;
}

/* Returns a string describing models supported by the driver for
   which the number is provided */
BRAILLE_API const char *
braille_drivermodels(int num)
{
  if(num < 0 || num > sizeof(driver_list)/sizeof(brl_driver))
    return NULL;

  return driver_list[num].models;
}

/* Returns whether a driver can autodetect its terminal */
BRAILLE_API char
braille_driverauto(int num)
{
  if(num < 0 || num > sizeof(driver_list)/sizeof(brl_driver))
    return 0;
  
  return driver_list[num].autodetect;
}

/* Returns whether a driver supports a specific connection type */
BRAILLE_API char
braille_driver_is_type(int num, char type)
{
  if(num < 0 || num > sizeof(driver_list)/sizeof(brl_driver))
    return 0;
  
  return (driver_list[num].type & type ? 1 : 0);
}

/* Configure some values */
BRAILLE_API char
braille_config(brl_config code, char *value)
{
  size_t len;

  switch(code)
    {
    case BRL_DEVICE:
      if(device)
	{
	  free(device);
	}
      if(value == NULL)
	{
	  device = NULL;
	  break;
	}
      len = strlen(value);
      device = (char *)malloc(len + 1);
      if(!device)
	{
	  brli_seterror("Error allocating memory:%s", strerror(errno));
	  return 0;
	}
      strncpy(device, value, len + 1);
      break;
    case BRL_DRIVER:
      if(driver)
	{
	  free(driver);
	}
      if(value == NULL)
	{
	  driver = NULL;
	  break;
	}
      len = strlen(value);
      driver = (char *)malloc(len + 1);
      if(!driver)
	{
	  brli_seterror("Error allocating memory:%s", strerror(errno));
	  return 0;
	}
      strncpy(driver, value, len + 1);
      break;
    case BRL_TABLE:
      if(table)
	{
	  free(table);
	}
      if(value == NULL)
	{
	  table = NULL;
	  break;
	}
      len = strlen(value);
      table = (char *)malloc(len + 1);
      if(!table)
	{
	  brli_seterror("Error allocating memory:%s", strerror(errno));
	  return 0;
	}
      strncpy(table, value, len + 1);
      break;
    case BRL_PATH:
#ifdef WIN32
      if(path)
	{
	  free(path);
	}
      if(value == NULL)
	{
	  path = NULL;
	  break;
	}
      len = strlen(value);
      path = (char *)malloc(len + 1);
      if(!path)
	{
	  brli_seterror("Error allocating memory:%s", strerror(errno));
	  return 0;
	}
      strncpy(path, value, len + 1);
      break;
#else
      break;
#endif
#ifndef WIN32
    case BRL_PATHCONF:
      if(pathcfg)
	{
	  free(pathcfg);
	}
      if(value == NULL)
	{
	  pathcfg = NULL;
	  break;
	}
      len = strlen(value);
      pathcfg = (char *)malloc(len + 1);
      if(!pathcfg)
	{
	  brli_seterror("Error allocating memory:%s", strerror(errno));
	  return 0;
	}
      strncpy(pathcfg, value, len + 1);
      break;
#endif
    case BRL_PATHDRV:
      if(pathdrv)
	{
	  free(pathdrv);
	}
      if(value == NULL)
	{
	  pathdrv = NULL;
	  break;
	}
      len = strlen(value);
      pathdrv = (char *)malloc(len + 1);
      if(!pathdrv)
	{
	  brli_seterror("Error allocating memory:%s", strerror(errno));
	  return 0;
	}
      strncpy(pathdrv, value, len + 1);
      break;
    case BRL_PATHTBL:
      if(pathtbl)
	{
	  free(pathtbl);
	}
      if(value == NULL)
	{
	  pathtbl = NULL;
	  break;
	}
      len = strlen(value);
      pathtbl = (char *)malloc(len + 1);
      if(!pathtbl)
	{
	  brli_seterror("Error allocating memory:%s", strerror(errno));
	  return 0;
	}
      strncpy(pathtbl, value, len + 1);
      break;
    default:
      brli_seterror("Unrecognised configuration code");
      return 0;
    }
  return 1;
}
