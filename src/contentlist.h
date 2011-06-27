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

#include <t3widget/widget_api.h>
struct transcript_t;

namespace t3_widget {

/** Abstract base class for string and file lists and filtered lists. */
class T3_WIDGET_API string_list_base_t {
	public:
		virtual ~string_list_base_t(void) {}
		/** Retrieve the size of the list. */
		virtual size_t size(void) const = 0;
		/** Retrieve element @p idx. */
		virtual const std::string *operator[](size_t idx) const = 0;
};

/** Abstract base class for string and file lists, but not for filtered lists.

    Because filtered lists need to provide the same basic members as string/file
    lists, i.e. @c size and @c operator[], they have to derive from
    string_list_base_t. However, to prevent them from having multiple
    instances of the @c content_changed signal, this is added in this base
    class. In other cases using virtual inheritance would have solved this
    problem, but because of the templating below the distance to the base class
    will not always be the same.
*/
class T3_WIDGET_API string_list_t : public virtual string_list_base_t {
	/** @fn sigc::connection connect_content_changed(const sigc::slot<void> &_slot)
	    Connect to signal emitted when the content of the list changed.*/
	/** Signal emitted when the content of the list changed. */
	T3_WIDGET_SIGNAL(content_changed, void);
};

/** Abstract base class for file lists. */
class T3_WIDGET_API file_list_t : public string_list_t {
	public:
		/** Get the file-system name for a particular @p idx.

		    The file-system name is the name of the file as it is written in the
		    file system. This is opposed to the name retrieved by @c operator[]
		    which has been converted to UTF-8. */
		virtual const std::string *get_fs_name(size_t idx) const = 0;
		/** Retrieve whether the file at index @p idx in the list is a directory. */
		virtual bool is_dir(size_t idx) const = 0;
};

/** Implementation of the file_list_t interface. */
class T3_WIDGET_API file_name_list_t : public file_list_t {
	protected:
		/** Class representing a single file. */
		class file_name_entry_t {
			public:
				std::string name, /**< The name of the file as written on disk. */
					utf8_name, /**< The name of the file converted to UTF-8 (or empty if the same as #name). */
					file_name_entry_t::*display_name; /**< Pointer to member to the name to use for dispay purposes. */
				bool is_dir; /**< Boolean indicating whether this name represents a directory. */
				/** Make a new file_name_entry_t. Implemented specifically to allow use in std::vector<file_name_entry_t>. */
				file_name_entry_t(void);
				/** Make a new file_name_entry_t. */
				file_name_entry_t(const char *_name, const std::string &_utf8_name, bool _is_dir);
				/** Construct a copy of an existing file_name_entry_t. */
				file_name_entry_t(const file_name_entry_t &other);
		};

		/** Check if two file_name_entry_t are the same. */
		static bool compare_entries(file_name_entry_t first, file_name_entry_t second);

		/** Vector holding a list of all the files in a directory. */
		std::vector<file_name_entry_t> files;

	public:
		virtual size_t size(void) const;
		virtual const std::string *operator[](size_t idx) const;
		virtual const std::string *get_fs_name(size_t idx) const;
		virtual bool is_dir(size_t idx) const;
		/** Load the contents of @p dir_name into this list. */
		void load_directory(std::string *dir_name);
		/** Compare this list with @p other. */
		file_name_list_t &operator=(const file_name_list_t& other);
};

/** Abstract base class for filtered string and file lists. */
class T3_WIDGET_API filtered_list_base_t : public virtual string_list_base_t {
	public:
		/** Set the filter callback. */
		virtual void set_filter(const sigc::slot<bool, string_list_t *, size_t> &) = 0;
		/** Reset the filter. */
		virtual void reset_filter(void) = 0;
};

/** Partial implementation of the filtered list. */
template <class list_t>
class T3_WIDGET_API filtered_list_internal_t : public list_t, public filtered_list_base_t {
	protected:
		/** Vector holding the indices in the base list of the items included in the filtered list. */
		std::vector<size_t> items;
		/** Base list of which this is a filtered view. */
		list_t *base;
		/** Filter function. */
		sigc::slot<bool, list_t *, size_t> test;

		/** Update the filtered list.
			Called automatically when the base list changes, through the use of the
		    @c content_changed signal.
		*/
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
		/** Make a new filtered_list_internal_t, wrapping an existing list. */
		filtered_list_internal_t(list_t *list) : base(list) {
			base->connect_content_changed(sigc::mem_fun(this, &filtered_list_internal_t::update_list));
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

/** Generic filtered list template.

    Because the instantiation of this template is a subclass of @p list_t,
    depending on the type of @p list_t, there may be more functions that need
    to be implemented.
*/
template <class list_t>
class T3_WIDGET_API filtered_list_t : public filtered_list_internal_t<list_t> {
	public:
		typedef sigc::slot<bool, list_t *, size_t> filter_type_t;

		filtered_list_t(list_t *list) : filtered_list_internal_t<list_t>(list) {}
		using filtered_list_internal_t<list_t>::set_filter;
		virtual void set_filter(const sigc::slot<bool, string_list_t *, size_t> &_test) {
			this->test = _test;
			this->update_list();
		}
};

/** Specialized filtered list template for string_list_t.

    A typedef named #filtered_string_list_t is provided for convenience.
*/
template <>
class T3_WIDGET_API filtered_list_t<string_list_t> : public filtered_list_internal_t<string_list_t> {
	public:
		filtered_list_t(string_list_t *list) : filtered_list_internal_t<string_list_t>(list) {}
};

/** Special name for filtered string lists. */
typedef filtered_list_t<string_list_t> filtered_string_list_t;

/** Filted file list implementation. */
class T3_WIDGET_API filtered_file_list_t : public filtered_list_t<file_list_t> {
	public:
		filtered_file_list_t(file_list_t *list) : filtered_list_t<file_list_t>(list) {}
		virtual const std::string *get_fs_name(size_t idx) const { return base->get_fs_name(test.empty() ? idx : items[idx]); }
		virtual bool is_dir(size_t idx) const { return base->is_dir(test.empty() ? idx : items[idx]); }
};

/** Filter function comparing the initial part of an entry with @p str. */
T3_WIDGET_API bool string_compare_filter(string_list_t *list, size_t idx, const std::string *str);
/** Filter function using glob on the fs_name of a file entry. */
T3_WIDGET_API bool glob_filter(file_list_t *list, size_t idx, const std::string *str, bool show_hidden);

}; // namespace
#endif
