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

brl_table table_list[] =
  {
    {"american.tbl", "American table"},
    {"british.tbl", "British table"},
    {"french.tbl", "French table"},
    {"german.tbl", "German table"},
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

/* Define error codes for configuration file processing. */
#define CF_OK 0		/* No error. */
#define CF_NOVAL 1	/* Operand not specified. */
#define CF_INVAL 2	/* Bad operand specified. */
#define CF_EXTRA 3	/* Too many operands. */

/* Local prototypes */

static void brli_process_lines(FILE *file, void (*handler) (char *line, void *data), void *data);
static void brli_process_configuration_line(char *line, void *data);

static char
brli_get_token(char **val, const char *delims)
{
  char *v;
  if(*val != NULL)
    return CF_OK;

  v = strtok(NULL, delims);
  if(v == NULL)
    {
      return CF_NOVAL;
    }
  if(strtok(NULL, delims) != NULL)
    {
      return CF_EXTRA;
    }
  *val = strdup(v);
  return CF_OK;
}

static char
brli_set_device(const char *delims)
{
  return brli_get_token(&device, delims);
}

static char
brli_set_driver(const char *delims)
{
  return brli_get_token(&driver, delims);
}

static char
brli_set_table(const char *delims)
{
  return brli_get_token(&table, delims);
}

int
brli_get_configuration(void)
{
  if((!device || !driver || !table) && pathcfg)
    {
      brli_log(LOG_INFO, "Looking for conf in %s", pathcfg);

      if(chdir(pathcfg) == 0)
	{
	  brli_process_configuration_file("libbraille.conf");
	}
    }
  
  if(!device || !driver || !table)
    {
      brli_log(LOG_INFO, "Looking for conf in " SYSCONFDIR);
      
      if(!pathcfg)
	{
	  pathcfg = (char *)malloc(strlen(SYSCONFDIR) + 1);
	  if(!pathcfg)
	    {
	      brli_seterror("Error allocating memory: %s", strerror(errno));
	      return 0;
	    }
	  strcpy(pathcfg, SYSCONFDIR);
	}

      /* Process the default configuration file. */
      if(chdir(SYSCONFDIR) == 0)
	{
	  brli_process_configuration_file("libbraille.conf");
	}
    }

  if(!path)
    {
      path = malloc(strlen(PREFIX) + 1);
      if(!path)
	{
	  brli_seterror("Error allocating memory: %s", strerror(errno));
	  return 0;
	}
      strcpy(path, PREFIX);
      brli_log(LOG_INFO, "Path of installation: %s", path);
    }

  if(!pathtbl)
    {
      pathtbl = malloc(strlen(PKGDATADIR) + 1);
      if(!pathtbl)
	{
	  brli_seterror("Error allocating memory: %s", strerror(errno));
	  return 0;
	}
      strcpy(pathtbl, PKGDATADIR);
      brli_log(LOG_INFO, "Path of tables: %s", pathtbl);
    }

  if(!pathdrv)
    {
      pathdrv = malloc(strlen(PKGLIBDIR) + 1);
      if(!pathdrv)
	{
	  brli_seterror("Error allocating memory: %s", strerror(errno));
	  return 0;
	}
      strcpy(pathdrv, PKGLIBDIR);
      brli_log(LOG_INFO, "Path of drivers: %s", pathdrv);
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


static void
brli_process_configuration_line(char *line, void *data)
{
  const char *word_delims = " \t"; /* Characters which separate words */
  char *keyword; /* Points to first word of each line */

  /* Relate each keyword to its handler via a table */
  typedef struct
  {
    const char *name;
    char (*handler)(const char *delims); 
  } keyword_entry;
  const keyword_entry keyword_table[] =
    {
      {"device", brli_set_device},
      {"driver", brli_set_driver},
      {"table", brli_set_table},
      {NULL, NULL}
    };

  /* Remove comment from end of line */
  {
    char *comment = strchr(line, '#');
    if (comment != NULL)
      *comment = '\0';
  }
  
  keyword = strtok(line, word_delims);
  if(keyword != NULL) /* Ignore blank lines */
    {
      const keyword_entry *kw = keyword_table;
      while(kw->name != NULL)
        {
	  if(strcasecmp(keyword, kw->name) == 0)
	    {
	      int code = kw->handler(word_delims);
	      switch(code)
	        {
		case CF_OK:
		  break;
		case CF_NOVAL:
		  brli_log(LOG_ERR, "Operand not supplied for configuration item '%s'.",
			   keyword);
		  break;
		case CF_INVAL:
		  brli_log(LOG_ERR,
			   "Invalid operand specified"
			   " for configuration item '%s'.",
			   keyword);
		  break;
		case CF_EXTRA:
		  brli_log(LOG_ERR,
			   "Too many operands supplied for configuration item '%s'.",
			   keyword);
		  break;
		default:
		  brli_log(LOG_ERR,
			   "Internal error: unsupported"
			   " configuration file error code: %d",
			   code);
		  break;
		}
	      return;
	    }
	  ++kw;
	}
      brli_log(LOG_ERR, "Unknown configuration item: '%s'.", keyword);
    }
}

void
brli_process_configuration_file(char *path)
{
  FILE *file = fopen(path, "r");
  if(file != NULL)
    {
      brli_process_lines(file, brli_process_configuration_line, NULL);
      fclose(file);
    }
  else if(errno != ENOENT)
    {
      brli_seterror("Cannot open configuration file '%s': %s",
		    path, strerror(errno));
    }
}

/** Process each line of an input text file safely.
    This routine handles the actual reading of the file, insuring that the
    input buffer is always big enough, and calls a caller-supplied handler
    once for each line in the file.  The caller-supplied data pointer is
    passed straight through to the handler.
    ARGS:
    file: the input file
    handler: the input line handler
    data: pointer to caller-specific data */
static void
brli_process_lines(FILE *file, void (*handler)(char *line, void *data), void *data)
{
  size_t buff_size = 0X80; /* Initial buffer size */
  char *buff_addr = malloc(buff_size); /* Allocate the buffer */
  char *line; /* Will point to each line that is read */

  /* Keep looping, once per line, until end-of-file */
  while((line = fgets(buff_addr, buff_size, file)) != NULL)
    {
      size_t line_len = strlen(line); /* Line length including new-line */

      /* No trailing new-line means that the buffer isn't big enough */
      while(line[line_len - 1] != '\n')
        {
          /* Extend the buffer, keeping track of its new size */
          buff_addr = realloc(buff_addr, (buff_size <<= 1));

          /* Read the rest of the line into the end of the buffer */
          line = fgets(buff_addr + line_len, buff_size - line_len, file);

          line_len += strlen(line); /* New total line length */
          line = buff_addr; /* Point to the beginning of the line */
        }
      line[line_len -= 1] = '\0'; /* Remove trailing new-line */

      handler(line, data);
    }

  /* Deallocate the buffer */
  free(buff_addr);
}
