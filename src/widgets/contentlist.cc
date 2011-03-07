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
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <algorithm>
#include <cstdarg>
#include <stdint.h>

#include "util.h"
#include "widgets/contentlist.h"

using namespace std;

string file_name_list_t::get_working_directory(void) {
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

			if (SIZE_MAX / 2 < buffer_max) {
				free(buffer);
				throw ENOMEM;
			}
		}
	} while (result == NULL);

	string retval(buffer);
	free(buffer);
	return retval;
}

string file_name_list_t::get_directory(const char *directory) {
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
					throw errno;
			}
		}
	}
	return dirstring;
}

bool file_name_list_t::is_dir(const string *current_dir, const char *name) {
	string file = *current_dir + "/" + name;
	struct stat file_info;

	if (stat(file.c_str(), &file_info) < 0)
		//This would be weird, but still we have to do something
		return false;
	return !!S_ISDIR(file_info.st_mode);
}

bool file_name_list_t::sort_entries(file_name_entry_t first, file_name_entry_t second) {
	if (first.is_dir && !second.is_dir)
		return true;

	if (!first.is_dir && second.is_dir)
		return false;

	if (first.is_dir && first.name.compare("..") == 0)
		return true;

	if (second.is_dir && second.name.compare("..") == 0)
		return false;

	if (first.name[0] == '.' && second.name[0] != '.')
		return true;

	if (first.name[0] != '.' && second.name[0] == '.')
		return false;

	return strcoll(first.name.c_str(), second.name.c_str()) < 0;
}



size_t file_name_list_t::get_length(void) const {
	return files.size();
}

const string *file_name_list_t::get_name(int idx) const {
	return &files[idx].name;
}

bool file_name_list_t::is_dir(int idx) const {
	return files[idx].is_dir;
}

void file_name_list_t::load_directory(string *dirName) {
	struct dirent *entry;
	DIR *dir;

	files.clear();

	if ((dir = opendir(dirName->c_str())) == NULL) {
		content_changed();
		throw errno;
	}

	// Make sure errno is clear on EOF
	errno = 0;
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || (dirName->compare("/") == 0 && strcmp(entry->d_name, "..") == 0))
			continue;
		files.push_back(file_name_entry_t(entry->d_name, is_dir(dirName, entry->d_name)));
		// Make sure errno is clear on EOF
		errno = 0;
	}

	if (errno != 0) {
		int error = errno;
		closedir(dir);
		content_changed();
		throw error;
	}
	closedir(dir);

	sort(files.begin(), files.end(), sort_entries);
	content_changed();
}

size_t file_name_list_t::get_index(const string *name) const {
	size_t i;

	if (name->size() == 0)
		return 0;
	for (i = 0; i < files.size(); i++)
		if (name->compare(files[i].name) == 0)
			return i;
	return 0;
}

file_name_list_t &file_name_list_t::operator=(const file_name_list_t& a) {
	files.resize(a.files.size());
	copy(a.files.begin(), a.files.end(), files.begin());
	return *this;
}

file_name_list_t::file_name_list_view_t::file_name_list_view_t(file_name_list_t *_base, bool showHidden, const string *filter) : base(_base) {
	size_t i;

	if (base->get_length() == 0)
		return;

	for (i = 0; i < base->get_length(); i++) {
		const string *base_name = base->get_name(i);
		if (base_name->compare("..") == 0) {
			items.push_back(i);
			continue;
		}
		if (!showHidden && (*base_name)[0] == '.')
			continue;
		if (!base->is_dir(i) && fnmatch(filter->c_str(), base_name->c_str(), 0) != 0)
			continue;
		items.push_back(i);
	}
}

file_name_list_t::file_name_list_view_t::file_name_list_view_t(file_name_list_t *_base, const string *start) : base(_base) {
	size_t i;

	for (i = 0; i < base->get_length(); i++) {
		const string *base_name = base->get_name(i);
		if (base_name->compare(0, start->size(), *start, 0, start->size()) == 0)
			items.push_back(i);
	}
}

size_t file_name_list_t::file_name_list_view_t::get_length(void) const {
	return items.size();
}

const string *file_name_list_t::file_name_list_view_t::get_name(int idx) const {
	return base->get_name(items[idx]);
}

bool file_name_list_t::file_name_list_view_t::is_dir(int idx) const {
	return base->is_dir(items[idx]);
}

size_t file_name_list_t::file_name_list_view_t::get_index(const string *name) const {
	size_t i;

	if (name == NULL || name->size() == 0)
		return 0;
	for (i = 0; i < items.size(); i++)
		if (name->compare(*base->get_name(items[i])) == 0)
			return i;
	return 0;
}


multi_string_list_t::multi_string_list_t(int _columns) : columns(_columns), data(new vector<const string *>[columns]) {}

size_t multi_string_list_t::get_length(void) const { return data[0].size(); }
int multi_string_list_t::get_columns(void) const { return columns; }
const string *multi_string_list_t::get_item(int idx, int column) const { return data[column][idx]; }

void multi_string_list_t::add_line(const string *first, ...) {
	va_list args;
	int i;

	data[0].push_back(first);
	va_start(args, first);
	for (i = 1; i < columns; i++)
		data[i].push_back(va_arg(args, const string *));
	va_end(args);
}
