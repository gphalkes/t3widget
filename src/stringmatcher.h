/* Copyright (C) 2011 G.P. Halkes
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
#ifndef T3_WIDGET_STRINGMATCHER_H
#define T3_WIDGET_STRINGMATCHER_H

#include <memory>
#include <string>
#include <t3widget/util.h>
#include <t3widget/widget_api.h>
#include <vector>

namespace t3_widget {

class T3_WIDGET_LOCAL string_matcher_t {
 private:
  std::string needle;
  std::vector<int> partial_match_table, reverse_partial_match_table, index_table;
  int i;
  void init();

 public:
  string_matcher_t(const std::string &_needle);
  string_matcher_t(char *_needle, size_t _needle_size);
  virtual ~string_matcher_t();
  void reset();
  int next_char(const std::string *c);
  int previous_char(const std::string *c);
  int next_char(const char *c, size_t c_size);
  int previous_char(const char *c, size_t c_size);
};

};  // namespace
#endif
