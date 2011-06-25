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
#include "widget_api.h"

namespace t3_widget {

#if INT_MAX < 2147483647L
typedef long key_t;
#else
typedef int key_t;
#endif

#define EKEY_ESC 27
#define EKEY_SHIFT 0x40000000
#define EKEY_META 0x20000000
#define EKEY_CTRL 0x10000000
#define EKEY_PROTECT 0x08000000
#define EKEY_KEY_MASK 0x1fffff

enum {
	EKEY_IGNORE = (key_t) -1,

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
	EKEY_KP_CENTER,

	EKEY_KP_END,
	EKEY_KP_HOME,
	EKEY_KP_PGUP,
	EKEY_KP_PGDN,
	EKEY_KP_LEFT,
	EKEY_KP_RIGHT,
	EKEY_KP_UP,
	EKEY_KP_DOWN,
	EKEY_KP_DEL,
	EKEY_KP_INS,
	EKEY_KP_NL,
	EKEY_KP_DIV,
	EKEY_KP_MUL,
	EKEY_KP_PLUS,
	EKEY_KP_MINUS,

	EKEY_F1 = 0x110100,
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
	EKEY_F13,
	EKEY_F14,
	EKEY_F15,
	EKEY_F16,
	EKEY_F17,
	EKEY_F18,
	EKEY_F19,
	EKEY_F20,
	EKEY_F21,
	EKEY_F22,
	EKEY_F23,
	EKEY_F24,
	EKEY_F25,
	EKEY_F26,
	EKEY_F27,
	EKEY_F28,
	EKEY_F29,
	EKEY_F30,
	EKEY_F31,
	EKEY_F32,
	EKEY_F33,
	EKEY_F34,
	EKEY_F35,
	EKEY_F36,

	/* Make sure the synthetic keys are out of the way of future aditions. */
	EKEY_RESIZE = 0x111000,
	EKEY_HOTKEY,
	EKEY_EXTERNAL_UPDATE,
	EKEY_UPDATE_TERMINAL,
};

T3_WIDGET_API key_t read_key(void);
T3_WIDGET_API void set_key_timeout(int msec);

}; // namespace
#endif
