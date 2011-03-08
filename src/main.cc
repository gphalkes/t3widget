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

//~ #include "keys.h"
//~ #include "textbuffer.h"
#include "main.h"
#include "log.h"
#include "colorscheme.h"
#include "widgets/widgets.h"
#include "dialogs/dialogs.h"
#include "lines.h"

using namespace std;
using namespace sigc;

namespace t3_widget {
//~ #include "log.h"
//~ #include "options.h"
//~ #include "util.h"
//~ #include "lines.h"
//~ #include "editwindow.h"
//~ #include "editsplit.h"
//~ #include "lines.h"
//~ #include "charactersets.h"
//~ #include "key/key.h"
//~ #include "dialogs/dialogs.h"

//~ static EditSplit *editwin;
//~ static menu_bar_t *menubar;
//~ static file_dialog_t *openFileDialog, *saveAsDialog;
//~ static message_dialog_t *errorDialog, *messageDialog;
//~ static question_dialog_t *continueAbortDialog;
//~ static find_dialog_t *findDialog, *replaceDialog;
//~ static insert_char_dialog_t *insertCharDialog;
//~ static close_confirm_dialog_t *closeConfirmDialog;
//~ static alt_message_dialog_t *altMessageDialog;
//~ static goto_dialog_t *gotoDialog;
//~ static overwrite_confirm_dialog_t *overwriteConfirmDialog;
//~ static replace_buttons_dialog_t *replaceButtonsDialog;
//~ static open_recent_dialog_t *openRecentDialog;
//~ static select_buffer_dialog_t *selectBufferDialog;
//~ static encoding_dialog_t *encodingDialog;
//~ OpenFiles openFiles;
//~ RecentFiles recentFiles;

//~ static const string *findText;


#define MESSAGE_DIALOG_WIDTH 50
#define MIN_LINES 16
#define MIN_COLUMNS 60


static int screen_lines, screen_columns;
text_line_t *copy_buffer;

static signal<void, int, int> resize;

connection connect_resize(const slot<void, int, int> &_slot) {
	return resize.connect(_slot);
}

void do_resize(void) {
	int new_screen_lines, new_screen_columns;
	bool need_redraw = false;

	t3_term_resize();
	t3_term_get_size(&new_screen_lines, &new_screen_columns);
	if (new_screen_lines == screen_lines && new_screen_columns == screen_columns)
		return;

	if (new_screen_lines < screen_lines || new_screen_columns < screen_columns)
		need_redraw = true;

	// Ensure minimal coordinates to maintain sort of sane screen layout
	screen_lines = new_screen_lines < MIN_LINES ? MIN_LINES : new_screen_lines;
	screen_columns = new_screen_columns < MIN_COLUMNS? MIN_COLUMNS : new_screen_columns;

	resize(screen_lines, screen_columns);

	if (need_redraw)
		t3_term_redraw();
}

#if 0
typedef enum {
	TERM_NONE,
	TERM_XTERM
} TerminalCode;

typedef struct {
	const char *name;
	TerminalCode code;
} TerminalMapping;

TerminalMapping terminalMapping[] = {
	{"xterm", TERM_XTERM},
	{NULL, TERM_NONE}
};

TerminalCode terminal;

static void terminalSpecificRestore(void) {
	switch (terminal) {
		case TERM_XTERM:
			/* Note: this may not actually reset to previous value, because of broken xterm */
			t3_term_putp("\033[?1036r");
			break;
		default:
			break;
	}
}

static void terminalSpecificSetup(void) {
	const char *term;
	int i;

	term = getenv("TERM");
	for (i = 0; terminalMapping[i].name != NULL && strcmp(terminalMapping[i].name, term) != 0; i++) {}
	terminal = terminalMapping[i].code;

	switch (terminal) {
		case TERM_XTERM:
			t3_term_putp("\033[?1036s\033[?1036h");
			break;
		default:
			return;
	}
	atexit(terminalSpecificRestore);
}
#endif

#warning FIXME: returning the value from t3_term_init is not very useful!
int init(main_window_t *main_window) {
	int result;

	init_log();
	init_colors();
	text_line_t::init();

	if ((result = t3_term_init(-1, NULL)) != T3_ERR_SUCCESS) {
		t3_term_restore();
		return result;
	}
	atexit(t3_term_restore);
	init_keys();
	do_resize();
	dialog_t::init(main_window);
	return 0;
}

void iterate(void) {
	key_t key;

	for (dialogs_t::iterator iter = dialog_t::dialogs.begin(); iter != dialog_t::dialogs.end(); iter++)
		(*iter)->update_contents();

	t3_term_update();
	key = wreadkey();

	switch (key) {
		case EKEY_RESIZE:
			do_resize();
			break;
		case EKEY_META | EKEY_ESC:
#warning FIXME: how do we handle the whole alt-message mess?
			//activate_window(WindowID::ALTMESSAGE_DIALOG);
			break;
		default:
			dialog_t::dialogs.back()->process_key(key);
			break;
	}
}

void main_loop(void) {
	while (true) {
		iterate();
	}
}

}; // namespace
