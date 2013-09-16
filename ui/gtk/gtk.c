/*
 *
 *  MyPlayer
 *
 *  Copyright © 2013 Sébastien Bianti
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#endif
#include <gdk/gdkkeysyms.h>

#include <common.h>
#include <myp_ui.h>
#include <print.h>

#define UI_NAME "MyPlayer-Gtk"
#define MYP_UI_VERSION "0.0.1"

static gboolean fullscreen;
static GtkWidget *top_window;
static char *ui_info;
static guint window_ready_id;
static window_handle_setter_t set_window_handle;
static handle_keypressed_t handle_keypressed;

static void my_init(int argc, char *argv[], handle_keypressed_t func)
{
  gtk_init(&argc, &argv);

  ui_info = g_strdup_printf("UI Gtk for MyPlayer based on GTK-%u.%u.%u",
#if GTK_MAJOR_VERSION == 3
			    gtk_get_major_version(),
			    gtk_get_minor_version(),
			    gtk_get_micro_version()
#else
			    GTK_MAJOR_VERSION,
			    GTK_MINOR_VERSION,
			    GTK_MICRO_VERSION
#endif
			    );
  fullscreen = FALSE;

  top_window = NULL;
  set_window_handle = NULL;
  handle_keypressed = func;
}

static void my_quit()
{
  printl("\n%s says bye bye! (but does nothing else in fact)", UI_NAME);
}

static gboolean my_close()
{
  if (top_window) {
    gtk_widget_destroy(top_window);
    top_window = NULL;
    fullscreen = FALSE;
  }

  return TRUE;
}

static gboolean my_toggle_fullscreen()
{
  fullscreen = !fullscreen;

  if (fullscreen)
    gtk_window_fullscreen(GTK_WINDOW(top_window));
  else
    gtk_window_unfullscreen(GTK_WINDOW(top_window));

  return TRUE;
}

static gboolean keypress_cb(GtkWidget *widget, GdkEventKey *event,
			    void *null_data)
{
  handle_keypressed(event->keyval, gdk_keyval_name(event->keyval));

  return TRUE;
}

static void realize_cb(GtkWidget *widget, void *null_data)
{
  guintptr window_handle;
  GdkWindow *window = gtk_widget_get_window(widget);

  if (!gdk_window_ensure_native(window))
    printerrl("Couldn't create native window needed for GstVideoOverlay!");

#if defined (GDK_WINDOWING_X11)
  window_handle = GDK_WINDOW_XID(window);
#endif

  set_window_handle(window_handle);

  g_signal_connect(top_window, "key-press-event",
		   G_CALLBACK(keypress_cb), NULL);
}

static gboolean draw_cb(GtkWidget *widget, GdkEventExpose *event,
			player_state_func player_state)
{
  GtkAllocation allocation;
  GdkWindow *window;
  cairo_t *cr;

  if (player_state() > STATE_NULL)
    return FALSE;

  window = gtk_widget_get_window(widget);

  gtk_widget_get_allocation(widget, &allocation);

  cr = gdk_cairo_create(window);

  cairo_set_source_rgb(cr, 0, 0, 0);

  cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);

  cairo_fill(cr);

  cairo_destroy(cr);

  return FALSE;
}

static void my_create_window(player_state_func player_state)
{
  GtkWidget *inner_window = gtk_drawing_area_new();

  top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_widget_set_double_buffered(inner_window, FALSE);

  g_signal_connect(inner_window, "realize", G_CALLBACK(realize_cb), NULL);
#if GTK_MAJOR_VERSION == 2
  g_signal_connect(inner_window, "expose_event", G_CALLBACK(draw_cb),
		   player_state);
#else
  g_signal_connect(inner_window, "draw", G_CALLBACK(draw_cb), player_state);
#endif

  gtk_container_add(GTK_CONTAINER(top_window), inner_window);

  gtk_widget_show_all(top_window);
}

static void my_set_window_handler(window_handle_setter_t func)
{
  set_window_handle = func;
}

static void my_set_size(guint width, guint height)
{
  gtk_widget_set_size_request(top_window, width, height);
}

static const char *my_ui_name()
{
  return UI_NAME;
}

static const char *my_ui_version()
{
  return MYP_UI_VERSION;
}

static const char *my_ui_info()
{
  return ui_info;
}

myp_ui_t prepare_ui()
{
  myp_ui_t gtk_ui = MYP_UI_NEW();

  gtk_ui->init = my_init;
  gtk_ui->quit = my_quit;
  gtk_ui->close = my_close;
  gtk_ui->create_window = my_create_window;
  gtk_ui->set_window_handler = my_set_window_handler;
  gtk_ui->set_size = my_set_size;
  gtk_ui->toggle_fullscreen = my_toggle_fullscreen;
  gtk_ui->ui_name = my_ui_name;
  gtk_ui->ui_version = my_ui_version;
  gtk_ui->ui_info = my_ui_info;

  return gtk_ui;
}
