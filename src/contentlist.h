/* Copyright (C) 2011-2012,2018 G.P. Halkes
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

#include <cstddef>
#include <iterator>
#include <string>
#include <t3widget/signals.h>
#include <t3widget/string_view.h>
#include <t3widget/util.h>
#include <t3widget/widget_api.h>
#include <vector>

struct transcript_t;

namespace t3widget {

class T3_WIDGET_API const_string_list_iterator_t {
 public:
  class T3_WIDGET_LOCAL adapter_base_t;

  using difference_type = std::ptrdiff_t;
  using value_type = const std::string;
  using pointer = const std::string *;
  using reference = const std::string &;
  using iterator_category = std::forward_iterator_tag;

  const_string_list_iterator_t(std::unique_ptr<adapter_base_t> impl);
  const_string_list_iterator_t(const const_string_list_iterator_t &other);
  const_string_list_iterator_t(const_string_list_iterator_t &&other);

  ~const_string_list_iterator_t();

  const_string_list_iterator_t &operator=(const const_string_list_iterator_t &other);
  const_string_list_iterator_t &operator=(const_string_list_iterator_t &&other);

  const_string_list_iterator_t &operator++();
  const_string_list_iterator_t operator++(int);

  const std::string &operator*() const;
  const std::string *operator->() const;
  bool operator==(const const_string_list_iterator_t &other) const;
  bool operator!=(const const_string_list_iterator_t &other) const;

 private:
  std::unique_ptr<adapter_base_t> impl_;
};

/** Abstract base class for string and file lists and filtered lists. */
class T3_WIDGET_API string_list_base_t {
 public:
  virtual ~string_list_base_t() {}
  /** Retrieve the size of the list. */
  virtual size_t size() const = 0;
  /** Retrieve element @p idx. */
  virtual const std::string &operator[](size_t idx) const = 0;

  virtual connection_t connect_content_changed(std::function<void()> cb) = 0;

  virtual const_string_list_iterator_t begin() const = 0;
  virtual const_string_list_iterator_t end() const = 0;
};

/** Abstract base class for file lists. */
class T3_WIDGET_API file_list_base_t : virtual public string_list_base_t {
 public:
  /** Get the file-system name for a particular @p idx.

      The file-system name is the name of the file as it is written in the
      file system. This is opposed to the name retrieved by @c operator[]
      which has been converted to UTF-8. */
  virtual const std::string &get_fs_name(size_t idx) const = 0;
  /** Retrieve whether the file at index @p idx in the list is a directory. */
  virtual bool is_dir(size_t idx) const = 0;
};

/** Abstract base class for filtered string and file lists. */
class T3_WIDGET_API filtered_string_list_base_t : public virtual string_list_base_t {
 public:
  /** Set the filter callback.
      The filter should return @c true if the item at the index indicated in the second parameter
      should be retained in the list. */
  virtual void set_filter(std::function<bool(const string_list_base_t &, size_t)>) = 0;
  /** Reset the filter. */
  virtual void reset_filter() = 0;
};

/** Abstract base class for filtered file lists. */
class T3_WIDGET_API filtered_file_list_base_t : public file_list_base_t,
                                                public filtered_string_list_base_t {};

//=================================== Implementations ==============================================

/** Abstract class for a string list that allows modification. */
class T3_WIDGET_API string_list_t : public string_list_base_t {
 public:
  string_list_t();
  ~string_list_t() override;
  size_t size() const override;
  void push_back(std::string str);
  const std::string &operator[](size_t idx) const override;

  connection_t connect_content_changed(std::function<void()> cb) override;

  const_string_list_iterator_t begin() const override;
  const_string_list_iterator_t end() const override;

 private:
  struct T3_WIDGET_LOCAL implementation_t;
  pimpl_t<implementation_t> impl;

  class T3_WIDGET_LOCAL iterator_adapter_t;
};

/** Implementation of the file_list_base_t interface. */
class T3_WIDGET_API file_list_t : public file_list_base_t {
 public:
  file_list_t();
  ~file_list_t() override;
  size_t size() const override;
  const std::string &operator[](size_t idx) const override;
  const std::string &get_fs_name(size_t idx) const override;
  bool is_dir(size_t idx) const override;
  /** Load the contents of @p dir_name into this list. */
  int load_directory(const std::string &dir_name);
  /** Compare this list with @p other. */
  file_list_t &operator=(const file_list_t &other);

  connection_t connect_content_changed(std::function<void()> cb) override;

  const_string_list_iterator_t begin() const override;
  const_string_list_iterator_t end() const override;

 private:
  struct T3_WIDGET_LOCAL implementation_t;
  pimpl_t<implementation_t> impl;

  class T3_WIDGET_LOCAL iterator_adapter_t;
};

std::unique_ptr<filtered_string_list_base_t> new_filtered_string_list(string_list_base_t *list);
std::unique_ptr<filtered_file_list_base_t> new_filtered_file_list(file_list_base_t *list);

/** Filter function comparing the initial part of an entry with @p str. */
/* This uses a pointer and not a reference for str, because it is intended to be used with
   bind_front. Using a reference would make a copy of the string, which is not intended. */
T3_WIDGET_API bool string_compare_filter(const std::string *str, const string_list_base_t &list,
                                         size_t idx);
/** Filter function using glob on the fs_name of a file entry. */
/* This uses a pointer and not a reference for str, because it is intended to be used with
   bind_front. Using a reference would make a copy of the string, which is not intended. */
T3_WIDGET_API bool glob_filter(const std::string *str, bool show_hidden,
                               const string_list_base_t &list, size_t idx);

}  // namespace t3widget
#endif
