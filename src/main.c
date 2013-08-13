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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <signal.h>

static volatile sig_atomic_t terminated = 0;
static status = 0;

static void quit(int status)
{
  exit(status);
}

static void sig_term(int sig)
{
  if (terminated > 0)
    return;

  terminated = 1;

  quit(status);
}

int main(int argc, char *argv[])
{
  
  return status;
}
