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

#include "playlist.h"
#include "print.h"

myp_playlist_t parse_cmdline(int argc, char **argv)
{
  myp_playlist_t playlist = g_try_new(struct __myp_playlist_t__, 1);

  if (playlist == NULL)
    return NULL;

  playlist->list = NULL;
  playlist->current = NULL;
  playlist->loop = FALSE;
  playlist->random = FALSE;

  for (; argc > 0; argc--)
    playlist->list = g_list_prepend(playlist->list, argv[argc]);

  return playlist;
}

gboolean play(myp_playlist_t playlist)
{
  if (playlist->current == NULL)
    playlist->current = playlist->list;

  return TRUE;
}

gboolean stop(myp_playlist_t playlist)
{
  return TRUE;
}

gboolean pause(myp_playlist_t playlist)
{
  return TRUE;
}

gboolean next(myp_playlist_t playlist)
{
  if (playlist->current == NULL) {
    playlist->current = playlist->list;
    return TRUE;
  }

  playlist->current = g_list_next(playlist->current);
  if (playlist->current == NULL) {
    if (playlist->loop == TRUE)
      playlist->current = g_list_first(playlist->list);
  }

  return TRUE;
}
