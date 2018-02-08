/* Copyright (C) 2012,2018 G.P. Halkes
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
#include "internal.h"
#include <cstring>
#include <t3widget/key.h>
#include <t3widget/keybuffer.h>
#include <t3widget/mouse.h>

namespace t3_widget {

static const char *disable_mouse = "\033[?1006l\033[?1015l\033[?1005l\033[?1002l\033[?1000l";
static const char *enable_mouse = "\033[?1000h\033[?1002h\033[?1005h\033[?1015h\033[?1006h";

static mouse_event_buffer_t mouse_event_buffer;
static int mouse_button_state;

static enum {               // Mouse reporting states:
  XTERM_MOUSE_NONE,         // disabled
  XTERM_MOUSE_SINGLE_BYTE,  // using a single byte for each coordinate and button state
  XTERM_MOUSE_COORD_UTF,    // using UTF-like encoding for coordinates, but not for button state
                            // (xterm 262-267)
  XTERM_MOUSE_ALL_UTF       // using UTF-like encoding for coordinates and button state
} xterm_mouse_reporting;

#ifdef HAS_GPM
#include <gpm.h>
#include <linux/keyboard.h>
static bool use_gpm;
#endif

mouse_event_t read_mouse_event() { return mouse_event_buffer.pop_front(); }

bool use_xterm_mouse_reporting() { return xterm_mouse_reporting != XTERM_MOUSE_NONE; }

#define ensure_buffer_fill()                             \
  do {                                                   \
    while (char_buffer_fill == idx) {                    \
      if (!read_keychar(1)) {                            \
        xterm_mouse_reporting = XTERM_MOUSE_SINGLE_BYTE; \
        goto convert_mouse_event;                        \
      }                                                  \
    }                                                    \
  } while (false)

static bool convert_x10_mouse_event(int x, int y, int buttons) {
  mouse_event_t event;
  event.x = x <= 32 ? -1 : x - 33;
  event.y = y <= 32 ? -1 : y - 33;
  event.previous_button_state = mouse_button_state;
  if (buttons < 32) {
    return false;
  }
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
  if (event.type == EMOUSE_BUTTON_RELEASE && event.previous_button_state == 0) {
    return false;
  }

  event.window = nullptr;
  event.modifier_state = (buttons >> 2) & 7;
  mouse_event_buffer.push_back(event);
  return true;
}

static void convert_sgr_mouse_event(int x, int y, int buttons, char closing_char) {
  mouse_event_t event;
  event.x = x - 1;
  event.y = y - 1;
  event.previous_button_state = mouse_button_state;
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
  } else if (closing_char == 'M') {
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
        break;
    }
  } else if (closing_char == 'm') {
    event.type = EMOUSE_BUTTON_RELEASE;
    switch (buttons & 3) {
      case 0:
        event.button_state = mouse_button_state &= ~EMOUSE_BUTTON_LEFT;
        break;
      case 1:
        event.button_state = mouse_button_state &= ~EMOUSE_BUTTON_MIDDLE;
        break;
      case 2:
        event.button_state = mouse_button_state &= ~EMOUSE_BUTTON_RIGHT;
        break;
      default:
        break;
    }
  }
  event.window = nullptr;
  event.modifier_state = (buttons >> 2) & 7;
  mouse_event_buffer.push_back(event);
}

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
bool decode_xterm_mouse() {
  int x, y, buttons, idx, i;

  while (char_buffer_fill < 3) {
    if (!read_keychar(1)) {
      return false;
    }
  }

  if (xterm_mouse_reporting > XTERM_MOUSE_SINGLE_BYTE) {
    idx = 1;
    if ((char_buffer[0] & 0x80) && xterm_mouse_reporting == XTERM_MOUSE_ALL_UTF) {
      if ((char_buffer[0] & 0xc0) != 0xc0 || (char_buffer[1] & 0xc0) != 0x80) {
        xterm_mouse_reporting = XTERM_MOUSE_COORD_UTF;
      } else {
        idx = 2;
      }
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

#define DECODE_UTF()                                                                            \
  (char_buffer[idx] & 0x80 ? (idx += 2, ((((unsigned char)char_buffer[idx - 2] & ~0xc0) << 6) | \
                                         ((unsigned char)char_buffer[idx - 1] & ~0xc0)))        \
                           : char_buffer[idx++])

convert_mouse_event:
  idx = 0;
  switch (xterm_mouse_reporting) {
    case XTERM_MOUSE_SINGLE_BYTE:
      buttons = static_cast<unsigned char>(char_buffer[0]);
      x = static_cast<unsigned char>(char_buffer[1]);
      y = static_cast<unsigned char>(char_buffer[2]);
      idx = 3;
      break;
    case XTERM_MOUSE_COORD_UTF:
      buttons = static_cast<unsigned char>(char_buffer[0]);
      idx = 1;
      goto convert_coordinates;
    case XTERM_MOUSE_ALL_UTF:
      buttons = DECODE_UTF();
    convert_coordinates:
      x = DECODE_UTF();
      y = DECODE_UTF();
      break;
    default:
      return false;
  }
  char_buffer_fill -= idx;
  memmove(char_buffer, char_buffer + idx, char_buffer_fill);

  return convert_x10_mouse_event(x, y, buttons);
}

/** Decode an XTerm mouse event using the SGR or URXVT protocols.

    These are merged together, because the decoding of the values is the same
    for both protocols. I.e. they need to decode three integers. Note that this
    code assumes that the last character of the sequence is either 'M' or 'm'.
 */
bool decode_xterm_mouse_sgr_urxvt(const char *data, size_t len) {
  bool sgr_mode = false;
  size_t idx = 2;
  if (data[2] == '<') {
    sgr_mode = true;
    ++idx;
  }

  int buttons = 0;
  int x = 0;
  int y = 0;
  int *current_value = &buttons;
  for (; idx < len; ++idx) {
    if (data[idx] >= '0' && data[idx] <= '9') {
      *current_value = 10 * *current_value + (data[idx] - '0');
    } else if (data[idx] == ';') {
      if (current_value == &buttons) {
        current_value = &x;
      } else if (current_value == &x) {
        current_value = &y;
      } else {
        return false;
      }
    } else if (data[idx] == 'm' || data[idx] == 'M') {
      if (current_value != &y) {
        return false;
      }

      if (sgr_mode) {
        convert_sgr_mouse_event(x, y, buttons, data[idx]);
        return true;
      } else if (data[idx] == 'm') {
        return false;
      } else {
        return convert_x10_mouse_event(x + 32, y + 32, buttons);
      }
    } else {
      return false;
    }
  }
  // Note that we should not get here to begin with, but we do want to handle it sanely.
  return false;
}

#ifdef HAS_GPM
static bool process_gpm_event() {
  Gpm_Event gpm_event;
  mouse_event_t mouse_event;

  if (Gpm_GetEvent(&gpm_event) < 1) {
    if (gpm_fd < 0) {
      use_gpm = false;
    }
    return false;
  }

  mouse_event.previous_button_state = mouse_button_state;
  mouse_event.button_state = mouse_button_state;
  mouse_event.x = gpm_event.x - 1;
  mouse_event.y = gpm_event.y - 1;

  if (gpm_event.type & GPM_DOWN) {
    mouse_event.type = EMOUSE_BUTTON_PRESS;
    if (gpm_event.buttons & GPM_B_LEFT) {
      mouse_event.button_state = mouse_button_state |= EMOUSE_BUTTON_LEFT;
    }
    if (gpm_event.buttons & GPM_B_RIGHT) {
      mouse_event.button_state = mouse_button_state |= EMOUSE_BUTTON_RIGHT;
    }
    if (gpm_event.buttons & GPM_B_MIDDLE) {
      mouse_event.button_state = mouse_button_state |= EMOUSE_BUTTON_MIDDLE;
    }
  } else if (gpm_event.type & GPM_UP) {
    mouse_event.type = EMOUSE_BUTTON_RELEASE;
    if (gpm_event.buttons & GPM_B_LEFT) {
      mouse_event.button_state = mouse_button_state &= ~EMOUSE_BUTTON_LEFT;
    }
    if (gpm_event.buttons & GPM_B_RIGHT) {
      mouse_event.button_state = mouse_button_state &= ~EMOUSE_BUTTON_RIGHT;
    }
    if (gpm_event.buttons & GPM_B_MIDDLE) {
      mouse_event.button_state = mouse_button_state &= ~EMOUSE_BUTTON_MIDDLE;
    }
  } else if (gpm_event.type & GPM_DRAG) {
    mouse_event.type = EMOUSE_MOTION;
  }

  mouse_event.window = nullptr;
  mouse_event.modifier_state = 0;
  if (gpm_event.modifiers & (1 << KG_SHIFT)) {
    mouse_event.modifier_state |= EMOUSE_SHIFT;
  }
  if (gpm_event.modifiers & ((1 << KG_ALT) | (1 << KG_ALTGR))) {
    mouse_event.modifier_state |= EMOUSE_META;
  }
  if (gpm_event.modifiers & (1 << KG_CTRL)) {
    mouse_event.modifier_state |= EMOUSE_CTRL;
  }
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

void deinit_mouse_reporting() {
  if (xterm_mouse_reporting) {
    t3_term_putp(disable_mouse);
  }
}

void reinit_mouse_reporting() {
  if (xterm_mouse_reporting) {
    t3_term_putp(enable_mouse);
  }
}

void stop_mouse_reporting() {
  if (xterm_mouse_reporting) {
    t3_term_putp(disable_mouse);
  }
#if defined(HAS_GPM)
  if (use_gpm) {
    Gpm_Close();
  }
#endif
}

void fd_set_mouse_fd(fd_set *readset, int *max_fd) {
#if defined(HAS_GPM)
  if (use_gpm) {
    FD_SET(gpm_fd, readset);
    if (gpm_fd > *max_fd) {
      *max_fd = gpm_fd;
    }
  }
#else
  (void)readset;
#endif
}

bool check_mouse_fd(fd_set *readset) {
#if defined(HAS_GPM)
  if (use_gpm) {
    if (FD_ISSET(gpm_fd, readset)) {
      return process_gpm_event();
    }
  }
#else
  (void)readset;
#endif
  return false;
}

};  // namespace
