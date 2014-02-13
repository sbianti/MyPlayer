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

#ifndef CONTEXT_H
#define CONTEXT_H

#include <glib.h>

#include "playlist.h"
#include <myp_plugin.h>
#include <myp_ui.h>

enum __myp_status_t__ { SUCCESS, EXIT, ERROR };
typedef enum __myp_status_t__ myp_status_t;

struct __myp_context_t__ {
  GMainLoop *process_loop;
  myp_playlist_t playlist;
  myp_plugin_t myp_plugin;
  myp_ui_t myp_ui;
};
typedef struct __myp_context_t__* myp_context_t;

myp_context_t myp_context_new();

#endif /* CONTEXT_H */
