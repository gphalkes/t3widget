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
//~ #include <sys/ioctl.h>
//~ #include <sys/types.h>
//~ #include <cstdlib>
//~ #include <cstring>
//~ #include <locale.h>
//~ #include <unistd.h>
//~ #include <climits>
//~ #include <cstdarg>
//~ #ifdef USE_NL_LANGINFO
//~ #include <langinfo.h>
//~ #endif

//~ #include <window.h>

//~ #ifdef DEBUG
//~ #	include <sys/time.h>
//~ #	include <sys/resource.h>
//~ #	include <csignal>
//~ #endif

//~ using namespace std;

//~ #include "keys.h"
//~ #include "files.h"
#include "main.h"
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
window_components_t components;
/* bool UTF8mode;

line_t *copyBuffer = NULL;


MenuItemDef filemenuopts[] = {
	{"_New;nN", ActionID::FILE_NEW, "^N"},
	{"_Open...;oO", ActionID::FILE_OPEN, "^O"},
	{"Open _Recent...;rR", ActionID::FILE_OPEN_RECENT, NULL},
	{"_Close;cC", ActionID::FILE_CLOSE, "^W"},
	{"_Save;sS", ActionID::FILE_SAVE, "^S"},
	{"Save _As...;aA", ActionID::FILE_SAVE_AS, NULL},
	{"-", ActionID::ACTION_NONE, NULL},
	{"Re_draw Screen;dD", ActionID::FILE_REPAINT, NULL},
	{"S_uspend;uU", ActionID::FILE_SUSPEND, NULL},
	{"E_xit;xX", ActionID::FILE_EXIT, "^Q"},
	{NULL, ActionID::ACTION_NONE, NULL}
};

MenuItemDef editmenuopts[] = {
	{"_Undo;uU", ActionID::EDIT_UNDO, "^Z"},
	{"_Redo;rR", ActionID::EDIT_REDO, "^Y"},
	{"-", ActionID::ACTION_NONE, NULL},
	{"_Copy;cC", ActionID::EDIT_COPY, "^C"},
	{"Cu_t;tT", ActionID::EDIT_CUT, "^X"},
	{"_Paste;pP", ActionID::EDIT_PASTE, "^V"},
	{"Select _All;aA", ActionID::EDIT_SELECT_ALL, "^A"},
	{"_Insert Character...;iI", ActionID::EDIT_INSERT_CHAR, "F9"},
	{NULL, ActionID::ACTION_NONE, NULL}
};

MenuItemDef searchmenuopts[] = {
	{"_Find...;fF", ActionID::SEARCH_SEARCH, "^F"},
	{"Find _Next;nN", ActionID::SEARCH_AGAIN, "F3"},
	{"Find _Previous;pP", ActionID::SEARCH_AGAIN_BACKWARD, "S-F3"},
	{"_Replace...;rR", ActionID::SEARCH_REPLACE, "^R"},
	{"_Go to line_t...;gG", ActionID::SEARCH_GOTO, "^G"},
	{NULL, ActionID::ACTION_NONE, NULL}
};

MenuItemDef buffersmenuopts[] = {
	{"_Next Buffer;nN", ActionID::WINDOWS_NEXT_BUFFER, "F6" },
	{"_Previous Buffer;pP", ActionID::WINDOWS_PREV_BUFFER, "S-F6" },
	{"_Select Buffer...;sS", ActionID::WINDOWS_SELECT, NULL},
	{"-", ActionID::ACTION_NONE, NULL},
	{"Split _Horizontal;hH", ActionID::WINDOWS_HSPLIT, NULL},
	{"Split _Vertical;vV", ActionID::WINDOWS_VSPLIT, NULL},
	{"_Close Window;cC", ActionID::WINDOWS_MERGE, NULL},
	{"Next Window", ActionID::WINDOWS_NEXT_WINDOW, "F8"},
	{"Previous Window", ActionID::WINDOWS_PREV_WINDOW, "S-F8"},
	{NULL, ActionID::ACTION_NONE, NULL}
};

MenuItemDef optionsmenuopts[] = {
	{"_Tabs...;tT", ActionID::OPTIONS_TABS, NULL},
	{"_Keys...;kK", ActionID::OPTIONS_KEYS, NULL},
	{NULL, ActionID::ACTION_NONE, NULL}
};

MenuItemDef helpmenuopts[] = {
	{"_Help;hH", ActionID::HELP_HELP, "F1"},
	{"_About;aA", ActionID::HELP_ABOUT, NULL},
	{NULL, ActionID::ACTION_NONE, NULL}
};

MenuDef menus[] = {
	{"_File;fF", filemenuopts},
	{"_Edit;eE", editmenuopts},
	{"_Search;sS", searchmenuopts},
	{"_Windows;wW", buffersmenuopts},
	{"_Options;oO", optionsmenuopts},
	{"_Help;hH", helpmenuopts},
	{NULL, NULL}
};



static bool makeWindows(void) {
	edit_window_t *edit = new edit_window_t(screenLines - (option.hide_menubar ? 0 : 1), screenColumns, option.hide_menubar ? 0 : 1, 0, NULL);
	editwin = new EditSplit(edit, true);

	menubar = new menu_bar_t(menus, screenColumns);
	openFileDialog = new open_file_dialog_t(screenLines - 4, (screenColumns - 4) & (~1));
	saveAsDialog = new save_as_dialog_t(screenLines - 4, (screenColumns - 4) & (~1));
	errorDialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, screenLines / 3, (screenColumns - MESSAGE_DIALOG_WIDTH) / 2, "Error");
	messageDialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, screenLines / 3, (screenColumns - MESSAGE_DIALOG_WIDTH) / 2, "Message");
	continueAbortDialog = new question_dialog_t(MESSAGE_DIALOG_WIDTH, screenLines / 3, (screenColumns - MESSAGE_DIALOG_WIDTH) / 2, "Question", "_Continue;cC", "_Abort;aA");
	findDialog = new find_dialog_t();
	replaceDialog = new find_dialog_t(true);
	insertCharDialog = new insert_char_dialog_t();
	closeConfirmDialog = new close_confirm_dialog_t();
	altMessageDialog = new alt_message_dialog_t("Press key to combine with meta");
	gotoDialog = new goto_dialog_t();
	overwriteConfirmDialog = new overwrite_confirm_dialog_t();
	replaceButtonsDialog = new replace_buttons_dialog_t();
	openRecentDialog = new open_recent_dialog_t();
	selectBufferDialog = new select_buffer_dialog_t();
	encodingDialog = new encoding_dialog_t();
	return true;
}
*/
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
/*
void activate_window(WindowID id, ...) {
	va_list args;
	va_start(args, id);

	components.back()->set_focus(false);
	switch (id) {
		case WindowID::MENU:
			if (!menubar->activate(*va_arg(args, const key_t *)))
				break;

			components.push_back(menubar);
			break;
		case WindowID::OPEN_FILE:
			openFileDialog->set_file("");
			encodingDialog->set_encoding(NULL);
			components.push_back(openFileDialog);
			waitCharacterSetInitialization();
			break;
		case WindowID::SAVE_FILE:
			saveAsDialog->set_file(editwin->get_current()->get_text_file()->get_name());
			encodingDialog->set_encoding(editwin->get_current()->get_text_file()->get_encoding());
			components.push_back(saveAsDialog);
			waitCharacterSetInitialization();
			break;
		case WindowID::ERROR_DIALOG:
			errorDialog->set_message(va_arg(args, const string *));
			components.push_back(errorDialog);
			break;
		case WindowID::MESSAGE_DIALOG:
			messageDialog->set_message(va_arg(args, const string *));
			components.push_back(messageDialog);
			break;
		case WindowID::CONTINUE_ABORT_DIALOG:
			continueAbortDialog->set_message(va_arg(args, const string *));
			continueAbortDialog->set_actions(ActionID::CALL_CONTINUATION, ActionID::DROP_CONTINUATION, va_arg(args, Continuation *));
			components.push_back(continueAbortDialog);
			break;
		case WindowID::FIND:
			components.push_back(findDialog);
			break;
		case WindowID::REPLACE:
			components.push_back(replaceDialog);
			break;
		case WindowID::INSERT_CHAR:
			components.push_back(insertCharDialog);
			break;
		case WindowID::CLOSE_CONFIRM: {
			const char *title = va_arg(args, const char *);
			if (title == NULL)
				title = "(Untitled)";
			closeConfirmDialog->set_file_name(title);
			components.push_back(closeConfirmDialog);
			break;
		}
		case WindowID::ALTMESSAGE_DIALOG:
			components.push_back(altMessageDialog);
			break;
		case WindowID::GOTO_DIALOG:
			components.push_back(gotoDialog);
			break;
		case WindowID::OVERWRITE_CONFIRM:
			overwriteConfirmDialog->set_state(va_arg(args, SaveState *));
			components.push_back(overwriteConfirmDialog);
			break;
		case WindowID::REPLACE_BUTTONS:
			components.push_back(replaceButtonsDialog);
			break;
		case WindowID::OPEN_RECENT:
			components.push_back(openRecentDialog);
			break;
		case WindowID::SELECT_BUFFER:
			components.push_back(selectBufferDialog);
			break;
		case WindowID::ENCODING:
			components.push_back(encodingDialog);
			break;
		default:
			break;
	}
	components.back()->set_show(true);
	components.back()->set_focus(true);

	va_end(args);
}
 */

void activate_window(dialog_t *dialog) {
	components.back()->set_focus(false);
	components.back()->set_show(false);
	#warning FIXME: this should change the window,s depth to make sure it appears above all other dialogs
	components.push_back(dialog);
	components.back()->set_show(true);
	components.back()->set_focus(true);
}

void deactivate_window(void) {
	if (components.size() > 1) {
		components.back()->set_focus(false);
		components.back()->set_show(false);
		components.pop_back();
		components.back()->set_focus(true);
	}
}

void iterate(void) {
	key_t key;

	for (window_components_t::iterator iter = components.begin(); iter != components.end(); iter++)
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
			components.back()->process_key(key);
			break;
	}
}

void main_loop(void) {
	while (true) {
		iterate();
	}
}
