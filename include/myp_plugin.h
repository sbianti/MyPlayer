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

#ifndef MYP_PLUGIN_H
#define MYP_PLUGIN_H

#include "common.h"
#include "myp_ui.h"

#define PROP_TIMELINE_VISIBLE "timeline-visible"

typedef void (*handle_stop_t)();

struct __myp_plugin_t__ {
  void (*init)(int argc, char *argv[], handle_stop_t func);
  void (*quit)();
  gboolean (*seturi)(char *uri);
  gboolean (*play)(gdouble speed);
  gboolean (*play_pause)();
  gboolean (*stop)();
  gboolean (*seek)(gint64 seek);
  gboolean (*set_pos)(gint64 pos);
  gboolean (*set_speed)(gboolean relative, gdouble val);
  gboolean (*step)(int n_frame);
  gboolean (*set_soft_volume)(gboolean relative, gint val);
  gboolean (*toggle_soft_mute)();
  gboolean (*set_prop)(const char *name, gboolean activate);
  gboolean (*discover)();
  void (*window_handler)(guintptr window_handle);
  void (*set_ui_plugin)(myp_ui_t myp_ui);

  enum myp_plugin_state_t (*state)();

  const char *(*plugin_name)();
  const char *(*plugin_version)();
  const char *(*plugin_info)();
};

typedef struct __myp_plugin_t__* myp_plugin_t;

#define MYP_PLUGIN_NEW() (g_try_new(struct __myp_plugin_t__, 1))

myp_plugin_t prepare_plugin();

#endif /* MYP_PLUGIN_H */
