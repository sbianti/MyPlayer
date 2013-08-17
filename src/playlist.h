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

#ifndef PLAYLIST_H
#define PLAYLIST_H

struct __myp_playlist_t__ { /* la rendre opaque */
  GList *list;
  GList *current;
  gboolean loop;
  gboolean random;
};

typedef struct __myp_playlist_t__* myp_playlist_t;

myp_playlist_t myp_plst_parse_cmdline(int argc, char **argv);

gboolean myp_plst_play(myp_playlist_t playlist);
gboolean myp_plst_stop(myp_playlist_t playlist);
gboolean myp_plst_pause(myp_playlist_t playlist);
gboolean myp_plst_next(myp_playlist_t playlist);
gboolean myp_plst_pred(myp_playlist_t playlist);

#endif /* PLAYLIST_H */
