/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#include <config.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "braille.h"
#include "brl_error.h"

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

#define ERRSTR_SIZE 256
static char read_error = 0;
static char *brli_strerror = NULL;
static unsigned char debug_level = 3;

BRAILLE_API void
braille_debug(unsigned char level)
{
  debug_level = level;
}

BRAILLE_API void
brli_seterror(const char *fmt, ...)
{
  va_list argp;
  char *strerror;

  va_start(argp, fmt);
  strerror = malloc(ERRSTR_SIZE);
  if(strerror == NULL)
    return;
  vsnprintf(strerror, ERRSTR_SIZE, fmt, argp);
  if(brli_strerror && !read_error)
    {
      brli_log(LOG_WARNING, "Warning: setting new error while old not read: %s", brli_strerror);
    }
  if(brli_strerror)
    free(brli_strerror);
  brli_strerror = strerror;
  va_end(argp);

  brli_log(LOG_ERR, "%s", brli_strerror);
}

BRAILLE_API const char*
brli_geterror()
{
  read_error = 1;
  return brli_strerror;
}

BRAILLE_API void
brli_log(int prio, char *fmt, ...)
{
  va_list argp;

  if(debug_level > prio)
    {
      va_start(argp, fmt);
      vfprintf(stderr, fmt, argp);
      fprintf(stderr,"\n");
      va_end(argp);
    }
}
