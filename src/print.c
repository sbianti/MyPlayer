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

#include <stdio.h>
#include <stdarg.h>

#include "print.h"

int print(const char *format, ...)
{
  va_list args;
  int ret;

  va_start(args, format);
  ret = vprintf(format, args);
  va_end(args);

  return ret;
}

int vprint(const char *format, va_list ap)
{
  return vprintf(format, ap);
}

int printl(const char *format, ...)
{
  va_list args;
  int ret;

  va_start(args, format);
  ret = vprintf(format, args);
  va_end(args);

  puts("");

  return ret+1;
}

int vprintl(const char *format, va_list ap)
{
  int ret = vprintf(format, ap);

  puts("");

  return ret+1;
}

int printerr(const char *format, ...)
{
  va_list args;
  int ret;

  va_start(args, format);
  ret = vfprintf(stderr, format, args);
  va_end(args);

  return ret;
}

int vprinterr(const char *format, va_list ap)
{
  return vfprintf(stderr, format, ap);
}

int printerrl(const char *format, ...)
{
  va_list args;
  int ret;

  va_start(args, format);
  ret = vfprintf(stderr, format, args);
  va_end(args);

  puts("");

  return ret+1;
}

int vprinterrl(const char *format, va_list ap)
{
  int ret = vfprintf(stderr, format, ap);

  puts("");

  return ret+1;
}
