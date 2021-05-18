/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#ifndef _BRL_CONFIG_H
#define _BRL_CONFIG_H 1

#include "config.h"
#include "brl_win32.h"
#include "braille.h"

extern char *device, *driver, *table, *path, *pathcfg, *pathdrv, *pathtbl;
extern void brli_process_configuration_file(char *path);
extern int brli_get_configuration(void);

#ifndef WIN32
#define BRLDEV "/dev/ttyS0"   /* Default device if not specified in conf file */
#else
#define BRLDEV "COM1"         /* Default device if not specified in conf file */
#endif

typedef struct
{
  const char *name;
  char autodetect;
  char type;
  const char *models;
} brl_driver;

typedef struct
{
  const char *name;
  const char *description;
} brl_table;

#endif
