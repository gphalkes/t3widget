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

static extclipboard_interface_t *extclipboard_calls;

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

void init_external_clipboard(void) {
#ifdef WITH_X11
	lt_dlhandle x11_mod;
	lt_dladvise advise;

	if (lt_dlinit() != 0)
		return;

	if (lt_dladvise_init(&advise) == 0) {
		if ((x11_mod = lt_dlopen(X11_MOD_NAME)) == NULL) {
			lprintf("Could not open X11 module\n");
			return;
		}
	} else {
		lt_dladvise_local(&advise);
		lt_dladvise_resident(&advise);
		if ((x11_mod = lt_dlopenadvise(X11_MOD_NAME, advise)) == NULL) {
			lt_dladvise_destroy(&advise);
			return;
		}
		lt_dladvise_destroy(&advise);
	}

	if ((extclipboard_calls = (extclipboard_interface_t *) lt_dlsym(x11_mod, "_t3_widget_extclipboard_calls")) == NULL) {
		lt_dlclose(x11_mod);
		return;
	}
	extclipboard_calls->init();
#endif
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

}; // namespace
