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
#include <string>
#include <limits.h>
#include <cstring>
#include <unicode/unicode.h>

#include "log.h"
#include "util.h"
#include "internal.h"

using namespace std;
namespace t3_widget {

const optint None;
text_line_t *copy_buffer;

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



int parse_escape(const string &str, const char **error_message, size_t &read_position, size_t max_read_position, bool replacements) {
	size_t i;

	switch(str[read_position++]) {
		case 'n':
			return '\n';
		case 'r':
			return '\r';
		case '\'':
			return '\'';
		case '\\':
			return '\\';
		case 't':
			return '\t';
		case 'b':
			return '\b';
		case 'f':
			return '\f';
		case 'a':
			return '\a';
		case 'v':
			return '\v';
		case '?':
			return '\?';
		case '"':
			return '"';
		case 'x': {
			/* Hexadecimal escapes */
			int value = 0;
			/* Read at most two characters, or as many as are valid. */
			for (i = 0; i < 2 && (read_position + i) < max_read_position && isxdigit(str[read_position + i]); i++) {
				value <<= 4;
				//FIXME: isdigit doesn't necessarily do what we want, because it is locale dependent
				if (isdigit(str[read_position + i]))
					value += (int) (str[read_position + i] - '0');
				else
					value += (int) (tolower(str[read_position + i]) - 'a') + 10;
				if (value > UCHAR_MAX) {
					*error_message = "Invalid hexadecimal escape sequence";
					return false;
				}
			}
			read_position += i;

			if (i == 0) {
				*error_message = "Invalid hexadecimal escape sequence";
				return false;
			}

			return value;
		}
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			if (replacements) {
				return (int)(str[read_position - 1] - '0') | ESCAPE_REPLACEMENT;
			} else {
		case '0':
				/* Octal escapes */
				int value = (int)(str[read_position - 1] - '0');
				size_t max_idx = str[read_position - 1] < '4' ? 2 : 1;
				for (i = 0; i < max_idx && read_position + i < max_read_position && str[read_position + i] >= '0' && str[read_position + i] <= '7'; i++)
					value = value * 8 + (int)(str[read_position + i] - '0');

				read_position += i;

				return value;
			}
		case '8':
		case '9':
			if (replacements)
				return (int)(str[read_position - 1] - '0') | ESCAPE_REPLACEMENT;
			else {
				*error_message = "Invalid escape sequence";
				return -1;
			}
		case 'u':
		case 'U': {
			//FIXME: int is not necessarily large enough!
			int value = 0;
			size_t chars = str[read_position - 1] == 'U' ? 8 : 4;

			if (max_read_position < read_position + chars) {
				*error_message = str[read_position - 1] == 'U' ? "Too short \\U escape" : "Too short \\u escape";
				return -1;
			}
			for (i = 0; i < chars; i++) {
				//FIXME: isxdigit doesn't necessarily do what we want, because it is locale dependent
				if (!isxdigit(str[read_position + i])) {
					*error_message = str[read_position - 1] == 'U' ? "Too short \\U escape" : "Too short \\u escape";
					return -1;
				}
				value <<= 4;
				//FIXME: isdigit doesn't necessarily do what we want, because it is locale dependent
				if (isdigit(str[read_position + i]))
					value += (int) (str[read_position + i] - '0');
				else
					value += (int) (tolower(str[read_position + i]) - 'a') + 10;
			}

			if (value > 0x10FFFFL) {
				*error_message = "\\U escape out of range";
				return -1;
			}
			read_position += chars;
			return value | ESCAPE_UNICODE;
		}
		default:
			*error_message = "Invalid escape sequence";
			return -1;
	}
}

/** Convert a str from the input format to an internally usable str.
	@param str A @a Token with the str to be converted.
	@param error_message Location to store the error message if necessary.
	@return The length of the resulting str.

	The use of this function processes escape characters. The converted
	characters are written in the original str.
*/
bool parse_escapes(string &str, const char **error_message, bool replacements) {
	size_t max_read_position = str.size();
	size_t read_position = 0, write_position = 0;

	while(read_position < max_read_position) {
		if (str[read_position] == '\\') {
			int value;

			read_position++;

			if (read_position == max_read_position) {
				*error_message = "Single backslash at end of string";
				return false;
			}
			value = parse_escape(str, error_message, read_position, max_read_position, replacements);
			lprintf("Got value: %d\n", value);
			if (value < 0)
				return false;

			if (value & ESCAPE_UNICODE) {
				/* The conversion won't overwrite subsequent characters because
				   \uxxxx is already the as long as the max utf-8 length */
				char buffer[5];
				//FIXME: mask anything outside the correct value range
				t3_putuc(value & ~ESCAPE_UNICODE, buffer);
				buffer[4] = 0;
				str.replace(write_position, strlen(buffer), buffer);
				write_position += strlen(buffer);
				break;
			} else if (value & ESCAPE_REPLACEMENT) {
				//FIXME implement replacements
			} else {
				str[write_position++] = (char) value;
			}
		} else {
			str[write_position++] = str[read_position++];
		}
	}
	/* Terminate str. */
	str.erase(write_position);
	return true;
}

}; // namespace
