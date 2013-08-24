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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#ifdef HAVE_TERMIOS
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_SYS_TERMIOS_H
#include <sys/termios.h>
#endif
#endif

#include <glib.h>

#include "print.h"
#include "playlist.h"
#include "context.h"

static volatile sig_atomic_t terminated = 0;
static myp_status_t status = SUCCESS;
static myp_context_t ctx;
static gboolean option_quiet = FALSE;
static gboolean option_version = FALSE;
static gboolean option_interactive_mode = FALSE;
static int option_loop = 1;
static gboolean option_random = FALSE;
static gboolean termset = FALSE;
#ifdef HAVE_TERMIOS
static struct termios tio_orig;
#endif

#define USAGE "Usage:  myplayer [options] [url|path/]filename"

static void set_terminal()
{
#ifdef HAVE_TERMIOS
  struct termios tio_new;

  if (isatty(0) != 1 || tcgetattr(0, &tio_orig) != 0) {
    printerrl("%s", g_strcmp0(ttyname(0), "") == 0 ?
	      "stdin is not a terminal":"tcgetattr failed !");
    return;
  }

  tio_new = tio_orig;

  tio_new.c_lflag &= ~(ECHO|ICANON);
  tio_new.c_cc[VMIN] = 1;
  tio_new.c_cc[VTIME] = 0;

  if (tcsetattr(0, TCSANOW, &tio_new) == 0)
    termset = TRUE;
  else
    printerr("tcsetattr failed !");
#endif
}

static void reset_terminal()
{
  if (termset == FALSE)
    return;

#ifdef HAVE_TERMIOS
  tcsetattr(0, TCSANOW, &tio_orig);
#endif
  termset = FALSE;
}

static void quit(int status, char *format, ...)
{
  va_list args;

  va_start(args, format);
  if (status > SUCCESS)
    vprinterrl(format, args);
  else
    vprintl(format, args);
  va_end(args);

  reset_terminal();
  exit(status);
}

static void sig_term(int sig)
{
  if (terminated > 0)
    return;

  terminated = 1;

  quit(status, "killed by sig %d", sig);
}

static void manage_options()
{
  if (option_version) {
#ifdef PACKAGE_VERSION
    printl("MyPlayer version "PACKAGE_VERSION);
#else
    printl("MyPlayer Unknown version");
#endif
    status = EXIT;
  }
}

static void enter_command_mode()
{
  printl("Command mode !!");
}

static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond,
				myp_context_t ctx)
{
  gchar car;

  if (g_io_channel_read_chars(source, &car, 1, NULL, NULL) !=
      G_IO_STATUS_NORMAL)
    return TRUE;

  switch (car) {
  case '':
    enter_command_mode();
    break;
  case 'q':
    g_main_loop_quit(ctx->process_loop);
    break;
  case '\n':
  case '>':
    myp_plst_next(ctx->playlist, ctx->myp_plugin);
    break;
  case '<':
    myp_plst_pred(ctx->playlist, ctx->myp_plugin);
    break;
  case 'f':
    printl("Full screen !");
    break;
  case 'o':
    printl("toggle OSD");
    break;
  case 'p':
  case ' ':
    printl("pause");
    break;
  default:
    printl("No bind found for key '%c'", car);
  }

  return TRUE;
}

int main(int argc, char *argv[])
{
  struct sigaction sa;
  GError *error = NULL;
  GIOChannel *io_stdin;
  myp_plugin_t my_plugin;
  GOptionContext *context;

  GOptionEntry options[] = {
    { "quiet", 'q', 0, G_OPTION_ARG_NONE, &option_quiet,
      "doesn't print anything on stdout", NULL },
    { "version", 'v', 0, G_OPTION_ARG_NONE, &option_version,
      "print version and bye byes", NULL },
    { "interactive-mode", 'i', 0, G_OPTION_ARG_NONE, &option_interactive_mode,
      "doesn't quit if playlist is empty or finished", NULL},
    { "loop", 'l', 0, G_OPTION_ARG_INT, &option_loop,
      "number of loop in the this playlist (0 means ∞)", NULL },
    { "random", 'r', 0, G_OPTION_ARG_NONE, &option_random,
      "play randomly in the playlist", NULL },
    { NULL }
  };

  context = g_option_context_new("");

  g_option_context_add_main_entries(context, options, "");
  if (!g_option_context_parse(context, &argc, &argv, &error))
    quit(ERROR, "option parsing failed: %s", error->message);

  g_option_context_free(context);

  ctx = myp_context_new();

  manage_options();
  if (status > SUCCESS)
    quit(status, NULL);

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sig_term;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  ctx->playlist = myp_plst_parse_cmdline(argc, argv);
  if (ctx->playlist == NULL)
    quit(ERROR, "myp_plst_parse_cmdline failed to create playlist");

  if (myp_plst_is_empty(ctx->playlist) && !option_interactive_mode)
    quit(ERROR, USAGE);

  if (option_loop < 0) {
    printerrl("Warning: loop < 0 is an illegal value. Set to default value 1");
    option_loop = 1;
  }

  myp_plst_set_random(ctx->playlist, option_random);
  myp_plst_set_loop(ctx->playlist, --option_loop);

  io_stdin = g_io_channel_unix_new(0);

  set_terminal();

  g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, ctx);

  my_plugin = prepare_plugin();

  myp_set_myp_plugin(ctx, my_plugin);

  ctx->myp_plugin->init(argc, argv);

  myp_plst_play(ctx->playlist, ctx->myp_plugin);
  ctx->process_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(ctx->process_loop);

  g_main_loop_unref(ctx->process_loop);
  g_io_channel_unref(io_stdin);
  reset_terminal();

  return status;
}
