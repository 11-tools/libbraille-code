/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#ifndef _DRIVER_H
#define _DRIVER_H 1

#include "config.h"

/* Make sure the correct platform symbols are defined */
#if !defined(WIN32) && defined(_WIN32)
#define WIN32
#endif /* Windows */

/* Make sure the correct debug symbols are defined */
#if !defined(DEBUG) && defined(_DEBUG)
#define DEBUG
#endif

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DRVNONE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DRVNONE_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
#ifdef BRLDRV_EXPORTS
#define BRLDRV_API __declspec(dllexport)
#else
#define BRLDRV_API __declspec(dllimport)
#endif
#else
#define BRLDRV_API
#endif

#include "braille.h"
#include "brl_error.h"
#include "brl_term.h"

/* Routines provided by this braille display driver. */
/* returns the name of the driver */
BRLDRV_API const char *brli_drvinfo(brli_term *, brl_config code);
/* initialise the Braille terminal */
BRLDRV_API char brli_drvinit(brli_term *, char type, const char *);
/* close the driver */
BRLDRV_API char brli_drvclose(brli_term *);
/* write to Braille terminal */
BRLDRV_API char brli_drvwrite(brli_term *);
/* get key pressed on the Braille terminal */
BRLDRV_API signed char brli_drvread(brli_term *, brl_key *);
/* set status cells */
BRLDRV_API char brli_drvstatus(brli_term *);
#endif
