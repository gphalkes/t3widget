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
#ifndef T3_WIDGET_CONTENTLIST_H
#define T3_WIDGET_CONTENTLIST_H

#include <string>
#include <vector>

namespace t3_widget {

class string_list_t {
	public:
		virtual size_t size(void) const = 0;
		virtual const std::string *operator[](size_t idx) const = 0;

	T3_WIDET_SIGNAL(content_changed, void);
};

class file_list_t : public string_list_t {
	public:
		virtual bool is_dir(int idx) const = 0;
};

class file_name_list_t : public file_list_t {
	private:
		struct file_name_entry_t {
			std::string name;
			bool is_dir;
			file_name_entry_t(void) : is_dir(false) {}
			file_name_entry_t(const char *_name, bool _isDir) : name(_name), is_dir(_isDir) {}
		};

		std::vector<file_name_entry_t> files;

		static bool compare_entries(file_name_entry_t first, file_name_entry_t second);

	public:
		virtual size_t size(void) const;
		virtual const std::string *operator[](size_t idx) const;
		virtual bool is_dir(int idx) const;
		void load_directory(std::string *dirName);
		size_t get_index(const std::string *name) const;
		file_name_list_t &operator=(const file_name_list_t& a);

		static std::string get_working_directory(void);
		static std::string get_directory(const char *directory);
		static bool is_dir(const std::string *current_dir, const char *name);

		class file_name_list_view_t : public file_list_t {
			private:
				file_name_list_t *base;
				std::vector<size_t> items;
			public:
				file_name_list_view_t(file_name_list_t *_base, bool showHidden, const std::string *filter);
				file_name_list_view_t(file_name_list_t *_base, const std::string *start);
				virtual size_t size(void) const;
				virtual const std::string *operator[](size_t idx) const;
				virtual bool is_dir(int idx) const;
				size_t get_index(const std::string *name) const;
		};
};

}; // namespace
#endif
