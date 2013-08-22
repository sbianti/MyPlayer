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

static gboolean prv_set_uri(myp_plugin_t myp_plugin, char *uri)
{
  if (myp_plugin->seturi(uri) == TRUE)
    return TRUE;

  printerrl("seturi <%s> failed", uri);
  return FALSE;
}

gboolean myp_plst_play(myp_playlist_t playlist, myp_plugin_t myp_plugin)
{
  if (myp_plst_is_empty(playlist))
    return FALSE;

  if (playlist->current == NULL) {
    playlist->current = playlist->list;
    if (prv_set_uri(myp_plugin, (char *)(playlist->current->data)) == FALSE)
      return FALSE;
  }

  myp_plugin->play();

  return TRUE;
}

gboolean myp_plst_stop(myp_playlist_t playlist, myp_plugin_t myp_plugin)
{
  if (myp_plst_is_empty(playlist))
    return FALSE;

  myp_plugin->stop();

  return TRUE;
}

gboolean myp_plst_pause(myp_playlist_t playlist, myp_plugin_t myp_plugin)
{
  if (myp_plst_is_empty(playlist))
    return FALSE;

  switch (myp_plugin->status()) {
  case STATUS_PLAYING:
    myp_plugin->pause();
    break;
  case STATUS_PAUSED:
    myp_plugin->play();
    break;
  case STATUS_NULL:
    return FALSE;
  }

  return TRUE;
}

gboolean myp_plst_next(myp_playlist_t playlist, myp_plugin_t myp_plugin)
{
  if (myp_plst_is_empty(playlist))
    return FALSE;

  if (playlist->current == NULL) {
    playlist->current = playlist->list;
    if (prv_set_uri(myp_plugin, (char *)(playlist->current->data)) == FALSE)
      return FALSE;

    myp_plugin->play();
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

gboolean myp_plst_pred(myp_playlist_t playlist, myp_plugin_t myp_plugin)
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