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

// #ifdef HAVE_UNISTD_H
// #include <unistd.h>
// #endif

#ifdef WIN32
#include <windows.h>
#endif

brl_table table_list[] =
  {
    {"french-win.tbl", "French table"}
  };

/* Returns the number of available tables */
BRAILLE_API int
braille_tablenum(void)
{
  return sizeof(table_list)/sizeof(brl_table);
}

/* Returns the name of the table for which the number is provided */
BRAILLE_API const char *
braille_tablename(int num)
{
  if(num < 0 || num > sizeof(table_list)/sizeof(brl_table))
    return NULL;
  
  return table_list[num].name;
}

/* Returns the description of the table for which the number is provided */
BRAILLE_API const char *
braille_tabledesc(int num)
{
  if(num < 0 || num > sizeof(table_list)/sizeof(brl_table))
    return NULL;
  
  return table_list[num].description;
}

int
brli_get_configuration(void)
{
#define BUFSIZE 80

  /* Read user preferences */

  if(!driver || !table || !device)
    {
      HKEY hKey;
      DWORD dwBufLen = BUFSIZE;
      LONG lRet;

      lRet = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\libbraille\\" PACKAGE_VERSION,
			  0, KEY_QUERY_VALUE, &hKey);
      if(lRet == ERROR_SUCCESS)
	{
	  if(!driver)
	    {
	      char *szDriver;

	      lRet = ERROR_MORE_DATA;
	      szDriver = (char *)malloc(BUFSIZE);
	      while(lRet == ERROR_MORE_DATA)
		{
		  lRet = RegQueryValueEx(hKey, "Driver", NULL, NULL, (LPBYTE)szDriver,
					 &dwBufLen);
		  if(lRet == ERROR_SUCCESS && dwBufLen)
		    {
		      driver = (char *)malloc(dwBufLen);
		      if(!driver)
			{
			  brli_seterror("Error allocating memory");
			  return 0;
			}		
		      strncpy(driver, szDriver, dwBufLen);
		      brli_log(LOG_INFO, "Driver in user registry: %s", driver);
		    }
		  else if(lRet == ERROR_MORE_DATA)
		    {
		      szDriver = (char *)realloc(szDriver, dwBufLen);
		    }
		}
	      if(szDriver)
		free(szDriver);
	    }

	  if(!table)
	    {
	      char *szTable;

	      lRet = ERROR_MORE_DATA;
	      szTable = (char *)malloc(BUFSIZE);
	      while(lRet == ERROR_MORE_DATA)
		{
		  lRet = RegQueryValueEx(hKey, "Table", NULL, NULL, (LPBYTE)szTable,
					 &dwBufLen);
		  if(lRet == ERROR_SUCCESS && dwBufLen)
		    {
		      table = (char *)malloc(dwBufLen);
		      if(!table)
			{
			  brli_seterror("Error allocating memory");
			  return 0;
			}
		      strncpy(table, szTable, dwBufLen);
		      brli_log(LOG_INFO, "Table in user registry: %s", table);
		    }
		  else if(lRet == ERROR_MORE_DATA)
		    {
		      szTable = (char *)realloc(szTable, dwBufLen);
		    }
		}
	      if(szTable)
		free(szTable);
	    }

	  if(!device)
	    {
	      char *szDevice;

	      lRet = ERROR_MORE_DATA;
	      szDevice = (char *)malloc(BUFSIZE);
	      while(lRet == ERROR_MORE_DATA)
		{
		  lRet = RegQueryValueEx(hKey, "Device", NULL, NULL, (LPBYTE)szDevice,
					 &dwBufLen);
		  if(lRet == ERROR_SUCCESS && dwBufLen)
		    {
		      device = (char *)malloc(dwBufLen);
		      if(!device)
			{
			  brli_seterror("Error allocating memory");
			  return 0;
			}
		      strncpy(device, szDevice, dwBufLen);
		      brli_log(LOG_INFO, "Device in user registry: %s", device);
		    }
		  else if(lRet == ERROR_MORE_DATA)
		    {
		      szDevice = (char *)realloc((void *)szDevice, dwBufLen);
		    }
		}
	      if(szDevice)
		free(szDevice);
	    }

	  RegCloseKey(hKey);
	}
    }
  
  /* Read system preferences */
	
  if(!path || !pathdrv || !pathtbl || !device || !driver || !table)
    {
      HKEY hKey;
      DWORD dwBufLen = BUFSIZE;
      LONG lRet;
		
      lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\libbraille\\" VERSION,
			  0, KEY_QUERY_VALUE, &hKey);
      if(lRet != ERROR_SUCCESS)
	{
	  brli_seterror("Could not open libbraille machine key in the registry");
	  return 0;
	}
      lRet = ERROR_MORE_DATA;

      if(!path)
	{
	  char *szPath;

	  szPath = (char *)malloc(BUFSIZE);
	  while(lRet == ERROR_MORE_DATA)
	    {
	      lRet = RegQueryValueEx(hKey, "InstallLocation", NULL, NULL, (LPBYTE) szPath, &dwBufLen);
	      if(lRet == ERROR_MORE_DATA)
		{
		  szPath = (char *)realloc(szPath, dwBufLen);
		}
	      else if(lRet != ERROR_SUCCESS)
		{
		  brli_seterror("Could not retrieve InstallLocation subkey in the registry");
		  return 0;
		}
	      else if(dwBufLen)
		{
		  path = (char *)malloc(dwBufLen);
		  if(!path)
		    {
		      brli_seterror("Error allocating memory");
		      return 0;
		    }
		  strncpy(path, szPath, dwBufLen);
		  brli_log(LOG_INFO, "InstallLocation in machine registry: %s", path);
		}
	      else
		{
		  brli_seterror("Empty InstallLocation subkey in the registry");
		  return 0;
		}
	    }
	}

      if(!pathdrv)
	{	
	  /* TODO: use glib functions concerning paths to make this */
			
	  pathdrv = (char *)malloc(strlen(path) + strlen("\\drivers") + 1);
	  if(!pathdrv)
	    {
	      brli_seterror("Error allocating memory");
	      return 0;
	    }
	  strncpy(pathdrv, path, strlen(path));
	  if(pathdrv[strlen(path) - 1] == 0x5C) // Ascii code for \ - TODO: use glib
	    strncpy(pathdrv + strlen(path), "drivers", strlen("drivers") + 1);
	  else
	    strncpy(pathdrv + strlen(path), "\\drivers", strlen("\\drivers") + 1);
	}
		
      if(!pathtbl)
	{	
	  /* TODO: use glib functions concerning paths to make this */
			
	  pathtbl = (char *)malloc(strlen(path) + strlen("\\tables") + 1);
	  if(!pathtbl)
	    {
	      brli_seterror("Error allocating memory");
	      return 0;
	    }
	  strncpy(pathtbl, path, strlen(path));
	  if(pathdrv[strlen(path) - 1] == 0x5C) // Ascii code for \ - TODO: use glib
	    {
	      strncpy(pathtbl + strlen(path), "tables", strlen("tables") + 1);
	    }
	  else
	    {
	      strncpy(pathtbl + strlen(path), "\\tables", strlen("\\tables") + 1);
	    }
	}
		
      if(!driver)
	{
	  char *szDriver;

	  lRet = ERROR_MORE_DATA;
	  szDriver = (char *)malloc(BUFSIZE);
	  while(lRet == ERROR_MORE_DATA)
	    {
	      lRet = RegQueryValueEx(hKey, "Driver", NULL, NULL, (LPBYTE) szDriver, &dwBufLen);
	      if(lRet == ERROR_SUCCESS && dwBufLen)
		{
		  driver = (char *)malloc(dwBufLen);
		  if(!driver)
		    {
		      brli_seterror("Error allocating memory");
		      return 0;
		    }				
		  strncpy(driver, szDriver, dwBufLen);
		  brli_log(LOG_INFO, "Driver in machine registry: %s", driver);
		}
	      else if(lRet == ERROR_MORE_DATA)
		{
		  szDriver = (char *)realloc(szDriver, dwBufLen);
		}
	    }
	  if(szDriver)
	    free(szDriver);
	}

      if(!table)
	{
	  char *szTable;

	  lRet = ERROR_MORE_DATA;
	  szTable = (char *)malloc(BUFSIZE);
	  while(lRet == ERROR_MORE_DATA)
	    {
	      lRet = RegQueryValueEx(hKey, "Table", NULL, NULL, (LPBYTE) szTable, &dwBufLen);
	      if(lRet == ERROR_SUCCESS && dwBufLen)
		{
		  table = (char *)malloc(dwBufLen);
		  if(!table)
		    {
		      brli_seterror("Error allocating memory");
		      return 0;
		    }				
		  strncpy(table, szTable, dwBufLen);
		  brli_log(LOG_INFO, "Table in machine registry: %s", table);
		}
	      else if(lRet == ERROR_MORE_DATA)
		{
		  szTable = (char *)realloc(szTable, dwBufLen);
		}
	    }
	  if(szTable)
	    free(szTable);
	}

      if(!device)
	{
	  char *szDevice;

	  lRet = ERROR_MORE_DATA;
	  szDevice = (char *)malloc(BUFSIZE);
	  while(lRet == ERROR_MORE_DATA)
	    {
	      lRet = RegQueryValueEx(hKey, "Device", NULL, NULL, (LPBYTE) szDevice, &dwBufLen);
	      if(lRet == ERROR_SUCCESS && dwBufLen)
		{
		  device = (char *)malloc(dwBufLen);
		  if(!device)
		    {
		      brli_seterror("Error allocating memory");
		      return 0;
		    }				
		  strncpy(device, szDevice, dwBufLen);
		  brli_log(LOG_INFO, "Device in machine registry: %s", device);
		}
	      else if(lRet == ERROR_MORE_DATA)
		{
		  szDevice = (char *)realloc(szDevice, dwBufLen);
		}
	    }
	  if(szDevice)
	    free(szDevice);
	}
      RegCloseKey( hKey );
    }

  if(!device)
    device = BRLDEV;
  if(!driver)
    {
      driver = (char *)malloc(strlen("auto") + 1);
      if(driver)
	strcpy(driver, "auto");
    }

  return 1;
}
