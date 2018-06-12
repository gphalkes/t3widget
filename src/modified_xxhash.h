/* Copyright (C) 2018 G.P. Halkes
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
#ifndef T3_WIDGET_MODIFIED_XXHASH_
#define T3_WIDGET_MODIFIED_XXHASH_

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

#include <cstddef>
#include <t3widget/widget_api.h>

namespace t3_widget {

/* Modified version of the xxHash{32,64} functions. The modification consists of skipping the
   endianess correction. This is OK, because we only guarantee that the hash value is stable within
   a single run of the program. We also use the 32-bit version on 32-bit computers, which means that
   there will be differences in outcome between different platforms. */
T3_WIDGET_LOCAL size_t ModifiedXXHash(const void *data, size_t length, size_t seed);
}  // namespace t3_widget

#endif
