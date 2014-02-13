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

#ifndef MYP_UI_H
#define MYP_UI_H

#include "common.h"

typedef void (*window_handle_setter_t)(guintptr window_handle);
typedef gboolean (*handle_keypressed_t)(guint key, char *key_name);

struct __myp_ui_t__ {
  void (*init)(int argc, char *argv[], handle_keypressed_t func);
  void (*quit)();
  gboolean (*close)();
  void (*create_window)(player_state_func player_state);
  void (*set_window_handler)(window_handle_setter_t func);
  void (*set_size)(guint width, guint height);
  guintptr (*get_window)();
  gboolean (*toggle_fullscreen)();

  const char *(*ui_name)();
  const char *(*ui_version)();
  const char *(*ui_info)();
};

typedef struct __myp_ui_t__* myp_ui_t;

#define MYP_UI_NEW() (g_try_new(struct __myp_ui_t__, 1))

myp_ui_t prepare_ui();

#endif /* MYP_UI_H */
