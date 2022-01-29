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
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fnmatch.h>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "t3widget/contentlist.h"
#include "t3widget/internal.h"
#include "t3widget/signals.h"
#include "t3widget/util.h"
#include "widget_api.h"

namespace t3widget {

//===================================== string_list_iterator_t =====================================

class const_string_list_iterator_t::adapter_base_t {
 public:
  virtual ~adapter_base_t() = default;
  virtual void operator++() = 0;
  virtual const std::string &operator*() const = 0;
  virtual bool operator==(const adapter_base_t &other) const = 0;
  virtual std::unique_ptr<adapter_base_t> clone() const = 0;
};

const_string_list_iterator_t::const_string_list_iterator_t(std::unique_ptr<adapter_base_t> impl)
    : impl_(std::move(impl)) {}
const_string_list_iterator_t::const_string_list_iterator_t(
    const const_string_list_iterator_t &other)
    : impl_(other.impl_->clone()) {}
const_string_list_iterator_t::const_string_list_iterator_t(const_string_list_iterator_t &&other)
    : impl_(std::move(other.impl_)) {}

const_string_list_iterator_t::~const_string_list_iterator_t() {}

const_string_list_iterator_t &const_string_list_iterator_t::operator=(
    const const_string_list_iterator_t &other) {
  impl_ = other.impl_->clone();
  return *this;
}

const_string_list_iterator_t &const_string_list_iterator_t::operator=(
    const_string_list_iterator_t &&other) {
  impl_ = std::move(other.impl_);
  return *this;
}

const_string_list_iterator_t &const_string_list_iterator_t::operator++() {
  ++*impl_;
  return *this;
}

const_string_list_iterator_t const_string_list_iterator_t::operator++(int) {
  const_string_list_iterator_t result = *this;
  ++*impl_;
  return result;
}

const std::string &const_string_list_iterator_t::operator*() const { return **impl_; }
const std::string *const_string_list_iterator_t::operator->() const { return &**impl_; }

bool const_string_list_iterator_t::operator==(const const_string_list_iterator_t &other) const {
  return *impl_ == *other.impl_;
}

bool const_string_list_iterator_t::operator!=(const const_string_list_iterator_t &other) const {
  return !(*impl_ == *other.impl_);
}

//===================================== string_list_t ==============================================
struct string_list_t::implementation_t {
  std::vector<std::string> strings;
  signal_t<> content_changed;
};

class string_list_t::iterator_adapter_t : public const_string_list_iterator_t::adapter_base_t {
 public:
  explicit iterator_adapter_t(std::vector<std::string>::const_iterator iter)
      : iter_(std::move(iter)) {}
  void operator++() override { ++iter_; }
  const std::string &operator*() const override { return *iter_; }
  bool operator==(const adapter_base_t &other) const override {
    const iterator_adapter_t *other_ptr = dynamic_cast<const iterator_adapter_t *>(&other);
    if (!other_ptr) {
      return false;
    }
    return iter_ == other_ptr->iter_;
  }
  std::unique_ptr<adapter_base_t> clone() const override {
    return t3widget::make_unique<iterator_adapter_t>(iter_);
  }

 private:
  std::vector<std::string>::const_iterator iter_;
};

string_list_t::string_list_t() : impl(new implementation_t) {}
string_list_t::~string_list_t() {}

size_t string_list_t::size() const { return impl->strings.size(); }

const std::string &string_list_t::operator[](size_t idx) const { return impl->strings[idx]; }

void string_list_t::push_back(std::string str) {
  impl->strings.push_back(str);
  impl->content_changed();
}

const_string_list_iterator_t string_list_t::begin() const {
  return const_string_list_iterator_t(
      t3widget::make_unique<iterator_adapter_t>(impl->strings.begin()));
}

const_string_list_iterator_t string_list_t::end() const {
  return const_string_list_iterator_t(
      t3widget::make_unique<iterator_adapter_t>(impl->strings.end()));
}

_T3_WIDGET_IMPL_SIGNAL(string_list_t, content_changed)

//===================================== file_name_entry_t ==========================================
/** Class representing a single file. */
class T3_WIDGET_LOCAL file_name_entry_t {
 public:
  std::string name, /**< The name of the file as written on disk. */
      utf8_name,    /**< The name of the file converted to UTF-8 (or empty if the same as #name). */
      /** Pointer to member to the name to use for dispay purposes. */
      file_name_entry_t::*display_name;
  bool is_dir; /**< Boolean indicating whether this name represents a directory. */
  /** Make a new file_name_entry_t. Implemented specifically to allow use in
      std::vector<file_name_entry_t>. */
  file_name_entry_t() : is_dir(false) { display_name = &file_name_entry_t::name; }

  /** Make a new file_name_entry_t. */
  file_name_entry_t(std::string _name, std::string _utf8_name, bool _is_dir)
      : name(_name), utf8_name(_utf8_name), is_dir(_is_dir) {
    display_name = utf8_name.empty() ? &file_name_entry_t::name : &file_name_entry_t::utf8_name;
  }

  /** Construct a copy of an existing file_name_entry_t. */
  file_name_entry_t(const file_name_entry_t &other) { *this = other; }

  file_name_entry_t operator=(const file_name_entry_t &other) {
    name = other.name;
    utf8_name = other.utf8_name;
    is_dir = other.is_dir;
    display_name = other.display_name;
    return *this;
  }
};

static bool compare_entries(file_name_entry_t first, file_name_entry_t second) {
  if (first.is_dir && !second.is_dir) {
    return true;
  }

  if (!first.is_dir && second.is_dir) {
    return false;
  }

  if (first.is_dir && first.name.compare("..") == 0) {
    return true;
  }

  if (second.is_dir && second.name.compare("..") == 0) {
    return false;
  }

  if (first.name[0] == '.' && second.name[0] != '.') {
    return true;
  }

  if (first.name[0] != '.' && second.name[0] == '.') {
    return false;
  }

  /* Use strcoll on the FS names. This will sort them as the user expects,
     provided the locale is set correctly. */
  return strcoll(first.name.c_str(), second.name.c_str()) < 0;
}

//===================================== file_list_t ===========================================

struct file_list_t::implementation_t {
  /** Vector holding a list of all the files in a directory. */
  std::vector<file_name_entry_t> files;
  signal_t<> content_changed;
};

class file_list_t::iterator_adapter_t : public const_string_list_iterator_t::adapter_base_t {
 public:
  explicit iterator_adapter_t(std::vector<file_name_entry_t>::const_iterator iter)
      : iter_(std::move(iter)) {}
  void operator++() override { ++iter_; }
  const std::string &operator*() const override { return (*iter_).*iter_->display_name; }
  bool operator==(const adapter_base_t &other) const override {
    const iterator_adapter_t *other_ptr = dynamic_cast<const iterator_adapter_t *>(&other);
    if (!other_ptr) {
      return false;
    }
    return iter_ == other_ptr->iter_;
  }
  std::unique_ptr<adapter_base_t> clone() const override {
    return t3widget::make_unique<iterator_adapter_t>(iter_);
  }

 private:
  std::vector<file_name_entry_t>::const_iterator iter_;
};

file_list_t::file_list_t() : impl(new implementation_t) {}
file_list_t::~file_list_t() {}

size_t file_list_t::size() const { return impl->files.size(); }

const std::string &file_list_t::operator[](size_t idx) const {
  return impl->files[idx].*(impl->files[idx].display_name);
}

const std::string &file_list_t::get_fs_name(size_t idx) const { return impl->files[idx].name; }

bool file_list_t::is_dir(size_t idx) const { return impl->files[idx].is_dir; }

int file_list_t::load_directory(const std::string &dir_name) {
  struct dirent *entry;
  DIR *dir;
  call_on_return_t cleanup([&] { impl->content_changed(); });

  impl->files.clear();
  if (dir_name.compare("/") != 0) {
    impl->files.push_back(file_name_entry_t("..", "..", true));
  }

  if ((dir = opendir(dir_name.c_str())) == nullptr) {
    return errno;
  }

  // Make sure errno is clear on EOF
  errno = 0;
  while ((entry = readdir(dir)) != nullptr) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    std::string utf8_name = convert_lang_codeset(entry->d_name, true);
    if (strcmp(entry->d_name, utf8_name.c_str()) == 0) {
      utf8_name.clear();
    }
    impl->files.push_back(
        file_name_entry_t(entry->d_name, utf8_name, t3widget::is_dir(dir_name, entry->d_name)));

    // Make sure errno is clear on EOF
    errno = 0;
  }

  sort(impl->files.begin(), impl->files.end(), compare_entries);

  if (errno != 0) {
    int error = errno;
    closedir(dir);
    return error;
  }
  closedir(dir);

  return 0;
}

file_list_t &file_list_t::operator=(const file_list_t &other) {
  if (&other == this) {
    return *this;
  }

  impl->files.resize(other.impl->files.size());
  copy(other.impl->files.begin(), other.impl->files.end(), impl->files.begin());
  impl->content_changed();
  return *this;
}

const_string_list_iterator_t file_list_t::begin() const {
  return const_string_list_iterator_t(
      t3widget::make_unique<iterator_adapter_t>(impl->files.begin()));
}
const_string_list_iterator_t file_list_t::end() const {
  return const_string_list_iterator_t(t3widget::make_unique<iterator_adapter_t>(impl->files.end()));
}

_T3_WIDGET_IMPL_SIGNAL(file_list_t, content_changed)

//===================================== filtered_list_internal_t ===================================

/** Partial implementation of the filtered list. */
template <typename L, typename B>
class T3_WIDGET_API filtered_list_internal_t : public B {
 protected:
  /** Vector holding the indices in the base list of the items included in the filtered list. */
  std::vector<size_t> items;
  /** Base list of which this is a filtered view. Not owned. */
  L *base;
  /** Filter function. */
  optional<std::function<bool(const string_list_base_t &, size_t)>> test;
  /** Connection to base list's content_changed signal. */
  connection_t base_content_changed_connection;
  signal_t<> content_changed;

  /** Update the filtered list.
          Called automatically when the base list changes, through the use of the
      @c content_changed signal.
  */
  void update_list() {
    if (!test.is_valid()) {
      return;
    }

    items.clear();

    const size_t base_size = base->size();
    for (size_t i = 0; i < base_size; i++) {
      if (test.value()(*base, i)) {
        items.push_back(i);
      }
    }
    items.reserve(items.size());
    content_changed();
  }

 public:
  /** Make a new filtered_list_internal_t, wrapping an existing list.
      The filtered_list_internal_t does not take ownership of the list_t. */
  filtered_list_internal_t(L *list)
      : base(list), test([](const string_list_base_t &, size_t) { return false; }) {
    base_content_changed_connection = base->connect_content_changed([this] { update_list(); });
  }
  ~filtered_list_internal_t() override { base_content_changed_connection.disconnect(); }
  void set_filter(std::function<bool(const string_list_base_t &, size_t)> _test) override {
    test = _test;
    update_list();
  }
  void reset_filter() override {
    items.clear();
    test.reset();
    content_changed();
  }
  size_t size() const override { return test.is_valid() ? items.size() : base->size(); }
  const std::string &operator[](size_t idx) const override {
    return (*base)[test.is_valid() ? items[idx] : idx];
  }

  connection_t connect_content_changed(std::function<void()> cb) override {
    return content_changed.connect(cb);
  }

  class iterator_adapter_t : public const_string_list_iterator_t::adapter_base_t {
   public:
    iterator_adapter_t(std::vector<size_t>::const_iterator iter, L *base)
        : iter_(std::move(iter)), base_(base) {}
    void operator++() override { ++iter_; }
    const std::string &operator*() const override { return (*base_)[*iter_]; }
    bool operator==(const adapter_base_t &other) const override {
      const iterator_adapter_t *other_ptr = dynamic_cast<const iterator_adapter_t *>(&other);
      if (!other_ptr) {
        return false;
      }
      return iter_ == other_ptr->iter_;
    }
    std::unique_ptr<adapter_base_t> clone() const override {
      return t3widget::make_unique<iterator_adapter_t>(iter_, base_);
    }

   private:
    std::vector<size_t>::const_iterator iter_;
    L *base_;
  };

  const_string_list_iterator_t begin() const override {
    return const_string_list_iterator_t(
        t3widget::make_unique<iterator_adapter_t>(items.begin(), base));
  }
  const_string_list_iterator_t end() const override {
    return const_string_list_iterator_t(
        t3widget::make_unique<iterator_adapter_t>(items.end(), base));
  }
};

//===================================== filtered_list_* implementations ============================

/** Specialized filtered list template for string_list_base_t.

    A typedef named #filtered_string_list_t is provided for convenience.
*/
class filtered_string_list_t
    : public filtered_list_internal_t<string_list_base_t, filtered_string_list_base_t> {
 public:
  filtered_string_list_t(string_list_base_t *list)
      : filtered_list_internal_t<string_list_base_t, filtered_string_list_base_t>(list) {}
};

/** Filted file list implementation. */
class filtered_file_list_t
    : public filtered_list_internal_t<file_list_base_t, filtered_file_list_base_t> {
 public:
  filtered_file_list_t(file_list_base_t *list)
      : filtered_list_internal_t<file_list_base_t, filtered_file_list_base_t>(list) {}
  const std::string &get_fs_name(size_t idx) const override {
    return base->get_fs_name(test.is_valid() ? items[idx] : idx);
  }
  bool is_dir(size_t idx) const override {
    return base->is_dir(test.is_valid() ? items[idx] : idx);
  }
};

std::unique_ptr<filtered_string_list_base_t> new_filtered_string_list(string_list_base_t *list) {
  return t3widget::make_unique<filtered_string_list_t>(list);
}

std::unique_ptr<filtered_file_list_base_t> new_filtered_file_list(file_list_base_t *list) {
  return t3widget::make_unique<filtered_file_list_t>(list);
}

//===================================== filters ====================================================

bool string_compare_filter(const std::string *str, const string_list_base_t &list, size_t idx) {
  return list[idx].compare(0, str->size(), *str, 0, str->size()) == 0;
}

bool glob_filter(const std::string *str, bool show_hidden, const string_list_base_t &list,
                 size_t idx) {
  const file_list_base_t *file_list = dynamic_cast<const file_list_base_t *>(&list);
  const std::string &item_name = list[idx];

  if (item_name.compare("..") == 0) {
    return true;
  }

  if (!show_hidden && item_name[0] == '.') {
    return false;
  }

  /* fnmatch discards strings with characters that are invalid in the locale
     codeset. However, we do want to use fnmatch because it also involves
     collation which is too complicated to handle ourselves. So we convert the
     file names to the locale codeset, and use fnmatch on those. Note that the
     filter string passed to this function is already in the locale codeset. */
  std::string fs_name = convert_lang_codeset(item_name, false);
  if ((file_list == nullptr || !file_list->is_dir(idx)) &&
      fnmatch(str->c_str(), fs_name.c_str(), 0) != 0) {
    return false;
  }
  return true;
}

}  // namespace t3widget
