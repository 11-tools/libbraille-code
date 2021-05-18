/*
 * test_libbraille.c - Test program and simple example
 *
 * See doc/TUTORIAL for an explanation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "braille.h"

#define MAXLEN 255
#ifdef WIN32
#define snprintf _snprintf
#endif


int 
main(int argc, char *argv[])
{
  int i;
  char message[MAXLEN];

  printf("Starting test_libbraille\n");

  braille_debug(8);

  for(i = 0; i < braille_drivernum(); i++)
    {
      printf("driver [%d]: %s, supporting models: %s", i, braille_drivername(i),
	     braille_drivermodels(i));
      printf("\n");
    }

  for(i = 0; i < braille_tablenum(); i++)
    {
      printf("table [%d]: %s, corresponding to: %s", i, braille_tablename(i),
	     braille_tabledesc(i));
      printf("\n");
    }

  /* uncomment to config some values without using the configuration
     file */
  /* braille_config(BRL_DEVICE, "COM1"); */
  /* braille_config(BRL_DRIVER, "eco"); */
  /* or braille_config(BRL_DRIVER, "auto"); */
  /* braille_config(BRL_TABLE, "foo.tbl"); */
  /* braille_config(BRL_PATH, "/foo/bar/"); */
  /* ... */

  if(!braille_init())
    {
      fprintf(stderr, "Error initialising libbraille: %s\n", braille_geterror());
      
/*
	fprintf(stderr, "Testing fake driver\n");
	braille_config(BRL_DRIVER, "none");
	if(!braille_init())
	{ 
	  fprintf(stderr, "Error initialising libbraille: %s\n", braille_geterror());
*/
#ifdef WIN32
	  printf("Press ENTER...\n");
	  getchar();
#endif
	  return 1;
/*	} */
    }

  printf("Libbraille version %s initialised correctly " \
	 "with driver %s using terminal %s with %d cells on device %s\n",
	 braille_info(BRL_VERSION), braille_info(BRL_DRIVER),
	 braille_info(BRL_TERMINAL), braille_size(),
	 braille_info(BRL_DEVICE));

  /* we want the braille_read function to wait forever while no key
     has been pressed */
  /* braille_timeout(-1); */

  /* we want the braille_read function to wait at most 1s while no key
     has been pressed */
  braille_timeout(1000);

  printf("displaying string\n");
  /* simple way to display something */
  braille_display("test_libbraille started");

  /* the complex and powerful way to display something */
  {
    char *hello_msg = "test_libbraille started";

    braille_write(hello_msg, strlen(hello_msg)); 
    braille_filter(BRAILLE(0, 0, 0, 0, 0, 0, 1, 1), 0);
    braille_filter(BRAILLE(0, 0, 0, 0, 0, 0, 1, 1), 23);
    braille_render();
  }

  /* Displaying a few things on the status cells */
  if(braille_statussize() > 0)
    braille_statusdisplay("abc");

  /* get the keys pressed on the display */
  for (i = 0; i < 20; i++)
  /* while (1) */
    {
      signed char status;
      brl_key key;
      
      status = braille_read(&key);
      if(status == -1)
	{
	  printf("error in braille_read: %s", braille_geterror());
	}
      else if(status)
	{
	  printf("Read key with type %d and code %d\n", key.type, key.code);
	  switch(key.type)
	    {
	    case BRL_NONE:
	      break;
	    case BRL_CURSOR:
	      printf("pressed cursor: %d\n", key.code);
	      snprintf (message, MAXLEN, "pressed cursor: %d", key.code);
	      braille_display(message);
	      break;
	    case BRL_CMD:
	      switch(key.code)
		{
		case BRLK_UP:
		  printf("pressed up\n");
		  braille_display("pressed up");
		  break;
		case BRLK_DOWN:
		  printf("pressed down\n");
		  braille_display("pressed down");
		  break;
		case BRLK_ABOVE:
		  printf("pressed above\n");
		  braille_display("pressed above");
		  break;
		case BRLK_BELOW:
		  printf("pressed below\n");
		  braille_display("pressed below");
		  break;
		case BRLK_HOME:
		  printf("pressed home\n");
		  braille_display("pressed home");
		  break;
		case BRLK_END:
		  printf("pressed end\n");
		  braille_display("pressed end");
		  break;
		case BRLK_BACKWARD:
		  printf("pressed backward\n");
		  braille_display("pressed backward");
		  break;
		case BRLK_FORWARD:
		  printf("pressed forward\n");
		  braille_display("pressed forward");
		  break;
		case BRLK_UNKNOWN:
		  printf("command unknown\n");
		  braille_display("command unknown");
		  break;
		default:
		  printf("unknown command with code: %d\n", key.code);
		  snprintf (message, MAXLEN, "unknown command with code: %d", key.code);
		  braille_display (message);
		  break;
		}
	      break;
	    case BRL_KEY:
	      printf("braille: 0x%x\n", key.braille);
	      printf("code: %d ou 0x%x\n", key.code, key.code);
	      printf("char: %c\n", key.code);
	      snprintf (message, MAXLEN, "pressed key with code: %d", key.code);
	      braille_display (message);
	      break;
	    default:
	      printf ("unknown type %d\n", key.type);
	    }
	}
    }

  /* now we stop everything */
  braille_close();

  printf("Leaving test_libbraille\n");
  braille_display ("Leaving test_libbraille");

#ifdef WIN32
  printf("Press ENTER...\n");
  getchar();
#endif

  return 0;
}
