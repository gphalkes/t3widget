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
#include <fnmatch.h>
#include <algorithm>
#include <cstdarg>
#include <stdint.h>

#include "util.h"
#include "widgets/contentlist.h"

using namespace std;
namespace t3_widget {

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
		files.push_back(file_name_entry_t(entry->d_name, t3_widget::is_dir(dirName, entry->d_name)));
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

	//FIXME: fnmatch discards strings which are not valid in the current encoding
	if (!list->is_dir(idx) && fnmatch(str->c_str(), item_name->c_str(), 0) != 0)
		return false;
	return true;
}

}; // namespace
