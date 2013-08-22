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

#include <glib.h>

#include "myp_plugin.h"

#ifndef PLAYLIST_H
#define PLAYLIST_H

struct __myp_playlist_t__ { /* la rendre opaque */
  GList *list;
  GList *current;
  int loop;
  gboolean random;
};

typedef struct __myp_playlist_t__* myp_playlist_t;

myp_playlist_t myp_plst_parse_cmdline(int argc, char **argv);

gboolean myp_plst_play(myp_playlist_t playlist, myp_plugin_t myp_plugin);
gboolean myp_plst_stop(myp_playlist_t playlist, myp_plugin_t myp_plugin);
gboolean myp_plst_pause(myp_playlist_t playlist, myp_plugin_t myp_plugin);
gboolean myp_plst_next(myp_playlist_t playlist, myp_plugin_t myp_plugin);
gboolean myp_plst_pred(myp_playlist_t playlist, myp_plugin_t myp_plugin);

gboolean myp_plst_is_empty(myp_playlist_t playlist);
gboolean myp_plst_set_loop(myp_playlist_t playlist, int loop);
int myp_plst_get_loop(myp_playlist_t playlist);
gboolean myp_plst_set_random(myp_playlist_t playlist, gboolean random);
gboolean myp_plst_get_random(myp_playlist_t playlist);

#endif /* PLAYLIST_H */
