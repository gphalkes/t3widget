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
//~ #include "files.h"
#include "main.h"
#include "colorscheme.h"
#include "widgets/widgets.h"
#include "dialogs/dialogs.h"
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


int screenLines, screenColumns;
line_t *copy_buffer;

bool doResize(void) {
	int newScreenLines, newScreenColumns;
	bool result = true, needRedraw = false;

	t3_term_resize();
	t3_term_get_size(&newScreenLines, &newScreenColumns);
	if (newScreenLines == screenLines && newScreenColumns == screenColumns)
		return 0;

	if (newScreenLines < screenLines || newScreenColumns < screenColumns)
		needRedraw = true;

	// Ensure minimal coordinates to maintain sort of sane screen layout
	screenLines = newScreenLines < MIN_LINES ? MIN_LINES : newScreenLines;
	screenColumns = newScreenColumns < MIN_COLUMNS? MIN_COLUMNS : newScreenColumns;
/*
	result &= menubar->resize(1, screenColumns, 0, 0);
	result &= editwin->resize(screenLines - 1, screenColumns, 1, 0);
	result &= openFileDialog->resize(screenLines - 4, (screenColumns - 4) & (~1), None, None);
	result &= saveAsDialog->resize(screenLines - 4, (screenColumns - 4) & (~1), None, None);
	result &= errorDialog->resize(None, MESSAGE_DIALOG_WIDTH, screenLines / 3, (screenColumns - MESSAGE_DIALOG_WIDTH) / 2);
	result &= messageDialog->resize(None, MESSAGE_DIALOG_WIDTH, screenLines / 3, (screenColumns - MESSAGE_DIALOG_WIDTH) / 2);
	result &= continueAbortDialog->resize(None, MESSAGE_DIALOG_WIDTH, screenLines / 3, (screenColumns - MESSAGE_DIALOG_WIDTH) / 2);
	result &= findDialog->resize(None, None, None, None);
	result &= replaceDialog->resize(None, None, None, None);
	result &= insertCharDialog->resize(None, None, None, None);
	result &= closeConfirmDialog->resize(None, None, None, None);
	result &= altMessageDialog->resize(None, None, None, None);
	result &= gotoDialog->resize(None, None, None, None);
	result &= overwriteConfirmDialog->resize(None, None, None, None);
	result &= replaceButtonsDialog->resize(None, None, None, None);
	result &= openRecentDialog->resize(None, None, None, None);
	result &= encodingDialog->resize(None, None, None, None);
*/
	if (needRedraw)
		t3_term_redraw();

	return result;
}

void iterate(void) {
	key_t key;

	for (dialogs_t::iterator iter = dialog_t::dialogs.begin(); iter != dialog_t::dialogs.end(); iter++)
		(*iter)->update_contents();

	t3_term_update();
	key = wreadkey();

	switch (key) {
		case EKEY_RESIZE:
			if (doResize() < 0) {
/*FIXME: this is to abrupt. It should show an error message or just use
	the current window settings */
			}
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

void init(void) {
	init_colors();
}
