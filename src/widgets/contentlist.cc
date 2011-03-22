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
namespace t3_widget {

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

bool file_name_list_t::compare_entries(file_name_entry_t first, file_name_entry_t second) {
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



size_t file_name_list_t::size(void) const {
	return files.size();
}

const string *file_name_list_t::operator[](size_t idx) const {
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

	sort(files.begin(), files.end(), compare_entries);
	content_changed();
}

file_name_list_t &file_name_list_t::operator=(const file_name_list_t& other) {
	if (&other == this)
		return *this;

	files.resize(other.files.size());
	copy(other.files.begin(), other.files.end(), files.begin());
	return *this;
}

bool string_compare_filter(string_list_t *list, size_t idx, const string *str) {
	return (*list)[idx]->compare(0, str->size(), *str, 0, str->size()) == 0;
}

bool glob_filter(file_list_t *list, size_t idx, const std::string *str, bool show_hidden) {
	const string *item_name = (*list)[idx];
	if (item_name->compare("..") == 0)
		return true;

	if (!show_hidden && (*item_name)[0] == '.')
		return false;

	if (!list->is_dir(idx) && fnmatch(str->c_str(), item_name->c_str(), 0) != 0)
		return false;
	return true;
}

}; // namespace
