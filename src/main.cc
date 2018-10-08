/* Copyright (C) 2011-2013,2018 G.P. Halkes
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

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <new>
#include <stdio.h>
#include <string>
#include <t3key/key.h>
#include <transcript/transcript.h>
#include <unistd.h>
#ifdef __linux__
#include <linux/kd.h>
#include <sys/ioctl.h>
#endif

#include "t3key/key_errors.h"
#include "t3widget/clipboard.h"
#include "t3widget/colorscheme.h"
#include "t3widget/dialogs/dialog.h"
#include "t3widget/dialogs/insertchardialog.h"
#include "t3widget/dialogs/messagedialog.h"
#include "t3widget/interfaces.h"
#include "t3widget/internal.h"
#include "t3widget/key.h"
#include "t3widget/log.h"
#include "t3widget/main.h"
#include "t3widget/mouse.h"
#include "t3widget/signals.h"
#include "t3widget/string_view.h"
#include "t3widget/textline.h"
#include "t3widget/util.h"
#include "t3window/terminal.h"

namespace t3widget {
#define MESSAGE_DIALOG_WIDTH 50
#define MIN_LINES 16
#define MIN_COLUMNS 60

static int init_level;
static int screen_lines, screen_columns;
static signal_t<int, int> resize;
static signal_t<> update_notification;

init_parameters_t *init_params;
bool disable_primary_selection;

#ifdef __linux__
static int linux_meta_mode = -1;
#endif

insert_char_dialog_t *insert_char_dialog;
message_dialog_t *message_dialog;

struct T3_WIDGET_LOCAL complex_error_t::implementation_t {
  bool success;
  source_t source;
  int error;
  const char *file_name;
  int line_number;

  implementation_t() : success(true), source(SRC_NONE), error(0) {}
  implementation_t(source_t _source, int _error, const char *_file_name, int _line_number)
      : success(false),
        source(_source),
        error(_error),
        file_name(_file_name),
        line_number(_line_number) {}
};

complex_error_t::complex_error_t() : impl(new implementation_t) {}
complex_error_t::complex_error_t(source_t source, int error, const char *file_name, int line_number)
    : impl(new implementation_t(source, error, file_name, line_number)) {}

complex_error_t::~complex_error_t() {}

complex_error_t::complex_error_t(const complex_error_t &other)
    : impl(new implementation_t(*other.impl)) {}

complex_error_t &complex_error_t::operator=(const complex_error_t &other) {
  *impl = *other.impl;
  return *this;
}

void complex_error_t::set_error(source_t source, int error, const char *file_name,
                                int line_number) {
  impl->success = false;
  impl->source = source;
  impl->error = error;
  impl->file_name = file_name;
  impl->line_number = line_number;
}

bool complex_error_t::get_success() { return impl->success; }
complex_error_t::source_t complex_error_t::get_source() { return impl->source; }
int complex_error_t::get_error() { return impl->error; }

std::string complex_error_t::get_string() {
  std::string error_str;

  switch (impl->source) {
    case SRC_ERRNO:
      if (impl->file_name != nullptr) {
        char number_buffer[128];
        error_str = impl->file_name;
        sprintf(number_buffer, ":%d: ", impl->line_number);
        error_str.append(number_buffer);
      }
      error_str.append(strerror(impl->error));
      break;
    case SRC_TRANSCRIPT:
      error_str = "libtranscript: ";
      error_str.append(transcript_strerror(static_cast<transcript_error_t>(impl->error)));
      break;
    case SRC_T3_KEY:
      error_str = "libt3key: ";
      error_str.append(t3_key_strerror(impl->error));
      break;
    case SRC_T3_WINDOW:
      error_str = "libt3window: ";
      error_str.append(t3_window_strerror(impl->error));
      break;
    default:
      return strerror(0);
  }
  return error_str;
}

std::unique_ptr<init_parameters_t> init_parameters_t::create() {
  return std::unique_ptr<init_parameters_t>(new init_parameters_t);
}

init_parameters_t::init_parameters_t()
    : separate_keypad(false), disable_external_clipboard(false) {}

connection_t connect_resize(std::function<void(int, int)> func) { return resize.connect(func); }

connection_t connect_update_notification(std::function<void()> func) {
  return update_notification.connect(func);
}

static signal_t<bool> &on_init() {
  static std::unique_ptr<signal_t<bool>> on_init_obj(new signal_t<bool>());
  return *on_init_obj;
}

connection_t connect_on_init(std::function<void(bool)> func) { return on_init().connect(func); }

static signal_t<> &terminal_settings_changed() {
  static std::unique_ptr<signal_t<>> terminal_settings_changed_obj(new signal_t<>());
  return *terminal_settings_changed_obj;
}

connection_t connect_terminal_settings_changed(std::function<void()> func) {
  return terminal_settings_changed().connect(func);
}

static void do_resize() {
  int new_screen_lines, new_screen_columns;

  t3_term_resize();
  t3_term_get_size(&new_screen_lines, &new_screen_columns);
  if (new_screen_lines == screen_lines && new_screen_columns == screen_columns) {
    return;
  }

  // Ensure minimal coordinates to maintain sort of sane screen layout
  screen_lines = new_screen_lines < MIN_LINES ? MIN_LINES : new_screen_lines;
  screen_columns = new_screen_columns < MIN_COLUMNS ? MIN_COLUMNS : new_screen_columns;

  resize(screen_lines, screen_columns);
}

void get_screen_size(int *height, int *width) {
  if (height != nullptr) {
    *height = screen_lines;
  }
  if (width != nullptr) {
    *width = screen_columns;
  }
}

void set_primary_selection_mode(bool on) { disable_primary_selection = !on; }

enum terminal_code_t { TERM_NONE, TERM_XTERM, TERM_LINUX };

struct terminal_mapping_t {
  const char *name;
  terminal_code_t code;
};

static terminal_mapping_t terminal_mapping[] = {{"xterm", TERM_XTERM},
                                                {"xterm-256color", TERM_XTERM},
                                                {"xterm-88color", TERM_XTERM},
                                                {nullptr, TERM_NONE}};

static terminal_code_t terminal;

static void terminal_specific_restore() {
  switch (terminal) {
    case TERM_XTERM:
      /* Note: this may not actually reset to previous value, because of broken xterm */
      t3_term_putp("\033[?1036r");
      break;
#ifdef __linux__
    case TERM_LINUX:
      if (linux_meta_mode > 0) {
        ioctl(0, KDSKBMETA, linux_meta_mode);
      }
      break;
#endif
    default:
      break;
  }
}

static void terminal_specific_setup() {
  int i;

  if (!init_params->term.is_valid()) {
    return;
  }

  for (i = 0;
       terminal_mapping[i].name != nullptr && terminal_mapping[i].name != init_params->term.value();
       i++) {
  }
  terminal = terminal_mapping[i].code;

  switch (terminal) {
    case TERM_XTERM:
      t3_term_putp("\033[?1036s\033[?1036h");
      break;
#ifdef __linux__
    case TERM_LINUX:
      if (ioctl(0, KDGKBMETA, &linux_meta_mode) < 0) {
        linux_meta_mode = -1;
      } else {
        ioctl(0, KDSKBMETA, K_ESCPREFIX);
      }
      break;
#endif
    default:
      return;
  }
}

void restore() {
  switch (init_level) {
    default:
    case 3:
      stop_clipboard();
      cleanup_keys();
      transcript_finalize();
    /* FALLTHROUGH */
    case 2:
      terminal_specific_restore();
    /* FALLTHROUGH */
    case 1:
      if (init_level < 3) {
        /* Eat up all keys/terminal replies, before restoring the terminal. */
        while (t3_term_get_keychar(100) >= 0) {
        }
      }
      t3_term_restore();
    /* FALLTHROUGH */
    case 0:
      if (init_params != nullptr) {
        delete init_params;
        init_params = nullptr;
      }
      break;
  }
  init_level = 0;
}

complex_error_t init(const init_parameters_t *params) {
  complex_error_t result;
  int term_init_result;

  if (init_level > 0) {
    return result;
  }

  init_log();
  text_line_t::init();

  if (init_params == nullptr) {
    init_params = init_parameters_t::create().release();
  }

  if (params == nullptr || !params->term.is_valid()) {
    const char *term_env = getenv("TERM");
    /* If term_env == nullptr, t3_term_init will abort anyway, so we ignore
       that case. */
    if (term_env != nullptr) {
      init_params->term = std::string(term_env);
    }
  } else {
    init_params->term = params->term;
  }

  if (params != nullptr) {
    init_params->program_name =
        params->program_name.empty() ? std::string("This program") : params->program_name;
    init_params->separate_keypad = params->separate_keypad;
    init_params->disable_external_clipboard = params->disable_external_clipboard;
  }

  atexit(restore);
  if ((term_init_result = t3_term_init(
           -1, init_params->term.is_valid() ? init_params->term.value().c_str() : nullptr)) !=
      T3_ERR_SUCCESS) {
    int saved_errno = errno;
    restore();
    result.set_error(complex_error_t::SRC_T3_WINDOW, term_init_result);
    errno = saved_errno;
    return result;
  }
  init_level++;

  terminal_specific_setup();
  init_level++;

  transcript_error_t transcript_error = transcript_init();
  if (transcript_error != TRANSCRIPT_SUCCESS) {
    result.set_error(complex_error_t::SRC_TRANSCRIPT, transcript_error);
    return result;
  }

  result = init_keys(init_params->term, init_params->separate_keypad);
  if (!result.get_success()) {
    int saved_errno = errno;
    restore();
    errno = saved_errno;
    return result;
  }
  init_level++;

  init_attributes();
  do_resize();
  try {
    /* Construct these here, such that the locale is set correctly and
       gettext therefore returns the correctly localized strings. */
    if (message_dialog == nullptr) {
      message_dialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, _("Message"), {_("Close")});
    }
    if (insert_char_dialog == nullptr) {
      insert_char_dialog = new insert_char_dialog_t();
    }
    on_init()(true);
  } catch (std::bad_alloc &ba) {
    restore();
    result.set_error(complex_error_t::SRC_ERRNO, ENOMEM);
  }
  t3_term_hide_cursor();
  return result;
}

void iterate() {
  key_t key;

  dialog_t::update_dialogs();
  t3_term_update();
  key = read_key();
  if (key == EKEY_MOUSE_EVENT) {
    mouse_event_t event = read_mouse_event();
    lprintf("Got mouse event: x=%d, y=%d, button_state=%d, modifier_state=%d\n", event.x, event.y,
            event.button_state, event.modifier_state);
    mouse_target_t::handle_mouse_event(event);
  } else {
    lprintf("Got key %04X\n", key);
    switch (key) {
      case EKEY_RESIZE:
        do_resize();
        break;
      case EKEY_EXTERNAL_UPDATE:
        update_notification();
        break;
      case EKEY_UPDATE_TERMINAL:
        terminal_settings_changed()();
        break;
      default:
        if (key >= EKEY_EXIT_MAIN_LOOP && key <= EKEY_EXIT_MAIN_LOOP + 255) {
          exit_main_loop(key - EKEY_EXIT_MAIN_LOOP);
        }
        // FIXME: pass unhandled keys to callback?
        dialog_t::active_dialogs.back()->process_key(key);
        break;
    }
  }
}

struct main_loop_exit_t {
  int retval;
  main_loop_exit_t(int _retval) : retval(_retval) {}
};

int main_loop() {
  try {
    while (true) {
      iterate();
    }
  } catch (main_loop_exit_t &e) {
    return e.retval;
  }
}

void exit_main_loop(int retval) { throw main_loop_exit_t(retval); }

void cleanup() {
  on_init()(false);
  delete message_dialog;
  message_dialog = nullptr;
  delete insert_char_dialog;
  insert_char_dialog = nullptr;
  restore();
  t3_term_deinit();
}

void suspend() {
  // FIXME: check return values!
  release_selections();
  deinit_keys();
  terminal_specific_restore();
  t3_term_restore();
  printf("%s has been stopped. You can return to %s by entering 'fg'.\n",
         init_params->program_name.c_str(), init_params->program_name.c_str());
  kill(getpid(), SIGSTOP);
  t3_term_init(-1, nullptr);
  terminal_specific_setup();
  reinit_keys();
  do_resize();
  t3_term_hide_cursor();
}

void redraw() {
  do_resize();
  t3_term_redraw();
}

long get_version() { return T3_WIDGET_VERSION; }

long get_libt3key_version() { return t3_key_get_version(); }

long get_libt3window_version() { return t3_window_get_version(); }

}  // namespace t3widget
