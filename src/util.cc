/* Copyright (C) 2011-2012 G.P. Halkes
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
#include <sys/types.h>
#include <sys/stat.h>
#include <t3window/utf8.h>
#include <transcript/transcript.h>
#include <unictype.h>

#include "log.h"
#include "util.h"
#include "internal.h"
#include "main.h"

using namespace std;

#ifndef HAS_STRDUP
/** strdup implementation if none is provided by the environment. */
char *_t3_widget_strdup(const char *str) {
	char *result;
	size_t len = strlen(str) + 1;

	if ((result = (char *) malloc(len)) == NULL)
		return NULL;
	memcpy(result, str, len);
	return result;
}
#endif

namespace t3_widget {

static void lang_codeset_init(bool init);

static sigc::connection lang_codeset_init_connection = connect_on_init(sigc::ptr_fun(lang_codeset_init));
static transcript_t *lang_codeset_handle;
static bool lang_codeset_is_utf8;

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

static bool is_hex_digit(int c) {
	return strchr("abcdefABCDEF0123456789", c) != NULL;
}

static int to_lower(int c) {
	return 'a' + (c - 'A');
}

int parse_escape(const string &str, const char **error_message, size_t &read_position, bool replacements) {
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
			for (i = 0; i < 2 && (read_position + i) < str.size() && is_hex_digit(str[read_position + i]); i++) {
				value <<= 4;
				if (str[read_position + i] >= '0' && str[read_position + i] <= '9')
					value += (int) (str[read_position + i] - '0');
				else
					value += (int) (to_lower(str[read_position + i]) - 'a') + 10;
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
				for (i = 0; i < max_idx && read_position + i < str.size() && str[read_position + i] >= '0' &&
						str[read_position + i] <= '7'; i++)
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
			uint32_t value = 0;
			size_t chars = str[read_position - 1] == 'U' ? 8 : 4;

			if (str.size() < read_position + chars) {
				*error_message = str[read_position - 1] == 'U' ? "Too short \\U escape" : "Too short \\u escape";
				return -1;
			}
			for (i = 0; i < chars; i++) {
				if (!is_hex_digit(str[read_position + i])) {
					*error_message = str[read_position - 1] == 'U' ? "Too short \\U escape" : "Too short \\u escape";
					return -1;
				}
				value <<= 4;
				if (str[read_position + i] >= '0' && str[read_position + i] <= '9')
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

/** Convert a string from the input format to an internally usable string.
	@param str A @a Token with the str to be converted.
	@param error_message Location to store the error message if necessary.
    @param replacements Mark replacment substitutions (\\1 .. \\9) in the string.
	@return The length of the resulting str.

	The use of this function processes escape characters. The converted
	characters are written in the original str.
*/
bool parse_escapes(string &str, const char **error_message, bool replacements) {
	size_t read_position = 0, write_position = 0;
	char buffer[5];

	while(read_position < str.size()) {
		if (str[read_position] == '\\') {
			int value;

			read_position++;

			if (read_position == str.size()) {
				*error_message = "Single backslash at end of string";
				return false;
			}
			value = parse_escape(str, error_message, read_position, replacements);

			if (value < 0)
				return false;

			if (value & ESCAPE_UNICODE) {
				/* The conversion won't overwrite subsequent characters because
				   \uxxxx is already the as long as the max utf-8 length */
				value &= ~ESCAPE_UNICODE;
				if (value == 0) {
					str[write_position++] = 0;
				} else {
					t3_utf8_put(value, buffer);
					buffer[4] = 0;
					str.replace(write_position, strlen(buffer), buffer);
					write_position += strlen(buffer);
				}
				break;
			} else if (value & ESCAPE_REPLACEMENT) {
				/* Write a specific invalid UTF-8 string, that we can later recognize.
				   For this we use the 0xd901-0xd909 range. */
				memset(buffer, 0, sizeof(buffer));
				t3_utf8_put((value & ~ESCAPE_REPLACEMENT) + 0xd900, buffer);
				str.replace(write_position, 2, buffer);
				/* Unfortunately, the expanded escape sequence doesn't fit in the previously
				   allocated space, so we have to adjust the read_position
				   as well as the write_position. */
				write_position += strlen(buffer);
				read_position += strlen(buffer) - 2;
			} else {
				/*FIXME: handle bytes with values > 0x7f
				   We should write a value in the range 0xd800-0xd8ff. However, if a
				   sequence of such bytes forms a valid UTF-8 sequence (taking into account
				   the validity of the referenced code point), it should be replaced by
				   the correct UTF-8 sequence.
				*/
				if (value < 0x80)
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

string get_working_directory(void) {
	size_t buffer_max = 511;
	char *buffer = NULL, *result;

	do {
		result = (char *) realloc(buffer, buffer_max);
		if (result == NULL) {
			free(buffer);
			throw ENOMEM;
		}
		buffer = result;
		if ((result = getcwd(buffer, buffer_max)) == NULL) {
			if (errno != ERANGE) {
				int error_save = errno;
				free(buffer);
				throw error_save;
			}

			if (((size_t) -1) / 2 < buffer_max) {
				free(buffer);
				throw ENOMEM;
			}
		}
	} while (result == NULL);

	string retval(buffer);
	free(buffer);
	return retval;
}

string get_directory(const char *directory) {
	string dirstring;

	if (directory == NULL) {
		dirstring = get_working_directory();
	} else {
		struct stat dir_info;
		dirstring = directory;
		if (stat(directory, &dir_info) < 0 || !S_ISDIR(dir_info.st_mode)) {
			size_t idx = dirstring.rfind('/');
			if (idx == string::npos) {
				dirstring = get_working_directory();
			} else {
				dirstring.erase(idx);
				if (stat(dirstring.c_str(), &dir_info) < 0)
					dirstring = get_working_directory();
			}
		}
	}
	return dirstring;
}

bool is_dir(const string *current_dir, const char *name) {
	struct stat file_info;
	string file;

	if (name[0] != '/') {
		file += *current_dir;
		file += '/';
	}
	file += name;

	if (stat(file.c_str(), &file_info) < 0)
		//This would be weird, but still we have to do something
		return false;
	return !!S_ISDIR(file_info.st_mode);
}

void lang_codeset_init(bool init) {
	if (init) {
		transcript_error_t error;
		const char *codeset = transcript_get_codeset();

		if (lang_codeset_handle == NULL) {
			lang_codeset_handle = transcript_open_converter(codeset,
				TRANSCRIPT_UTF8, TRANSCRIPT_ALLOW_FALLBACK | TRANSCRIPT_SUBST_UNASSIGNED | TRANSCRIPT_SUBST_ILLEGAL, &error);
			if (transcript_equal("UTF-8", codeset))
				lang_codeset_is_utf8 = true;
		}
	} else {
		if (lang_codeset_handle != NULL)
			transcript_close_converter(lang_codeset_handle);
	}
}

void convert_lang_codeset(const char *str, size_t len, std::string *result, bool from) {
	char output_buffer[1024], *output_buffer_ptr;
	const char *str_ptr = str;
	transcript_error_t conversion_result;
	transcript_error_t (*convert)(transcript_t *, const char **, const char *, char **, const char *, int) =
		from ? transcript_to_unicode : transcript_from_unicode;

	result->clear();
	if (!from && lang_codeset_is_utf8) {
		result->append(str, len);
		return;
	}

	while (true) {
		output_buffer_ptr = output_buffer;

		conversion_result = convert(lang_codeset_handle, &str_ptr, str + len, &output_buffer_ptr, output_buffer + sizeof(output_buffer),
				str_ptr == str ? TRANSCRIPT_FILE_START | TRANSCRIPT_END_OF_TEXT : TRANSCRIPT_END_OF_TEXT);
		result->append(output_buffer, output_buffer_ptr - output_buffer);
		if (conversion_result != TRANSCRIPT_NO_SPACE)
			return;
	}
}

void convert_lang_codeset(const char *str, std::string *result, bool from) {
	convert_lang_codeset(str, strlen(str), result, from);
}

void convert_lang_codeset(const std::string *str, std::string *result, bool from) {
	convert_lang_codeset(str->c_str(), str->size(), result, from);
}

int get_class(const string *str, int pos) {
	size_t data_len = str->size() - pos;
	uint32_t c = t3_utf8_get(str->data() + pos, &data_len);

	if (uc_is_property_id_continue(c))
		return CLASS_ALNUM;
	if (!uc_is_general_category_withtable(c, T3_UTF8_CONTROL_MASK | UC_CATEGORY_MASK_Zs))
		return CLASS_GRAPH;
	if (c == '\t' || uc_is_general_category_withtable(c, UC_CATEGORY_MASK_Zs))
		return CLASS_WHITESPACE;
	return CLASS_OTHER;
}

}; // namespace
