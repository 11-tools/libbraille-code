/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#ifndef _BRL_ERROR_H
#define _BRL_ERROR_H 1

#include "config.h"
#include "brl_win32.h"

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#include <stdarg.h>
#else
#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7
#endif

extern BRAILLE_API void brli_log(int prio, char *fmt, ...);
extern BRAILLE_API const char *brli_geterror();
extern BRAILLE_API void brli_seterror(const char *fmt, ...);

#endif
