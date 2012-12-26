/* Copyright (C) 2012 G.P. Halkes
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
#include <cstring>
#include <t3widget/key.h>
#include <t3widget/mouse.h>
#include <t3widget/keybuffer.h>
#include "internal.h"

namespace t3_widget {

static const char *disable_mouse = "\033[?1005l\033[?1002l\033[?1000l";
static const char *enable_mouse = "\033[?1000h\033[?1002h\033[?1005h";

static mouse_event_buffer_t mouse_event_buffer;
static int mouse_button_state;

static enum { // Mouse reporting states:
	XTERM_MOUSE_NONE, // disabled
	XTERM_MOUSE_SINGLE_BYTE, // using a single byte for each coordinate and button state
	XTERM_MOUSE_COORD_UTF, // using UTF-like encoding for coordinates, but not for button state (xterm 262-267)
	XTERM_MOUSE_ALL_UTF // using UTF-like encoding for coordinates and button state
} xterm_mouse_reporting;


#ifdef HAS_GPM
#include <gpm.h>
#include <linux/keyboard.h>
static bool use_gpm;
#endif

mouse_event_t read_mouse_event(void) {
	return mouse_event_buffer.pop_front();
}

bool use_xterm_mouse_reporting(void) {
	return xterm_mouse_reporting != XTERM_MOUSE_NONE;
}

#define ensure_buffer_fill() do { \
	while (char_buffer_fill == idx) { \
		if (!read_keychar(1)) { \
			xterm_mouse_reporting = XTERM_MOUSE_SINGLE_BYTE; \
			goto convert_mouse_event; \
		} \
	} \
} while (0)

/** Decode an XTerm mouse event.

    This routine would have been simple, had it not been for the fact that the
    original XTerm mouse protocol is a little broken, and it is hard to detect
    whether the fix is present. First of all, the original protocol can not
    handle coordinates above 223. This is due to the fact that a coordinate is
    encoded as coordinate + 32 (and coordinates start at 1).

    Once screens got big enough to make terminals more than 223 columns wide,
    a fix was implemented, which uses UTF-8 encoding (but only using at most
    2 bytes, instead of simply using the full range). However, to accomodate
    clients which simply assumed all input was UTF-8 encoded, several versions
    later the buttons were encoded as UTF-8 as well. Thus we now have three
    different versions of the XTerm mouse reporting protocol.

    This wouldn't be so bad, if it wasn't for all those terminal emulators out
    there claiming to be XTerm. They make detection of the specific version of
    the protocol practically impossible, because they may report any version
    number in response to a "Send Device Attributes" request. Thus, this
    routine tries to do automatic switching, based on the received mouse
    reports.

    Autodetection logic:
    - start by assuming full UTF-8 encoded mode
    - if the buttons have the top bit set, but are not properly UTF-8 encoded,
      assume that only the coordinates have been UTF-8 encoded
    - if a coordinate is not properly UTF-8 encoded, assume single byte coding
    - if during the examination of the coordinate we find that not enough bytes
      are available to decode, assume single byte coding. (This situation can
      not be caused by incorrecly assuming that the buttons are UTF-8 encoded,
      because then the first coordinate byte would be invalid.)
*/
bool decode_xterm_mouse(void) {
	mouse_event_t event;
	int buttons, idx, i;

	while (char_buffer_fill < 3) {
		if (!read_keychar(1))
			return false;
	}

	if (xterm_mouse_reporting > XTERM_MOUSE_SINGLE_BYTE) {
		idx = 1;
		if ((char_buffer[0] & 0x80) && xterm_mouse_reporting == XTERM_MOUSE_ALL_UTF) {
			if ((char_buffer[0] & 0xc0) != 0xc0 || (char_buffer[1] & 0xc0) != 0x80)
				xterm_mouse_reporting = XTERM_MOUSE_COORD_UTF;
			else
				idx = 2;
		}
		for (i = 0; i < 2; i++) {
			ensure_buffer_fill();
			if ((char_buffer[idx] & 0x80) && xterm_mouse_reporting >= XTERM_MOUSE_COORD_UTF) {
				idx++;
				ensure_buffer_fill();
				if ((char_buffer[idx - 1] & 0xc0) != 0xc0 || (char_buffer[idx] & 0xc0) != 0x80) {
					xterm_mouse_reporting = XTERM_MOUSE_SINGLE_BYTE;
					goto convert_mouse_event;
				}
			}
			idx++;
		}
	}

#define DECODE_UTF() (char_buffer[idx] & 0x80 ? (idx += 2, ((((unsigned char) char_buffer[idx - 2] & ~0xc0) << 6) | \
	((unsigned char) char_buffer[idx - 1] & ~0xc0))) : char_buffer[idx++])

convert_mouse_event:
	idx = 0;
	switch (xterm_mouse_reporting) {
		case XTERM_MOUSE_SINGLE_BYTE:
			buttons = (unsigned char) char_buffer[0];
			event.x = (unsigned char) char_buffer[1];
			event.y = (unsigned char) char_buffer[2];
			idx = 3;
			break;
		case XTERM_MOUSE_COORD_UTF:
			buttons = (unsigned char) char_buffer[0];
			idx = 1;
			goto convert_coordinates;
		case XTERM_MOUSE_ALL_UTF:
			buttons = DECODE_UTF();
		convert_coordinates:
			event.x = DECODE_UTF();
			event.y = DECODE_UTF();
			break;
		default:
			return false;
	}
	char_buffer_fill -= idx;
	memmove(char_buffer, char_buffer + idx, char_buffer_fill);

	event.x = event.x <= 32 ? event.x = -1 : event.x - 33;
	event.y = event.y <= 32 ? event.y = -1 : event.y - 33;
	event.previous_button_state = mouse_button_state;
	buttons -= 32;

	if (buttons & 64) {
		event.type = EMOUSE_BUTTON_PRESS;
		switch (buttons & 3) {
			case 0:
				event.button_state = mouse_button_state | EMOUSE_SCROLL_UP;
				break;
			case 1:
				event.button_state = mouse_button_state | EMOUSE_SCROLL_DOWN;
				break;
			default:
				event.button_state = mouse_button_state;
				break;
		}
	} else if (buttons & 32) {
		event.type = EMOUSE_MOTION;
		/* Trying to decode the button state here is pretty much useless, because
		   it can only encode a single button. The saved mouse_button_state is
		   more accurate. */
		event.button_state = mouse_button_state;
	} else {
		event.type = EMOUSE_BUTTON_PRESS;
		switch (buttons & 3) {
			case 0:
				event.button_state = mouse_button_state |= EMOUSE_BUTTON_LEFT;
				break;
			case 1:
				event.button_state = mouse_button_state |= EMOUSE_BUTTON_MIDDLE;
				break;
			case 2:
				event.button_state = mouse_button_state |= EMOUSE_BUTTON_RIGHT;
				break;
			default:
				event.type = EMOUSE_BUTTON_RELEASE;
				event.button_state = mouse_button_state = 0;
				break;
		}
	}
	/* Due to the fact that the XTerm mouse protocol doesn't say which mouse button
	   was released, we assume that all buttons are released on a  button release
	   event. However, when multiple buttons are pressed simultaneously, this may
	   result in button release events where no buttons where previously thought
	   to be depressed. We filter those out here. */
	if (event.type == EMOUSE_BUTTON_RELEASE && event.previous_button_state == 0)
		return false;

	event.window = NULL;
	event.modifier_state = (buttons >> 2) & 7;
	mouse_event_buffer.push_back(event);
	return true;
}

#ifdef HAS_GPM
static bool process_gpm_event() {
	Gpm_Event gpm_event;
	mouse_event_t mouse_event;

	if (Gpm_GetEvent(&gpm_event) < 1) {
		if (gpm_fd < 0)
			use_gpm = false;
		return false;
	}

	mouse_event.previous_button_state = mouse_button_state;
	mouse_event.button_state = mouse_button_state;
	mouse_event.x = gpm_event.x - 1;
	mouse_event.y = gpm_event.y - 1;

	if (gpm_event.type & GPM_DOWN) {
		mouse_event.type = EMOUSE_BUTTON_PRESS;
		if (gpm_event.buttons & GPM_B_LEFT)
			mouse_event.button_state = mouse_button_state |= EMOUSE_BUTTON_LEFT;
		if (gpm_event.buttons & GPM_B_RIGHT)
			mouse_event.button_state = mouse_button_state |= EMOUSE_BUTTON_RIGHT;
		if (gpm_event.buttons & GPM_B_MIDDLE)
			mouse_event.button_state = mouse_button_state |= EMOUSE_BUTTON_MIDDLE;
	} else if (gpm_event.type & GPM_UP) {
		mouse_event.type = EMOUSE_BUTTON_RELEASE;
		if (gpm_event.buttons & GPM_B_LEFT)
			mouse_event.button_state = mouse_button_state &= ~EMOUSE_BUTTON_LEFT;
		if (gpm_event.buttons & GPM_B_RIGHT)
			mouse_event.button_state = mouse_button_state &= ~EMOUSE_BUTTON_RIGHT;
		if (gpm_event.buttons & GPM_B_MIDDLE)
			mouse_event.button_state = mouse_button_state &= ~EMOUSE_BUTTON_MIDDLE;
	} else if (gpm_event.type & GPM_DRAG) {
		mouse_event.type = EMOUSE_MOTION;
	}

	mouse_event.window = NULL;
	mouse_event.modifier_state = 0;
	if (gpm_event.modifiers & (1 << KG_SHIFT))
		mouse_event.modifier_state |= EMOUSE_SHIFT;
	if (gpm_event.modifiers & ((1 << KG_ALT) | (1 << KG_ALTGR)))
		mouse_event.modifier_state |= EMOUSE_META;
	if (gpm_event.modifiers & (1 << KG_CTRL))
		mouse_event.modifier_state |= EMOUSE_CTRL;
	mouse_event_buffer.push_back(mouse_event);
	return true;
}
#endif

void init_mouse_reporting(bool xterm_mouse) {
	if (xterm_mouse) {
		/* Start out in ALL_UTF mode. The decode_xterm_mouse routine will switch back
		   to a different mode if necessary. */
		xterm_mouse_reporting = XTERM_MOUSE_ALL_UTF;
		t3_term_putp(enable_mouse);
		return;
	}
#if defined(HAS_GPM)
	{
		Gpm_Connect connect;
		connect.eventMask = GPM_DOWN | GPM_UP | GPM_DRAG;
		connect.defaultMask = GPM_MOVE;
		connect.minMod = 0;
		connect.maxMod = ~0;

		if (Gpm_Open(&connect, 0) >= 0) {
			lprintf("Socket opened successfully\n");
			use_gpm = true;
		}
	}
#endif
}

void deinit_mouse_reporting(void) {
	if (xterm_mouse_reporting)
		t3_term_putp(disable_mouse);
}

void reinit_mouse_reporting(void) {
	if (xterm_mouse_reporting)
		t3_term_putp(enable_mouse);
}

void stop_mouse_reporting(void) {
	if (xterm_mouse_reporting)
		t3_term_putp(disable_mouse);
#if defined(HAS_GPM)
	if (use_gpm)
		Gpm_Close();
#endif
}

void fd_set_mouse_fd(fd_set *readset, int *max_fd) {
#if defined(HAS_GPM)
	if (use_gpm) {
		FD_SET(gpm_fd, readset);
		if (gpm_fd > *max_fd)
			*max_fd = gpm_fd;
	}
#else
	(void) readset;
#endif
}

bool check_mouse_fd(fd_set *readset) {
#if defined(HAS_GPM)
	if (use_gpm) {
		if (FD_ISSET(gpm_fd, readset))
			return process_gpm_event();
	}
#else
	(void) readset;
#endif
	return false;
}

}; //namespace
