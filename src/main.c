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

#include <glib.h>

#include "print.h"
#include "playlist.h"
#include "context.h"

static volatile sig_atomic_t terminated = 0;
static myp_status_t status = SUCCESS;
static gboolean option_quiet = FALSE;
static gboolean option_version = FALSE;

static void quit(int status)
{
  exit(status);
}

static void sig_term(int sig)
{
  if (terminated > 0)
    return;

  terminated = 1;
  printl("killed by sig %d", sig);
  quit(status);
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

static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond,
				myp_context_t ctx)
{
  gchar *str = NULL;

  if (g_io_channel_read_line(source, &str, NULL, NULL, NULL) !=
      G_IO_STATUS_NORMAL)
    return TRUE;

  switch (g_ascii_tolower(str[0])) {
  case 'q':
    g_main_loop_quit(ctx->loop);
    break;
  case '\n':
  case '>':
    printl("→ next !");
    break;
  case '<':
    printl("← pred !");
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
    printl("No bind found for key '%c'", str[0]);
  }

  return TRUE;
}

int main(int argc, char *argv[])
{
  struct sigaction sa;
  GError *error = NULL;
  myp_playlist_t playlist = NULL;
  GIOChannel *io_stdin;
  myp_context_t ctx;
  GOptionContext *context;

  GOptionEntry options[] = {
    { "quiet", 'q', 0, G_OPTION_ARG_NONE, &option_quiet,
      "doesn't print anything on stdout", NULL },
    { "version", 'v', 0, G_OPTION_ARG_NONE, &option_version,
      "print version and bye byes", NULL },
    { NULL }
  };

  context = g_option_context_new("");

  g_option_context_add_main_entries(context, options, "");
  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    printl("option parsing failed: %s", error->message);
    quit(ERROR);
  }
  g_option_context_free(context);

  ctx = myp_context_new();

  manage_options();
  if (status > SUCCESS)
    quit(status);

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sig_term;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  playlist = parse_cmdline(argc, argv);
  if (playlist == NULL)
    quit(ERROR);

  io_stdin = g_io_channel_unix_new(fileno(stdin));
  g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, NULL);

  play(playlist);
  ctx->loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(ctx->loop);

  g_main_loop_unref(ctx->loop);
  g_io_channel_unref(io_stdin);

  return status;
}
