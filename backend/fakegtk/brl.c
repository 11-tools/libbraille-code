/* Libbraille - a portable library for Braille displays
 * Copyright (C) 2001-2006 by Sébastien Sablé
 *
 * This program comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the GNU Lesser
 * Library General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.  */

/* Author : Sebatien Sablé
 * Changes by Cédric Checoury : Added cursor keys
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <errno.h>

#include <driver.h>

/* file containing picture for a raised dot */
#define PICTURE_DOT_UP PKGDATADIR "/dots_b.png"
/* lowered dot */
#define PICTURE_DOT_DOWN PKGDATADIR "/dots_w.png"
/* cursor */
#define PICTURE_CURSOR PKGDATADIR "/cursor.png"
/* alternate image for the lowered dots */
#define BLANK PKGDATADIR "/blank.png"
/* number of cells of the braille display */
#define DISPLAY_X 30

static gboolean external_gtk;
static gboolean timeout_callback(gpointer data);
  
static GAsyncQueue *queue;

static brl_key key_none;
static brl_key key_1, key_2, key_3, key_4, key_5, key_6, key_7, key_8, key_space;
static brl_key key_left, key_rigth;
static brl_key key_cursor[DISPLAY_X];

static GtkWidget *window;        /* the bone of the graphic app own everything that is displayed */
/* static GtkWidget *menubar; */       /* the menu bar item */
static GtkWidget *label;         /* will display in text format the content of the braille display */
static GtkWidget *dots_image[4][2 * DISPLAY_X];
static GdkPixbuf *dot_up;        /* picture of a raised dots */
static GdkPixbuf *dot_down;      /* picture of a down dot */
static char *display_string;     /* string containing the text to be displayed */

/* prototypes */
static void callback(GtkWidget *widget, gpointer data);
static void callback2(GtkWidget *widget, GdkEvent *event, gpointer callback_data);
static void none(void);		 /* used to bypass the default exit signal of GTK */
static gpointer mainthread(gpointer ignored); /* the main thread running the gtk look */

/* returns the name of the terminal */
BRLDRV_API const char *
brli_drvinfo(brli_term *term, brl_config code)
{
  switch(code)
    {
    case BRL_DRIVER:
      return "fake";
    case BRL_TERMINAL:
      return "Fake terminal";
    default:
      return NULL;
    }
}

BRLDRV_API char
brli_drvinit(brli_term *brl, char type, const char *dev)
{
  int i, j;
  int argc = 0;
  char ***argv = NULL;
  GError *error = NULL;

  GtkWidget *button;
  GtkWidget *separator;       /* more visually convenient */
  GtkWidget *hbox_0;          /* in the box hierarchy the first and the biggest */
  GtkWidget *vbox_1_button;   /* own the 9 first button */
  GtkTable *vbox_2_dots;      /* own the braille display */
  GtkWidget *vbox_3_button;   /* own the 2 last button */

  brl->width = DISPLAY_X;     /* size of the braille display */

  key_none.type = BRL_NONE;
  key_none.code = 0;
  key_none.braille = 0;

  key_1.type = BRL_KEY;
  key_1.code = braille_braille2ascii(BRAILLE(1,0,0,0,0,0,0,0));
  key_1.braille = BRAILLE(1,0,0,0,0,0,0,0);

  key_2.type = BRL_KEY;
  key_2.code = braille_braille2ascii(BRAILLE(0,1,0,0,0,0,0,0));
  key_2.braille = BRAILLE(0,1,0,0,0,0,0,0);

  key_3.type = BRL_KEY;
  key_3.code = braille_braille2ascii(BRAILLE(0,0,1,0,0,0,0,0));
  key_3.braille = BRAILLE(0,0,1,0,0,0,0,0);

  key_4.type = BRL_KEY;
  key_4.code = braille_braille2ascii(BRAILLE(0,0,0,1,0,0,0,0));
  key_4.braille = BRAILLE(0,0,0,1,0,0,0,0);

  key_5.type = BRL_KEY;
  key_5.code = braille_braille2ascii(BRAILLE(0,0,0,0,1,0,0,0));
  key_5.braille = BRAILLE(0,0,0,0,1,0,0,0);

  key_6.type = BRL_KEY;
  key_6.code = braille_braille2ascii(BRAILLE(0,0,0,0,0,1,0,0));
  key_6.braille = BRAILLE(0,0,0,0,0,1,0,0);

  key_7.type = BRL_KEY;
  key_7.code = braille_braille2ascii(BRAILLE(0,0,0,0,0,0,1,0));
  key_7.braille = BRAILLE(0,0,0,0,0,0,1,0);

  key_8.type = BRL_KEY;
  key_8.code = braille_braille2ascii(BRAILLE(0,0,0,0,0,0,0,1));
  key_8.braille = BRAILLE(0,0,0,0,0,0,0,1);

  key_space.type= BRL_KEY;
  key_space.code = BRLK_SPACE;
  key_space.braille = 0;

  key_left.type= BRL_CMD;
  key_left.code = BRLK_BACKWARD;
  key_left.braille = 0;

  key_rigth.type= BRL_CMD;
  key_rigth.code = BRLK_FORWARD;
  key_rigth.braille = 0;

  brl->display = malloc(brl->width);
  brl->display_ascii = malloc(brl->width);
  display_string = malloc(brl->width + 1);
  if(!brl->display || !brl->display_ascii || !display_string)
    {
      brli_seterror("%s", strerror(errno));
      brli_drvclose(brl);
      return 0;
    }
  display_string[brl->width + 1] = '\0';

  if (!g_thread_supported ()) g_thread_init (NULL);
  gdk_threads_init();
  if(gtk_init_check(&argc, argv) == FALSE)
    {
      brli_seterror("Error initialising Gtk+");
      return 0;
    }

  queue = g_async_queue_new();

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(window, "delete_event", none, NULL);
  g_signal_connect(window, "destroy", none, NULL);
  g_signal_connect(window, "key_press_event", G_CALLBACK(callback2), NULL);
  gtk_window_set_title(GTK_WINDOW(window), "Fake terminal");  /* the title */

  /* vertical hbox_0 */
  hbox_0 = gtk_vbox_new(FALSE, FALSE);
  gtk_container_add(GTK_CONTAINER(window), hbox_0);

  /* horizontal vbox_1_button */
  vbox_1_button = gtk_hbox_new(TRUE, FALSE);
  gtk_box_pack_start(GTK_BOX(hbox_0), vbox_1_button, TRUE, FALSE, 0);

  /* buttons */
  button = gtk_button_new_with_label("7/ESC");
  gtk_box_pack_start(GTK_BOX(vbox_1_button), button, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(callback), (gpointer)&key_7);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("3");
  gtk_box_pack_start(GTK_BOX(vbox_1_button), button, TRUE, TRUE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(callback), (gpointer)&key_3);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("2");
  gtk_box_pack_start(GTK_BOX(vbox_1_button), button, TRUE, TRUE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(callback), (gpointer)&key_2);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("1");
  gtk_box_pack_start(GTK_BOX(vbox_1_button), button, TRUE, TRUE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(callback), (gpointer)&key_1);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("ESP");
  gtk_box_pack_start(GTK_BOX(vbox_1_button), button, TRUE, TRUE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(callback), (gpointer)&key_space);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("4");
  gtk_box_pack_start(GTK_BOX(vbox_1_button), button, TRUE, TRUE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(callback), (gpointer)&key_4);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("5");
  gtk_box_pack_start(GTK_BOX(vbox_1_button), button, TRUE, TRUE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(callback), (gpointer)&key_5);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("6");
  gtk_box_pack_start(GTK_BOX(vbox_1_button), button, TRUE, TRUE, 0);
  g_signal_connect(button, "clicked",G_CALLBACK(callback) , (gpointer)&key_6);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("8/RET");
  gtk_box_pack_start(GTK_BOX(vbox_1_button), button, TRUE, TRUE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(callback), (gpointer)&key_8);
  gtk_widget_show(button);

  /* separator from the first part */
  separator  = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(hbox_0), separator, TRUE, TRUE, 0);
  gtk_widget_show(separator);

  label = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox_0), label, TRUE, TRUE, 0);
  gtk_widget_show(label);

  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(hbox_0), separator, TRUE, TRUE, 0);
  gtk_widget_show(separator);
  
  /* vbox_2_dots */
  vbox_2_dots = (GtkTable *)gtk_table_new(8, 2 * DISPLAY_X, TRUE);
  gtk_box_pack_start(GTK_BOX(hbox_0), (GtkWidget *)vbox_2_dots, FALSE, FALSE, 0);
  for(i = 1; i < 2 * DISPLAY_X; i += 2)
    {
      gtk_table_set_col_spacing((GtkTable *)vbox_2_dots, i, 10); /* spacing columns 2 by 2 */
    }

  dot_down = gdk_pixbuf_new_from_file(PICTURE_DOT_DOWN, &error);
  if(error != NULL)
    {
      fprintf(stderr, "Unable to read file: %s\n", error->message);
      g_error_free(error);
      brli_drvclose(brl);
      return 0;
    } 
  
  dot_up = gdk_pixbuf_new_from_file(PICTURE_DOT_UP, &error);
  if(error != NULL)
    {
      fprintf(stderr, "Unable to read file: %s\n", error->message);
      g_error_free(error);
      brli_drvclose(brl);
      return 0;
    } 

  /* create cursor line */
  for(i = 0; i < DISPLAY_X; i++)
    {
      button = gtk_button_new_with_label("-");
      gtk_table_attach_defaults(GTK_TABLE(vbox_2_dots), button,2*i , 2*i+2,0 ,4);
      key_cursor[i].type=BRL_CURSOR;
      key_cursor[i].braille=0;
      key_cursor[i].code=i;
      g_signal_connect(button,"clicked", G_CALLBACK(callback), (gpointer)&key_cursor[i]);
      gtk_widget_show(button);
    }

  /* create the table on the screen representing the Braille dots */
  for(i = 0; i < 2 * DISPLAY_X; i++)
    {
      for(j = 0; j < 4; j++)
	{
	  dots_image[j][i] = (GtkWidget*)gtk_image_new_from_pixbuf(dot_down);
	  gtk_table_attach_defaults(GTK_TABLE(vbox_2_dots), dots_image[j][i],
				    i, i + 1, j + 4, j + 5);
	  gtk_widget_show(dots_image[j][i]);
	}
    }

  /* separator braille - directional key */
  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(hbox_0), separator, TRUE, TRUE, 0);
  gtk_widget_show(separator);

  /* vbox_3_button */
  vbox_3_button = gtk_hbox_new(TRUE, FALSE);
  gtk_box_pack_start(GTK_BOX(hbox_0), vbox_3_button, TRUE, FALSE, 0);

  /* button - directional key : left */
  button = gtk_button_new_with_label("<--");
  g_signal_connect(button, "clicked", G_CALLBACK(callback), (gpointer)&key_left);
  gtk_box_pack_start(GTK_BOX(vbox_3_button), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  /* button - directional key : right */
  button = gtk_button_new_with_label("-->");
  g_signal_connect(button, "clicked", G_CALLBACK(callback), (gpointer)&key_rigth);
  gtk_box_pack_start(GTK_BOX(vbox_3_button), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  /* we show everything here */
  gtk_widget_show(hbox_0);
  gtk_widget_show(vbox_1_button);
  gtk_widget_show((GtkWidget *)vbox_2_dots);
  gtk_widget_show(vbox_3_button);
  gtk_widget_show(window);

  if (gtk_main_level () == 0)
  {
      external_gtk = 0;
      g_thread_create (mainthread, NULL, FALSE, NULL);
  }
  else
  {
      external_gtk = 1;
  }

  return 1;
}

BRLDRV_API char
brli_drvclose(brli_term *term)
{
  gtk_main_quit(); /* exit gtk main loop */
  if(term->display)
    free(term->display);
  if(term->display_ascii)
    free(term->display_ascii);
  term->width = -1;

  g_async_queue_unref(queue);

  return 1;
}

/* this function is called whenever one wants to write something on */
/* the braille display */
BRLDRV_API char
brli_drvwrite(brli_term *brl)
{
  int i;

  if (!external_gtk)
      gdk_threads_enter();

  for(i = 0; i < brl->width; i++)
    {
      if(brl->display[i] & 1)
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[0][i * 2], dot_up);
	}
      else
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[0][i * 2], dot_down);
	}
      if(brl->display[i] & 2)
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[1][i * 2], dot_up);
	}
      else
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[1][i * 2], dot_down);
	}
      if(brl->display[i] & 4)
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[2][i * 2], dot_up);
	}
      else
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[2][i * 2], dot_down);
	}
      if(brl->display[i] & 8)
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[0][i * 2 + 1], dot_up);
	}
      else
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[0][i * 2 + 1], dot_down);
	}
      if(brl->display[i] & 16)
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[1][i * 2 + 1], dot_up);
	}
      else
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[1][i * 2 + 1], dot_down);
	}
      if(brl->display[i] & 32)
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[2][i * 2 + 1], dot_up);
	}
      else
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[2][i * 2 + 1], dot_down);
	}
      if(brl->display[i] & 64)
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[3][i * 2], dot_up);
	}
      else
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[3][i * 2], dot_down);
	}
      if(brl->display[i] & 128)
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[3][i * 2 + 1], dot_up);
	}
      else
	{
	  gtk_image_set_from_pixbuf((GtkImage *)dots_image[3][i * 2 + 1], dot_down);
	}
    }

  /* display the string on the window */
  strncpy(display_string, brl->display_ascii, brl->width);
  gtk_label_set_text((GtkLabel *)label, display_string);

  gdk_flush ();
  if (!external_gtk)
      gdk_threads_leave();

  return 1;
}

/* set status cell */
BRLDRV_API char
brli_drvstatus(brli_term *term)
{
  return 1;
}

BRLDRV_API signed char
brli_drvread(brli_term *term, brl_key *key)
{
  guint timeout;
  brl_key *key_pressed;

  if(term->timeout == 0)
    return 0;

  timeout = g_timeout_add(term->timeout, timeout_callback, NULL);
  key_pressed = (brl_key *)g_async_queue_pop(queue);
  g_source_remove(timeout);

  if(key_pressed->type != BRL_NONE)
    {
      memcpy(key, key_pressed, sizeof(brl_key));
      free(key_pressed);
      return 1;
    }
  else
    {
      free(key_pressed);
      return 0;
    }
}

static void
callback(GtkWidget *widget, gpointer data)
{
  brl_key *key;

  key = malloc(sizeof(brl_key));
  if(!key)
    {
      brli_log(LOG_WARNING, "Cannot allocate memory in callback");
      return;
    }
  memcpy(key, (brl_key *)data, sizeof(brl_key));
  g_async_queue_push(queue, key);
}

static void
callback2(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  GdkEventKey* temp = (GdkEventKey*)event;

  /* only char and num key */
  if((*temp).keyval < 129 && (*temp).keyval > 31)
    {
      brl_key *key;

      key = malloc(sizeof(brl_key));
      if(!key)
	{
	  brli_log(LOG_WARNING, "Cannot allocate memory in callback");
	  return;
	}
      key->type = BRL_KEY;
      key->code = (*temp).keyval;
      /* key.braille = braille_ascii2braille((*temp).keyval); */
      g_async_queue_push(queue, key);
    }
}

static gboolean
timeout_callback(gpointer data)
{
  brl_key *key;

  key = malloc(sizeof(brl_key));
  if(!key)
    {
      brli_log(LOG_WARNING, "Cannot allocate memory in timeout_callback");
      return FALSE;
    }
  memcpy(key, &key_none, sizeof(brl_key));
  g_async_queue_push(queue, key);

  return FALSE;
}

static gpointer
mainthread(gpointer ignored)
{
  gdk_threads_enter();
  gtk_main();
  gdk_threads_leave();
  return NULL;
}

static void
none(void)
{
}
