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

myp_playlist_t myp_plst_parse_cmdline(int argc, char **argv)
{
  myp_playlist_t playlist = g_try_new(struct __myp_playlist_t__, 1);

  if (playlist == NULL)
    return NULL;

  playlist->list = NULL;
  playlist->current = NULL;
  playlist->loop = 0;
  playlist->random = FALSE;

  for (argc--; argc > 0; argc--)
    playlist->list = g_list_prepend(playlist->list, argv[argc]);

  return playlist;
}

gboolean myp_plst_play(myp_playlist_t playlist)
{
  if (myp_plst_is_empty(playlist))
    return FALSE;

  if (playlist->current == NULL)
    playlist->current = playlist->list;

  return TRUE;
}

gboolean myp_plst_stop(myp_playlist_t playlist)
{
  if (myp_plst_is_empty(playlist))
    return FALSE;

  return TRUE;
}

gboolean myp_plst_pause(myp_playlist_t playlist)
{
  if (myp_plst_is_empty(playlist))
    return FALSE;

  return TRUE;
}

gboolean myp_plst_next(myp_playlist_t playlist)
{
  if (myp_plst_is_empty(playlist))
    return FALSE;

  if (playlist->current == NULL) {
    playlist->current = playlist->list;
    return TRUE;
  }

  playlist->current = g_list_next(playlist->current);
  if (playlist->current != NULL)
    return TRUE;

  if (playlist->loop == 0) {
    playlist->current = g_list_last(playlist->list);
    return FALSE;
  }

  playlist->current = g_list_first(playlist->list);

  if (playlist->loop != -1)
    playlist->loop--;

  return TRUE;
}

gboolean myp_plst_pred(myp_playlist_t playlist)
{
  if (myp_plst_is_empty(playlist))
    return FALSE;

  if (playlist->current == NULL) {
    playlist->current = playlist->list;
    return TRUE;
  }

  playlist->current = g_list_previous(playlist->current);

  if (playlist->current != NULL)
    return TRUE;

  if (playlist->loop == 0) {
    playlist->current = playlist->list;
    return FALSE;
  }

  playlist->current = g_list_last(playlist->list);
  if (playlist->loop != -1)
    playlist->loop++;

  return TRUE;
}

gboolean myp_plst_is_empty(myp_playlist_t playlist)
{
  return playlist->list == NULL;
}
