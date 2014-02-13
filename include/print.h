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

#ifndef PRINT_H
#define PRINT_H

int print(const char *format, ...);
int vprint(const char *format, va_list ap);
int printl(const char *format, ...);
int vprintl(const char *format, va_list ap);
int printerr(const char *format, ...);
int vprinterr(const char *format, va_list ap);
int printerrl(const char *format, ...);
int vprinterrl(const char *format, va_list ap);

#endif /* PRINT_H */
