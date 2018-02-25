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
#include "widgets/colorpicker.h"
#include "colorscheme.h"
#include <cstdio>

namespace t3_widget {
// FIXME: handle terminals which only do color pairs, although maybe it is better to make a separate
// widget for that
#define COLORS_PER_LINE 36
color_picker_base_t::color_picker_base_t(bool _fg)
    : current_color(-2), fg(_fg), has_focus(false), undefined_colors(0) {}

bool color_picker_base_t::process_key(key_t key) {
  int start_color = current_color;
  int x, y;
  switch (key) {
    case EKEY_UP:
      color_to_xy(current_color, x, y);
      if (y == 1) {
        break;
      }

      current_color = xy_to_color(x, y - 1);
      if (current_color == INT_MIN) {
        current_color = xy_to_color(1, y) - 1;
      }
      break;
    case EKEY_DOWN:
      color_to_xy(current_color, x, y);
      current_color = xy_to_color(x, y + 1);
      if (current_color == INT_MIN) {
        int new_y;
        color_to_xy(max_color, x, new_y);
        current_color = (new_y != y) ? max_color : start_color;
      }
      break;
    case EKEY_RIGHT:
      if (current_color < max_color) {
        current_color++;
      }
      break;
    case EKEY_LEFT:
      if (current_color > -2) {
        current_color--;
      }
      break;
    case EKEY_HOME:
      current_color = -2;
      break;
    case EKEY_END:
      current_color = max_color;
      break;
    case EKEY_NL:
      activated();
      break;
    default:
      return false;
  }
  if (current_color != start_color) {
    selection_changed();
  }
  return true;
}

bool color_picker_base_t::set_size(optint height, optint width) {
  (void)height;
  (void)width;
  return true;
}

bool color_picker_base_t::process_mouse_event(mouse_event_t event) {
  int new_color;

  if (event.window != window) {
    return true;
  }
  if (event.button_state & EMOUSE_CLICKED_LEFT) {
    new_color = xy_to_color(event.x, event.y);

    if (new_color == INT_MIN) {
      return true;
    }

    current_color = new_color;
    redraw = true;
    selection_changed();
    if (event.button_state & EMOUSE_DOUBLE_CLICKED_LEFT) {
      activated();
    }
  }
  return true;
}

void color_picker_base_t::set_focus(focus_t focus) {
  has_focus = focus != window_component_t::FOCUS_OUT;
  redraw = true;
}

void color_picker_base_t::set_undefined_colors(t3_attr_t attr) {
  undefined_colors = attr & (T3_ATTR_FG_MASK | T3_ATTR_BG_MASK);
}

t3_attr_t color_picker_base_t::get_color() {
  return fg ? (current_color >= 0 ? T3_ATTR_FG(current_color)
                                  : (current_color == -1 ? T3_ATTR_FG_DEFAULT : 0))
            : (current_color >= 0 ? T3_ATTR_BG(current_color)
                                  : (current_color == -1 ? T3_ATTR_BG_DEFAULT : 0));
}

void color_picker_base_t::set_color(t3_attr_t attr) {
  int color;
  if (fg) {
    color = (attr & T3_ATTR_FG_MASK) >> T3_ATTR_COLOR_SHIFT;
  } else {
    color = (attr & T3_ATTR_BG_MASK) >> (T3_ATTR_COLOR_SHIFT + 9);
  }

  if (color == 0) {
    current_color = -2;
  } else if (color == 257) {
    current_color = -1;
  } else {
    current_color = color - 1;
  }
}

void color_picker_base_t::update_contents() {
  int i, old_y, x, y;

  if (!redraw) {
    return;
  }

  window.set_default_attrs(attributes.dialog);
  window.set_paint(0, 0);
  window.clrtobot();
  window.box(0, 0, window.get_height(), window.get_width(), 0);
  window.set_paint(1, 1);

  for (i = -2, old_y = 1; i <= max_color; i++) {
    color_to_xy(i, x, y);
    if (y != old_y) {
      window.addch(T3_ACS_VLINE, T3_ATTR_ACS);
      window.set_paint(y, 1);
      old_y = y;
    }
    window.addch(' ', get_paint_attr(i));
  }
  window.addch(T3_ACS_VLINE, T3_ATTR_ACS);

  if (has_focus) {
    color_to_xy(current_color, x, y);
    window.set_paint(0, x);
    window.addch(T3_ACS_DARROW, T3_ATTR_ACS);
    window.set_paint(y, 0);
    window.addch(T3_ACS_RARROW, T3_ATTR_ACS);
    window.set_paint(y, x);
    window.addch(T3_ACS_DIAMOND, T3_ATTR_ACS | get_paint_attr(current_color));
  }

  window.set_paint(window.get_height() - 1, 1);
  window.addstr(color_str, 0);
  paint_color_name(current_color);
  window.addch(' ', 0);
}

void color_picker_base_t::paint_color_name(int color) {
  if (color == -2) {
    if ((fg && (undefined_colors & T3_ATTR_FG_MASK)) ||
        (!fg && (undefined_colors & T3_ATTR_BG_MASK))) {
      window.addstr("Base", 0);
    } else {
      window.addstr("Undefined", 0);
    }
  } else if (current_color == -1) {
    window.addstr("Terminal default", 0);
  } else {
    char color_number[20];
    sprintf(color_number, "%d", color);
    window.addstr(color_number, 0);
  }
}

//=================================================================
color_picker_t::color_picker_t(bool _fg) : color_picker_base_t(_fg) {
  int x, y;
  t3_term_caps_t terminal_capabilities;
  t3_term_get_caps(&terminal_capabilities);
  color_str = " Color: ";

  max_color = terminal_capabilities.colors - 1;
  if (max_color > 255) {
    max_color = 255;
  }
  color_picker_t::color_to_xy(max_color, x, y);

  init_window(y + 2, COLORS_PER_LINE + 2);
}

t3_attr_t color_picker_t::get_paint_attr(int color) {
  switch (color) {
    case -2:
      return fg
                 ? (attributes.dialog & T3_ATTR_FG_MASK) |
                       ((undefined_colors & T3_ATTR_FG_MASK) << 9)
                 : (undefined_colors & T3_ATTR_BG_MASK);
    case -1:
      return fg
                 ? T3_ATTR_REVERSE | T3_ATTR_FG_DEFAULT |
                       ((attributes.dialog & T3_ATTR_FG_MASK) << 9)
                 : T3_ATTR_BG_DEFAULT;
    default:
      return T3_ATTR_BG(color);
  }
}

void color_picker_t::paint_color_name(int color) {
  static const char *color_to_text[] = {
      "Black",      "Red",           "Green",      "Yellow",    "Blue",        "Magenta",
      "Cyan",       "Gray",          "Dark gray",  "Light red", "Light green", "Light yellow",
      "Light blue", "Light magenta", "Light cyan", "White"};
  if (color >= 0 && color < 16) {
    window.addstr(color_to_text[color], 0);
  } else {
    color_picker_base_t::paint_color_name(color);
  }
}

int color_picker_t::xy_to_color(int x, int y) {
  int color;
  if (x == 0 || x == window.get_width() - 1 || y == 0 || y == window.get_height() - 1) {
    return INT_MIN;
  }
  if (y == 1) {
    color = x - 3;
    if (color > 16 || color > max_color || current_color == color) {
      return INT_MIN;
    }
  } else {
    color = 16 + (y - 2) * COLORS_PER_LINE + x - 1;
    if (color > max_color) {
      return INT_MIN;
    }
  }
  return color;
}

void color_picker_t::color_to_xy(int color, int &x, int &y) {
  if (color < 16) {
    y = 1;
    x = color + 3;
  } else if (color < 232) {
    y = 2 + (color - 16) / COLORS_PER_LINE;
    x = 1 + (color - 16) % COLORS_PER_LINE;
  } else {
    y = 8;
    x = 1 + color - 232;
  }
}

//================================================================
color_pair_picker_t::color_pair_picker_t() : color_picker_base_t(true) {
  int x, y;
  t3_term_caps_t terminal_capabilities;
  t3_term_get_caps(&terminal_capabilities);
  color_str = " Color pair: ";

  max_color = terminal_capabilities.pairs - 1;
  if (max_color > 255) {
    max_color = 255;
  }
  color_pair_picker_t::color_to_xy(max_color, x, y);

  init_window(y + 1, COLORS_PER_LINE + 2);
}

int color_pair_picker_t::xy_to_color(int x, int y) {
  int color;

  if (x == 0 || x == window.get_width() - 1 || y == 0 || y == window.get_height() - 1) {
    return INT_MIN;
  }
  color = (x - 1) + (y - 1) * COLORS_PER_LINE;
  /* Take undefined and default color into account. */
  color -= 2;
  if (color > max_color) {
    return INT_MIN;
  }
  return color;
}

void color_pair_picker_t::color_to_xy(int color, int &x, int &y) {
  /* Take undefined and default color into account. */
  color += 2;
  x = 1 + color % COLORS_PER_LINE;
  y = 1 + color / COLORS_PER_LINE;
}

t3_attr_t color_pair_picker_t::get_paint_attr(int color) {
  switch (color) {
    case -2:
      return undefined_colors & T3_ATTR_FG_MASK;
    case -1:
      return T3_ATTR_FG_DEFAULT;
    default:
      return T3_ATTR_FG(color);
  }
}

}  // namespace
