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

struct transcript_t;

namespace t3_widget {

class string_list_base_t {
	public:
		virtual ~string_list_base_t(void) {}
		virtual size_t size(void) const = 0;
		virtual const std::string *operator[](size_t idx) const = 0;
};

class string_list_t : public virtual string_list_base_t {
	T3_WIDGET_SIGNAL(content_changed, void);
};

class file_list_t : public string_list_t {
	public:
		virtual const std::string *get_fs_name(size_t idx) const = 0;
		virtual bool is_dir(size_t idx) const = 0;
};

class file_name_list_t : public file_list_t {
	protected:
		class file_name_entry_t {
			public:
			std::string name, utf8_name, file_name_entry_t::*display_name;
			bool is_dir;
			file_name_entry_t(void);
			file_name_entry_t(const char *_name, const std::string &_utf8_name, bool _is_dir);
			file_name_entry_t(const file_name_entry_t &other);
		};

		static bool compare_entries(file_name_entry_t first, file_name_entry_t second);

		std::vector<file_name_entry_t> files;

	public:
		virtual size_t size(void) const;
		virtual const std::string *operator[](size_t idx) const;
		virtual const std::string *get_fs_name(size_t idx) const;
		virtual bool is_dir(size_t idx) const;
		void load_directory(std::string *dirName);
		file_name_list_t &operator=(const file_name_list_t& other);
};

class filtered_list_base_t : public virtual string_list_base_t {
	public:
		virtual void set_filter(const sigc::slot<bool, string_list_t *, size_t> &) = 0;
		virtual void reset_filter(void) = 0;
};

template <class list_t>
class filtered_list_internal : public list_t, public filtered_list_base_t {
	protected:
		std::vector<size_t> items;
		list_t *base;
		sigc::slot<bool, list_t *, size_t> test;

		void update_list(void) {
			if (test.empty())
				return;

			items.clear();

			for (size_t i = 0; i < base->size(); i++) {
				if (test(base, i))
					items.push_back(i);
			}
			items.reserve(items.size());
			list_t::content_changed();
		}

	public:
		filtered_list_internal(list_t *list) : base(list) {
			base->connect_content_changed(sigc::mem_fun(this, &filtered_list_internal::update_list));
		}
		virtual void set_filter(const sigc::slot<bool, list_t *, size_t> &_test) {
			test = _test;
			update_list();
		}
		virtual void reset_filter(void) {
			items.clear();
			test.disconnect();
			list_t::content_changed();
		}
		virtual size_t size(void) const { return test.empty() ? base->size() : items.size(); }
		virtual const std::string *operator[](size_t idx) const { return (*base)[test.empty() ? idx : items[idx]]; }
};

template <class list_t>
class filtered_list : public filtered_list_internal<list_t> {
	public:
		typedef sigc::slot<bool, list_t *, size_t> filter_type_t;

		filtered_list(list_t *list) : filtered_list_internal<list_t>(list) {}
		using filtered_list_internal<list_t>::set_filter;
		virtual void set_filter(const sigc::slot<bool, string_list_t *, size_t> &_test) {
			this->test = _test;
			this->update_list();
		}
};

template <>
class filtered_list<string_list_t> : public filtered_list_internal<string_list_t> {
	public:
		filtered_list(string_list_t *list) : filtered_list_internal<string_list_t>(list) {}
};

typedef filtered_list<string_list_t> filtered_string_list_t;

class filtered_file_list_t : public filtered_list<file_list_t> {
	public:
		filtered_file_list_t(file_list_t *list) : filtered_list<file_list_t>(list) {}
		virtual const std::string *get_fs_name(size_t idx) const { return base->get_fs_name(test.empty() ? idx : items[idx]); }
		virtual bool is_dir(size_t idx) const { return base->is_dir(test.empty() ? idx : items[idx]); }
};

bool string_compare_filter(string_list_t *list, size_t idx, const std::string *str);
bool glob_filter(file_list_t *list, size_t idx, const std::string *str, bool show_hidden);

}; // namespace
#endif
