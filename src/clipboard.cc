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
#ifdef WITH_X11
#include <ltdl.h>
#endif

#include "clipboard.h"
#include "ptr.h"
#include "extclipboard.h"

/* Ensure that functions with internal linkage as defined in internal.h actually
   get compiled with internal linkage. */
#include "internal.h"
#include "log.h"

using namespace std;

namespace t3_widget {

linked_ptr<string> clipboard_data;
linked_ptr<string> primary_data;

static void init_external_clipboard(bool init);

static extclipboard_interface_t *extclipboard_calls;
static sigc::connection init_connected = connect_on_init(sigc::ptr_fun(init_external_clipboard));

/** Get the clipboard data.

    While the returned linked_ptr is in scope, the clipboard should be locked.
    See lock_clipboard for details.
*/
linked_ptr<string> get_clipboard(void) {
	if (extclipboard_calls != NULL)
		return extclipboard_calls->get_selection(true);
	return clipboard_data;
}

/** Get the primary selection data.

    While the returned linked_ptr is in scope, the clipboard should be locked.
    See lock_clipboard for details.
*/
linked_ptr<string> get_primary(void) {
	if (extclipboard_calls != NULL)
		return extclipboard_calls->get_selection(false);
	return primary_data;
}

void set_clipboard(string *str) {
	if (str != NULL && str->size() == 0) {
		delete str;
		str = NULL;
	}

	if (extclipboard_calls != NULL) {
		extclipboard_calls->claim_selection(true, str);
		return;
	}
	clipboard_data = str;
}

void set_primary(string *str) {
	if (str != NULL && str->size() == 0) {
		delete str;
		str = NULL;
	}

	if (extclipboard_calls != NULL) {
		extclipboard_calls->claim_selection(false, str);
		return;
	}
	primary_data = str;
}

static void init_external_clipboard(bool init) {
#ifdef WITH_X11
	static lt_dlhandle extclipboard_mod;
#	ifdef WITH_LT_DLADVISE
	lt_dladvise advise;
#	endif
#endif
	if (init_params->disable_external_clipboard)
		return;

	if (init) {
#ifdef WITH_X11
		if (lt_dlinit() != 0)
			return;

#	ifdef WITH_LT_DLADVISE
		if (lt_dladvise_init(&advise) == 0) {
#	endif
			if ((extclipboard_mod = lt_dlopen(X11_MOD_NAME)) == NULL) {
				lprintf("Could not open external clipboard module (X11)\n");
				return;
			}
#	ifdef WITH_LT_DLADVISE
		} else {
			lt_dladvise_local(&advise);
			lt_dladvise_resident(&advise);
			if ((extclipboard_mod = lt_dlopenadvise(X11_MOD_NAME, advise)) == NULL) {
				lprintf("Could not open external clipboard module (X11)\n");
				lt_dladvise_destroy(&advise);
				return;
			}
			lt_dladvise_destroy(&advise);
		}
#	endif

		if ((extclipboard_calls = (extclipboard_interface_t *) lt_dlsym(extclipboard_mod, "_t3_widget_extclipboard_calls")) == NULL) {
			lprintf("External clipboard module does not export interface symbol\n");
			lt_dlclose(extclipboard_mod);
			return;
		}
		if (extclipboard_calls->version != EXTCLIPBOARD_VERSION) {
			lprintf("External clipboard module has incompatible version\n");
			extclipboard_mod = NULL;
			lt_dlclose(extclipboard_mod);
		}
		if (!extclipboard_calls->init()) {
			lprintf("Failed to initialize external clipboard module\n");
			extclipboard_calls = NULL;
			lt_dlclose(extclipboard_mod);
		}
#endif
	} else {
#ifdef WITH_X11
		if (extclipboard_calls != NULL) {
			extclipboard_calls->stop();
			extclipboard_calls = NULL;
		}
		lt_dlclose(extclipboard_mod);
		lt_dlexit();
#endif
	}
}

void release_selections(void) {
	if (extclipboard_calls != NULL)
		extclipboard_calls->release_selections();
}

void lock_clipboard(void) {
	if (extclipboard_calls != NULL)
		extclipboard_calls->lock();
}

void unlock_clipboard(void) {
	if (extclipboard_calls != NULL)
		extclipboard_calls->unlock();
}

void stop_clipboard(void) {
	if (extclipboard_calls != NULL)
		extclipboard_calls->stop();
}

}; // namespace
