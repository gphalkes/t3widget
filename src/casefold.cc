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
#include <cstring>
#include <fcntl.h>
#include <algorithm>

#include "casefold.h"
#include "filewrapper.h"
#include "lines.h"
#include "util.h"

case_fold_t::replacements_t case_fold_t::replacements;

//FIXME: int is not necessarily large enough!


case_fold_t::replacement_t::replacement_t(int _base) {
	convertToUTF8(_base, base);
}

void case_fold_t::replacement_t::append(int _replacement) {
	char buffer[5];
	convertToUTF8(_replacement, buffer);
	buffer[4] = 0;
	replacement += buffer;
}

bool case_fold_t::replacement_t::operator<(const replacement_t &other) const {
	return memcmp(base, other.base, 4) < 0;
}

bool case_fold_t::replacement_t::operator<(const char *other) const {
	return memcmp(base, other, 4) < 0;
}

//FIXME: check whether errno numbers are greater or smaller than 0
int case_fold_t::init(const char *fileName) {
	FileReadWrapper *reader;
	string *line = NULL;
	int fd;

	if ((fd = open(fileName, O_RDONLY)) < 0)
		return errno;

	reader = new FileReadWrapper(fd);
	try {
		size_t text_start;
		line = reader->readLine();
		text_start = line->find_first_not_of(" \t");
		if (text_start == string::npos || (*line)[text_start] != '#' || line->find("CaseFolding-") == string::npos)
			throw -1;
		delete line;
		line = NULL;

		for (; (line = reader->readLine()) != NULL; delete line) {
			int base, matched;
			char replacement_string[1024];
			char type, semicolon;
			int replacement_string_start;

			text_start = line->find_first_not_of(" \t\r");

			if (text_start == string::npos || (*line)[text_start] == '#')
				continue;

			if (sscanf(line->c_str(), " %6x ; %c ; %1023[0-9A-Fa-f \t] %c %n", &base, &type, replacement_string, &semicolon, &matched) < 4)
				throw -1;

			if (semicolon != ';')
				throw -1;

			if ((size_t) matched < line->size() && (*line)[matched] != '#' && (*line)[matched] != '\r')
				throw -1;

			if (type == 'T' || type == 'S')
				continue;

			if (type != 'C' && type != 'F')
				throw -1;

			replacements.push_back(replacement_t(base));
			replacement_string_start = 0;
			do {
				int replacemen_char, after, next;
				if (sscanf(replacement_string + replacement_string_start, " %6x%n %n", &replacemen_char, &after, &next) < 1)
					throw -1;
				if (replacement_string[replacement_string_start + after] != ' ' && replacement_string[replacement_string_start + after] != 0)
					throw -1;
				replacements.back().append(replacemen_char);
				replacement_string_start += next;
			} while (replacement_string[replacement_string_start] != 0);
		}
	} catch (int error) {
		if (error != -1)
			delete line;
		delete reader;
		close(fd);
		return error;
	}

	delete reader;
	close(fd);
	sort(replacements.begin(), replacements.end());

	return 0;
}

line_t *case_fold_t::fold(const line_t *line, int *pos) {
	line_t *result = new line_t();
	size_t idx, i, bytes, size = line->get_length();
	const char *data = line->buffer.data();
	const char *meta_data = line->meta_buffer.data();
	replacements_t::iterator iter;
	char utf8[4];
	int old_pos = *pos;

	for (idx = 0; idx < size; idx += bytes) {
		if ((size_t) old_pos == idx)
			*pos = result->get_length();

		memset(utf8, 0, 4);
		if ((unsigned char) data[idx] < 0x7f)
			bytes = 1;
		else if ((unsigned char) data[idx] < 0xdf)
			bytes = 2;
		else if ((unsigned char) data[idx] < 0xf7)
			bytes = 3;
		else
			bytes = 4;
		// Should not happen, because we should only have valid UTF-8 strings inside the program
		if (idx + bytes > size)
			break;
		memcpy(utf8, data + idx, bytes);
		if ((iter = lower_bound(replacements.begin(), replacements.end(), utf8)) == replacements.end() || memcmp(utf8, iter->base, 4) != 0) {
			result->buffer.append(utf8, bytes);
			result->meta_buffer.append(meta_data + idx, 1);
			for (i = 1; i < bytes; i++)
				result->meta_buffer.append(1, '\0');
		} else {
			result->buffer.append(iter->replacement);
			result->meta_buffer.append(meta_data + idx, 1);
			for (i = 1; i < iter->replacement.size(); i++)
				result->meta_buffer.append(1, '\0');
		}
	}
	// Should not happen, because we should only have valid UTF-8 strings inside the program
	if (idx < size) {
		result->buffer.append(data + idx, size - idx);
		result->meta_buffer.append(meta_data + idx, size - idx);
	}
	return result;
}

string *case_fold_t::fold(const string *line, string *result) {
	const char *data = line->data();
	size_t idx, bytes, size = line->size();
	replacements_t::iterator iter;
	char utf8[4];

	if (result == NULL)
		result = new string;

	for (idx = 0; idx < size; idx += bytes) {
		memset(utf8, 0, 4);
		if ((unsigned char) data[idx] < 0x7f)
			bytes = 1;
		else if ((unsigned char) data[idx] < 0xdf)
			bytes = 2;
		else if ((unsigned char) data[idx] < 0xf7)
			bytes = 3;
		else
			bytes = 4;
		// Should not happen, because we should only have valid UTF-8 strings inside the program
		if (idx + bytes > size)
			break;
		memcpy(utf8, data + idx, bytes);
		if ((iter = lower_bound(replacements.begin(), replacements.end(), utf8)) == replacements.end() || memcmp(utf8, iter->base, 4) != 0)
			result->append(utf8, bytes);
		else
			result->append(iter->replacement);
	}
	// Should not happen, because we should only have valid UTF-8 strings inside the program
	if (idx < size)
		result->append(data + idx, size - idx);
	return result;
}

#if 0
int main(int argc, char *argv[]) {
	if (case_fold_t::init("/usr/share/unicode/CaseFolding.txt") == 0) {
		string test = "AbcdË";
		string *folded = case_fold_t::fold(&test);
		printf("%s -> %s\n", test.c_str(), folded->c_str());
	} else {
		printf("Error loading case folding table\n");
	}
	return 0;
}
#endif
