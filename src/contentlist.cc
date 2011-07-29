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
#include "contentlist.h"
#include "main.h"

using namespace std;
namespace t3_widget {

file_name_list_t::file_name_entry_t::file_name_entry_t(void) : is_dir(false) {
	display_name = &file_name_entry_t::name;
}

file_name_list_t::file_name_entry_t::file_name_entry_t(const char *_name, const std::string &_utf8_name, bool _is_dir) :
		name(_name), utf8_name(_utf8_name), is_dir(_is_dir)
{
	display_name = utf8_name.size() == 0 ? &file_name_entry_t::name : &file_name_entry_t::utf8_name;
}

/* We need to supply a copy constructor, or else we risk a memory leak from the
   fact that a simple bit copy may destroy a reference to memory allocated by a
   std::string. */
file_name_list_t::file_name_entry_t::file_name_entry_t(const file_name_entry_t &other) {
	name = other.name;
	utf8_name = other.utf8_name;
	is_dir = other.is_dir;
	display_name = other.display_name;
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

	/* Use strcoll on the FS names. This will sort them as the user expects,
	   provided the locale is set correctly. */
	return strcoll(first.name.c_str(), second.name.c_str()) < 0;
}

size_t file_name_list_t::size(void) const {
	return files.size();
}

const string *file_name_list_t::operator[](size_t idx) const {
	return &(files[idx].*(files[idx].display_name));
}

const string *file_name_list_t::get_fs_name(size_t idx) const {
	return &files[idx].name;
}

bool file_name_list_t::is_dir(size_t idx) const {
	return files[idx].is_dir;
}

void file_name_list_t::load_directory(string *dir_name) {
	struct dirent *entry;
	DIR *dir;

	files.clear();

	if ((dir = opendir(dir_name->c_str())) == NULL) {
		content_changed();
		throw errno;
	}

	// Make sure errno is clear on EOF
	errno = 0;
	while ((entry = readdir(dir)) != NULL) {
		string utf8_name;

		if (strcmp(entry->d_name, ".") == 0 || (dir_name->compare("/") == 0 && strcmp(entry->d_name, "..") == 0))
			continue;

		convert_lang_codeset(entry->d_name, &utf8_name, true);
		if (strcmp(entry->d_name, utf8_name.c_str()) == 0)
			utf8_name.clear();
		files.push_back(file_name_entry_t(entry->d_name, utf8_name, t3_widget::is_dir(dir_name, entry->d_name)));

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
	content_changed();
	return *this;
}

bool string_compare_filter(string_list_t *list, size_t idx, const string *str) {
	return (*list)[idx]->compare(0, str->size(), *str, 0, str->size()) == 0;
}

bool glob_filter(file_list_t *list, size_t idx, const std::string *str, bool show_hidden) {
	const string *item_name = (*list)[idx];
	string fs_name;

	if (item_name->compare("..") == 0)
		return true;

	if (!show_hidden && (*item_name)[0] == '.')
		return false;

	/* fnmatch discards strings with characters that are invalid in the locale
	   codeset. However, we do want to use fnmatch because it also involves
	   collation which is too complicated to handle ourselves. So we convert the
	   file names to the locale codeset, and use fnmatch on those. Note that the
	   filter string passed to this function is already in the locale codeset. */
	convert_lang_codeset(item_name, &fs_name, false);
	if (!list->is_dir(idx) && fnmatch(str->c_str(), fs_name.c_str(), 0) != 0)
		return false;
	return true;
}

}; // namespace
