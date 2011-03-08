/* Copyright (C) 2010 G.P. Halkes
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
#include <unistd.h>
#include <cerrno>

#include "util.h"

namespace t3_widget {

const optint None;

ssize_t nosig_write(int fd, const char *buffer, size_t bytes) {
	size_t start = 0;

	// Loop to ensure we write everything even when interrupted
	while (start < bytes) {
		ssize_t retval = write(fd, buffer + start, bytes - start);
		if (retval < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		start += retval;
	}
	return start;
}

ssize_t nosig_read(int fd, char *buffer, size_t bytes) {
	size_t bytes_read = 0;

	/* Use while loop to allow interrupted reads */
	while (true) {
		ssize_t retval = read(fd, buffer + bytes_read, bytes - bytes_read);
		if (retval == 0) {
			break;
		} else if (retval < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		} else {
			bytes_read += retval;
			if (bytes_read >= bytes)
				break;
		}
	}
	return bytes_read;
}

}; // namespace
