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
#ifdef WITH_X11
#ifdef HAS_DLFCN
#include <dlfcn.h>

using lt_dlhandle = void *;
#define lt_dlinit() 0
#define lt_dlexit()
#define lt_dlopen(name) dlopen(name, RTLD_NOW | RTLD_LOCAL)
#define lt_dlsym dlsym
#define lt_dlclose dlclose
#else
#include <ltdl.h>
#endif
#endif

#include <memory>
#include <string>
#include <utility>

/* Ensure that functions with internal linkage as defined in internal.h actually
   get compiled with internal linkage. */
#include "internal.h"
#include "log.h"
#include "t3widget/clipboard.h"
#include "t3widget/extclipboard.h"
#include "t3widget/main.h"
#include "t3widget/signals.h"

namespace t3widget {

std::shared_ptr<std::string> clipboard_data;
std::shared_ptr<std::string> primary_data;

static void init_external_clipboard(bool init);

static extclipboard_interface_t *extclipboard_calls;
static connection_t init_connected = connect_on_init(init_external_clipboard);

/** Get the clipboard data.

    While the returned linked_ptr is in scope, the clipboard should be locked.
    See lock_clipboard for details.
*/
std::shared_ptr<std::string> get_clipboard() {
  if (extclipboard_calls != nullptr) {
    return extclipboard_calls->get_selection(true);
  }
  return clipboard_data;
}

/** Get the primary selection data.

    While the returned linked_ptr is in scope, the clipboard should be locked.
    See lock_clipboard for details.
*/
std::shared_ptr<std::string> get_primary() {
  if (extclipboard_calls != nullptr) {
    return extclipboard_calls->get_selection(false);
  }
  return primary_data;
}

void set_clipboard(std::unique_ptr<std::string> str) {
  if (str != nullptr && str->size() == 0) {
    str.reset();
  }

  if (extclipboard_calls != nullptr) {
    extclipboard_calls->claim_selection(true, std::move(str));
    return;
  }
  clipboard_data.reset(str.release());
}

void set_primary(std::unique_ptr<std::string> str) {
  if (str != nullptr && str->size() == 0) {
    str.reset();
  }

  if (extclipboard_calls != nullptr) {
    if (!disable_primary_selection) {
      extclipboard_calls->claim_selection(false, std::move(str));
    }
    return;
  }
  primary_data.reset(str.release());
}

static void init_external_clipboard(bool init) {
#ifdef WITH_X11
  static lt_dlhandle extclipboard_mod;
#endif
  if (init_params->disable_external_clipboard) {
    return;
  }

  if (init) {
#ifdef WITH_X11
    if (lt_dlinit() != 0) {
      return;
    }

    if ((extclipboard_mod = lt_dlopen(X11_MOD_NAME)) == nullptr) {
      lprintf("Could not open external clipboard module (X11): %s\n", X11_MOD_NAME);
      return;
    }

    if ((extclipboard_calls = reinterpret_cast<extclipboard_interface_t *>(
             lt_dlsym(extclipboard_mod, "_t3_widget_extclipboard_calls"))) == nullptr) {
      lprintf("External clipboard module does not export interface symbol\n");
      lt_dlclose(extclipboard_mod);
      extclipboard_mod = nullptr;
      return;
    }
    if (extclipboard_calls->version != EXTCLIPBOARD_VERSION) {
      lprintf("External clipboard module has incompatible version\n");
      lt_dlclose(extclipboard_mod);
      extclipboard_mod = nullptr;
      return;
    }
    if (!extclipboard_calls->init()) {
      lprintf("Failed to initialize external clipboard module\n");
      lt_dlclose(extclipboard_mod);
      extclipboard_calls = nullptr;
    }
#endif
  } else {
#ifdef WITH_X11
    if (extclipboard_calls != nullptr) {
      extclipboard_calls->stop();
      extclipboard_calls = nullptr;
      lt_dlclose(extclipboard_mod);
    }
    lt_dlexit();
#endif
  }
}

void release_selections() {
  if (extclipboard_calls != nullptr) {
    extclipboard_calls->release_selections();
  }
}

void lock_clipboard() {
  if (extclipboard_calls != nullptr) {
    extclipboard_calls->lock();
  }
}

void unlock_clipboard() {
  if (extclipboard_calls != nullptr) {
    extclipboard_calls->unlock();
  }
}

void stop_clipboard() {
  if (extclipboard_calls != nullptr) {
    extclipboard_calls->stop();
  }
}

}  // namespace t3widget
