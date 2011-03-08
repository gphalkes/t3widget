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
#ifndef T3_WIDGET_KEYS_H
#define T3_WIDGET_KEYS_H

#include <climits>

#if INT_MAX < 2147483647L
typedef long key_t;
#else
typedef int key_t;
#endif

int init_keys(void);
void deinit_keys(void);
void reinit_keys(void);
key_t wreadkey(void);
#define EKEY_ESC 27
#define EKEY_SHIFT 0x40000000
#define EKEY_META 0x20000000
#define EKEY_CTRL 0x10000000
#define EKEY_PROTECT 0x08000000

enum {
	EKEY_END = 0x110000,
	EKEY_HOME,
	EKEY_PGUP,
	EKEY_PGDN,
	EKEY_LEFT,
	EKEY_RIGHT,
	EKEY_UP,
	EKEY_DOWN,
	EKEY_DEL,
	EKEY_INS,
	EKEY_BS,
	EKEY_NL,
	EKEY_F1,
	EKEY_F2,
	EKEY_F3,
	EKEY_F4,
	EKEY_F5,
	EKEY_F6,
	EKEY_F7,
	EKEY_F8,
	EKEY_F9,
	EKEY_F10,
	EKEY_F11,
	EKEY_F12,
	EKEY_RESIZE,
	EKEY_HOTKEY,
	EKEY_MENU,
	EKEY_NOP
};

void insert_protected_key(key_t key);
#endif
