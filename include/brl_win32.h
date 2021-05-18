/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

#ifndef _BRL_WIN32_H
#define _BRL_WIN32_H 1

/* Make sure the correct platform symbols are defined */
#if !defined(WIN32) && defined(_WIN32)
#define WIN32
#endif /* Windows */

/* Make sure the correct debug symbols are defined */
#if !defined(DEBUG) && defined(_DEBUG)
#define DEBUG
#endif

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

#endif
