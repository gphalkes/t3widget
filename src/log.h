/* Copyright (C) 2011-2012,2018 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef T3_WIDGET_LOG_H
#define T3_WIDGET_LOG_H

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

#ifdef _T3_WIDGET_DEBUG
#include <typeinfo>
#endif

#include <stdio.h>
#include <t3widget/widget_api.h>

namespace t3widget {

#ifdef _T3_WIDGET_DEBUG

T3_WIDGET_LOCAL void init_log();
/* Note: these must be declared with T3_WIDGET_API such that they can be accessed
   from the clipboard modules. */
T3_WIDGET_API void lprintf(const char *fmt, ...)
#ifdef __GNUC__
    __attribute__((format(printf, 1, 2)))
#endif
    ;
T3_WIDGET_API void ldumpstr(const char *str, int length);
T3_WIDGET_API void logkeyseq(const char *keys);

#else

#define init_log()
#define lprintf(fmt, ...)
#define ldumpstr(str, length)
#define logkeyseq(keys)

#endif

}  // namespace t3widget
#endif
