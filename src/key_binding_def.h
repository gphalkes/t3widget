/* Copyright (C) 2018 G.P. Halkes
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

// Explicitly doesn't have an include guard!
#define _T3_ACTION(action, name, ...) {ACTION_##action, name, {__VA_ARGS__}},
key_bindings_t<_T3_ACTION_TYPE::Action> _T3_ACTION_TYPE::key_bindings{
#include _T3_ACTION_FILE
};
#undef _T3_ACTION

key_bindings_t<_T3_ACTION_TYPE::Action> *_T3_ACTION_TYPE::get_key_binding() {
  return &key_bindings;
}
