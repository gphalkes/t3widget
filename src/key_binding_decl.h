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
public:
/** Actions which can be bound to keys. */
enum Action {
#define _T3_ACTION(action, ...) ACTION_##action,
#include _T3_ACTION_FILE
#undef _T3_ACTION
};

/** Returns the object holding the key bindings for this type. */
static key_bindings_t<Action> *get_key_binding();

private:
static key_bindings_t<Action> key_bindings;
