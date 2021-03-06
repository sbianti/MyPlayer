/*
 *
 *  MyPlayer
 *
 *  Copyright © 2013-2014 Sébastien Bianti
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

#include <print.h>
#include "playlist.h"
#include "context.h"

#define CNTRL_R 65508
#define CNTRL_L 65507
#define LEFT_KEYVAL 65361
#define RIGHT_KEYVAL 65363
#define PAGE_UP_KEYVAL 65365
#define PAGE_DOWN_KEYVAL 65366
#define UP_KEYVAL 65362
#define DOWN_KEYVAL 65364
#define HOME_KEYVAL 65360
#define END_KEYVAL 65367
#define ENTER_KEYVAL 65293
#define KP_Enter_KEYVAL 65421
#define BACKSPACE_KEYVAL 65288
#define KP_Divide_KEYVAL 65455
#define KP_Multiply_KEYVAL 65450

static volatile sig_atomic_t terminated = 0;
static myp_status_t status = SUCCESS;
static myp_context_t ctx;
static gboolean option_quiet = FALSE;
static gboolean option_version = FALSE;
static gboolean option_idle = FALSE;
static int option_loop = 1;
static gboolean option_random = FALSE;
static gboolean option_hide_timeline = FALSE;
static gboolean option_fullscreen = FALSE;
static gboolean termset = FALSE;
#ifdef HAVE_TERMIOS
static struct termios tio_orig;
#endif

#define USAGE "Usage:  myplayer [options] [url|path/]filename"

#define PAGE_UP_KEY 0
#define PAGE_DOWN_KEY 1
#define HOME_OR_END_KEY 2

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

static void next_or_quit()
{
  if (myp_plst_next(ctx->playlist, ctx->myp_plugin, ctx->myp_ui) == FALSE &&
      option_idle == FALSE)
    g_main_loop_quit(ctx->process_loop);
}

static gboolean prv_exec_command_third_char_special_sequence(char car,
							     gboolean *next_is,
							     myp_context_t ctx)
{
  switch (car) {
  case 'A':
    ctx->myp_plugin->seek(60);
    break;
  case 'B':
    ctx->myp_plugin->seek(-60);
    break;
  case 'C':
    ctx->myp_plugin->seek(10);
    break;
  case 'D':
    ctx->myp_plugin->seek(-10);
    break;
  case '5':
    next_is[PAGE_UP_KEY] = TRUE;
    break;
  case '6':
    next_is[PAGE_DOWN_KEY] = TRUE;
    break;
  default:
    return FALSE;
  }

  return TRUE;
}

static gboolean prv_exec_command_last_char_special_sequence(char car,
							    gboolean *next_is,
							    myp_context_t ctx)
{
  gboolean ret = TRUE;

  switch (car) {
  case 'H':
    if (next_is[HOME_OR_END_KEY])
      ctx->myp_plugin->set_pos(0);
    else
      ret = FALSE;
    break;
  case 'F':
    if (next_is[HOME_OR_END_KEY])
      ctx->myp_plugin->set_pos(-1);
    else
      ret = FALSE;
    break;
  case '~':
    if (next_is[PAGE_UP_KEY])
      ctx->myp_plugin->seek(600);
    else if (next_is[PAGE_DOWN_KEY])
      ctx->myp_plugin->seek(-600);
    else
      ret = FALSE;
    break;
  default:
    ret = FALSE;
  }

  next_is[HOME_OR_END_KEY] = FALSE;
  next_is[PAGE_UP_KEY] = FALSE;
  next_is[PAGE_DOWN_KEY] = FALSE;

  return ret;
}

static gboolean handle_keypressed(guint key, char *key_name)
{
  static gboolean cntrl = FALSE;

  if (cntrl) {
    if (key == 'x')
      printl("^x");

    cntrl = FALSE;
    return TRUE;
  }

  switch (key) {
  case CNTRL_L:
  case CNTRL_R:
    cntrl = TRUE;
    break;
  case LEFT_KEYVAL:
    ctx->myp_plugin->seek(-10);
    break;
  case RIGHT_KEYVAL:
    ctx->myp_plugin->seek(10);
    break;
  case PAGE_UP_KEYVAL:
    ctx->myp_plugin->seek(600);
    break;
  case PAGE_DOWN_KEYVAL:
    ctx->myp_plugin->seek(-600);
    break;
  case UP_KEYVAL:
    ctx->myp_plugin->seek(60);
    break;
  case DOWN_KEYVAL:
    ctx->myp_plugin->seek(-60);
    break;
  case HOME_KEYVAL:
    ctx->myp_plugin->set_pos(0);
    break;
  case END_KEYVAL:
    ctx->myp_plugin->set_pos(-1);
    break;
  case 'q':
    myp_plst_stop(ctx->playlist, ctx->myp_plugin);
    g_main_loop_quit(ctx->process_loop);
    break;
  case KP_Enter_KEYVAL:
  case ENTER_KEYVAL:
  case '>':
    next_or_quit();
    break;
  case '<':
    myp_plst_pred(ctx->playlist, ctx->myp_plugin, ctx->myp_ui);
    break;
  case 'f':
    ctx->myp_ui->toggle_fullscreen();
    break;
  case 'o':
    printl("toggle OSD");
    break;
  case 'p':
  case ' ':
    ctx->myp_plugin->play_pause();
    break;
  case '{':
    ctx->myp_plugin->set_speed(TRUE, 0.5);
    break;
  case '}':
    ctx->myp_plugin->set_speed(TRUE, 2.0);
    break;
  case '[':
    ctx->myp_plugin->set_speed(TRUE, 0.9);
    break;
  case ']':
    ctx->myp_plugin->set_speed(TRUE, 1.1);
    break;
  case BACKSPACE_KEYVAL:
    ctx->myp_plugin->set_speed(FALSE, 1.0);
    break;
  case '.':
    ctx->myp_plugin->step(1);
    break;
  case 'm':
    ctx->myp_plugin->toggle_soft_mute();
    break;
  case KP_Multiply_KEYVAL:
  case '*':
    ctx->myp_plugin->set_soft_volume(TRUE, 10);
    break;
  case KP_Divide_KEYVAL:
  case '/':
    ctx->myp_plugin->set_soft_volume(TRUE, -10);
    break;
  default:
    if (strlen(key_name) < 2)
      printl("No bind found for key: %s (%d)", key_name, key);
  }

  return TRUE;
}

static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond,
				myp_context_t ctx)
{
  static gboolean echap[2] = { FALSE, FALSE };
  static gboolean next_is[3] = { FALSE, FALSE, FALSE };
  gunichar car = 0;

  if (g_io_channel_read_unichar(source, &car, NULL) !=
      G_IO_STATUS_NORMAL)
    return TRUE;

  if (echap[0] && !echap[1]) {
    if (car == '[')
      return echap[1] = TRUE;
    else {
      echap[0] = FALSE;
      if (car == 'O')
	return next_is[HOME_OR_END_KEY] = TRUE;
    }
  }

  if (echap[0] && echap[1]) {
    echap[0] = FALSE;
    echap[1] = FALSE;
    if (prv_exec_command_third_char_special_sequence(car, next_is, ctx))
      return TRUE;
  }

  if (next_is[0] || next_is[1] || next_is[2]) {
    if (prv_exec_command_last_char_special_sequence(car, next_is, ctx))
      return TRUE;
  }

  switch (car) {
  case '':
    enter_command_mode();
    break;
  case 'q':
    myp_plst_stop(ctx->playlist, ctx->myp_plugin);
    g_main_loop_quit(ctx->process_loop);
    break;
  case '\n':
  case '>':
    next_or_quit();
    break;
  case '<':
    myp_plst_pred(ctx->playlist, ctx->myp_plugin, ctx->myp_ui);
    break;
  case 'f':
    ctx->myp_ui->toggle_fullscreen();
    break;
  case 'o':
    printl("toggle OSD");
    break;
  case 'p':
  case ' ':
    ctx->myp_plugin->play_pause();
    break;
  case '{':
    ctx->myp_plugin->set_speed(TRUE, 0.5);
    break;
  case '}':
    ctx->myp_plugin->set_speed(TRUE, 2.0);
    break;
  case '[':
    ctx->myp_plugin->set_speed(TRUE, 0.9);
    break;
  case ']':
    ctx->myp_plugin->set_speed(TRUE, 1.1);
    break;
  case '':
    ctx->myp_plugin->set_speed(FALSE, 1.0);
    break;
  case 'm':
    ctx->myp_plugin->toggle_soft_mute();
    break;
  case '.':
    ctx->myp_plugin->step(1);
    break;
  case '*':
    ctx->myp_plugin->set_soft_volume(TRUE, 10);
    break;
  case '/':
    ctx->myp_plugin->set_soft_volume(TRUE, -10);
    break;
  case '':
    echap[0] = TRUE;
    break;
  default:
    printerr("No bind found for key ");
    if (car < 127)
      printerrl("%c", car);
    else
      printerrl("U+%04X", car);
  }

  return TRUE;
}

int main(int argc, char *argv[])
{
  struct sigaction sa;
  GError *error = NULL;
  GIOChannel *io_stdin;
  GOptionContext *context;

  GOptionEntry options[] = {
    { "quiet", 'q', 0, G_OPTION_ARG_NONE, &option_quiet,
      "doesn't print anything on stdout", NULL },
    { "version", 'v', 0, G_OPTION_ARG_NONE, &option_version,
      "print version and bye byes", NULL },
    { "idle-mode", 'i', 0, G_OPTION_ARG_NONE, &option_idle,
      "doesn't quit if playlist is empty or finished", NULL},
    { "loop", 'l', 0, G_OPTION_ARG_INT, &option_loop,
      "number of loop in the this playlist (0 means ∞)", NULL },
    { "random", 'r', 0, G_OPTION_ARG_NONE, &option_random,
      "play randomly in the playlist", NULL },
    { "hide-timeline", '\0', 0, G_OPTION_ARG_NONE, &option_hide_timeline,
      "doesn't display the timeline", NULL },
    { "fs", '\0', 0, G_OPTION_ARG_NONE, &option_fullscreen,
      "display video in fullscreen", NULL },
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

  if (myp_plst_is_empty(ctx->playlist) && !option_idle)
    quit(ERROR, USAGE);

  if (option_loop < 0) {
    printerrl("Warning: loop < 0 is an illegal value. Set to default value 1");
    option_loop = 1;
  }

  if (option_quiet)
    fclose(stdout);

  myp_plst_set_random(ctx->playlist, option_random);
  myp_plst_set_loop(ctx->playlist, --option_loop);

  io_stdin = g_io_channel_unix_new(0);

  set_terminal();

  g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, ctx);

  ctx->myp_plugin = prepare_plugin();
  ctx->myp_ui = prepare_ui();

  ctx->myp_plugin->init(argc, argv, next_or_quit);
  ctx->myp_ui->init(argc, argv, handle_keypressed);

  ctx->myp_plugin->set_ui_plugin(ctx->myp_ui);
  ctx->myp_ui->set_window_handler(ctx->myp_plugin->window_handler);

  printl("We are using %s version %s\n with more infos:%s"
	 "\n             %s version %s\n with more infos:%s",
	 ctx->myp_plugin->plugin_name(),
	 ctx->myp_plugin->plugin_version(),
	 ctx->myp_plugin->plugin_info(),
	 ctx->myp_ui->ui_name(),
	 ctx->myp_ui->ui_version(),
	 ctx->myp_ui->ui_info());

  if (option_hide_timeline)
    ctx->myp_plugin->set_prop("timeline-visible", FALSE);
  if (option_fullscreen)
    myp_plst_set_fullscreen(ctx->playlist, TRUE);

  if (myp_plst_play(ctx->playlist, ctx->myp_plugin, ctx->myp_ui) ||
      option_idle) {
    ctx->process_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(ctx->process_loop);
  }

  ctx->myp_plugin->quit();
  ctx->myp_ui->quit();

  printl("Exiting… (Quit)");

  g_main_loop_unref(ctx->process_loop);
  g_io_channel_unref(io_stdin);
  reset_terminal();

  return status;
}
