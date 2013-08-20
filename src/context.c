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

#include "context.h"

myp_context_t myp_context_new()
{
  return g_try_new(struct __myp_context_t__, 1);
}

gboolean myp_set_myp_plugin(myp_context_t context, myp_plugin_t myp_plugin)
{
  context->myp_plugin = myp_plugin;
}

