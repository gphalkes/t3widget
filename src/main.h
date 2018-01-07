/* Copyright (C) 2011-2012 G.P. Halkes
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
#ifndef T3_WIDGET_MAIN_H
#define T3_WIDGET_MAIN_H

#include <t3widget/util.h>
#include <t3widget/dialogs/dialog.h>
#include <t3widget/dialogs/insertchardialog.h>
#include <t3widget/dialogs/messagedialog.h>

namespace t3_widget {

/** The version of libt3widget encoded as a single integer.

    The least significant 8 bits represent the patch level.
    The second 8 bits represent the minor version.
    The third 8 bits represent the major version.

	At runtime, the value of T3_WIDGET_VERSION can be retrieved by calling
	#get_version.

    @internal
    The value 0 is an invalid value which should be replaced by the script
    that builds the release package.
*/
#define T3_WIDGET_VERSION 0

/** A class representing an error from one of the supporting libraries. */
class T3_WIDGET_API complex_error_t {
	public:
		enum source_t{
			SRC_NONE,
			SRC_ERRNO,
			SRC_TRANSCRIPT,
			SRC_T3_KEY,
			SRC_T3_WINDOW
		};
	private:
		bool success;
		source_t source;
		int error;
		const char *file_name;
		int line_number;

	public:
		complex_error_t(void);
		complex_error_t(source_t _source, int _error, const char *_file_name = NULL, int _line_number = 0);
		void set_error(source_t _source, int _error, const char *_file_name = NULL, int _line_number = 0);
		bool get_success(void);
		source_t get_source(void);
		int get_error(void);
		const char *get_string(void);
};

/** Structure holding the parameters for initialization for libt3widget.
*/
class T3_WIDGET_API init_parameters_t {
	public:
		const char *program_name; /**< Name of the program to print where appropriate. */
		const char *term; /**< Override the terminal name derived from @c TERM. */
		/** Boolean indicating whether keypad keys are returned as separate from the regular cursor control keys.

			If @c false, there will be no distinction between the user pressing e.g.
			left arrow and keypad left arrow. This is the recommended behavior. */
		bool separate_keypad;
		/** Boolean indicating whether to explicitly disable the external clipboard.

		    The external clipboard is (at the time of this writing) the X11 clipboard.
		    In some cases it may be desirable to disable the X11 interface, even though
		    we may be able to connect to it. For example, if it is connected over a
		    slow link. */
		bool disable_external_clipboard;

		/** Construct a new init_parameters_t object. */
		static init_parameters_t *create(void);
	private:
		init_parameters_t(void);
};

//FIXME: shouldn't these be internal?
/** Global insert_char_dialog_t dialog. */
T3_WIDGET_API extern insert_char_dialog_t *insert_char_dialog;
/** Global message_dialog_t dialog. */
T3_WIDGET_API extern message_dialog_t *message_dialog;

/** Connect a callback to the @c resize signal. */
T3_WIDGET_API signals::connection connect_resize(const signals::slot<void, int, int> &slot);
/** Connect a callback to the @c update_notification signal.
    The @c update_notification signal is sent in response to #signal_update function.
*/
T3_WIDGET_API signals::connection connect_update_notification(const signals::slot<void> &slot);
/** Connect a callback to the @c on_init signal.
    The @c on_init signal is emitted after initialization is complete. The signal
    is provided to allow initialization of global variables after initialization
    is complete, but without knowledge of when #init is called.
*/
T3_WIDGET_API signals::connection connect_on_init(const signals::slot<void, bool> &slot);
/** Connect a callback to the @c terminal_settings_changed signal.
    The @c terminal_settings_changed signal is emitted when the libt3window
    library has completed the terminal capability detection.
*/
T3_WIDGET_API signals::connection connect_terminal_settings_changed(const signals::slot<void> &slot);

/** Initialize the libt3widget library.

    This function should be called before any other function in the libt3widget
    library.
*/
T3_WIDGET_API complex_error_t init(const init_parameters_t *params);
/** Function to restore the terminal to the original settings.
    This function is called automatically on program termination by use of
    @c atexit(3).
*/
T3_WIDGET_API void restore(void);
/** Perform a single iteration of the main loop.
    This function updates the contents of the terminal, waits for a key press
	and sends it to the currently focussed dialog. Called repeatedly from
    #main_loop.
*/
T3_WIDGET_API void iterate(void);
/** Run the main event loop of the libt3widget library.
    This function will return only by calling #exit_main_loop, yielding the
    value passed to that function.
*/
T3_WIDGET_API int main_loop(void);
/** Suspend execution of this program by sending a @c SIGSTOP signal.
    Before sending the @c SIGSTOP signal, the terminal is reset to its original
    state. This allows the parent process (usually the shell) to continue
    running, while the current program is temporarily suspended.
*/
T3_WIDGET_API void suspend(void);
/** Force a complete redraw of the terminal contents.
    The terminal contents may get corrupted due to the output from other
    processes. The only remedy is to clear the terminal and redraw it, which is
    exactly what this function does.
*/
T3_WIDGET_API void redraw(void);

/** Exit the main loop.
    Calling this function will cause an exit from the main loop. This is
    accomplished by throwing an exception, so using an unqualified @c catch
    clause will interfere with the execution of this action.
*/
T3_WIDGET_API void exit_main_loop(int exit_code)
#ifdef __GNUC__
__attribute__((noreturn))
#endif
;

/** Exit the main loop from any thread or signal handler.
    Calling this function will (eventually) cause an exit from the main loop.
    "Eventually" meaning that a key press event is inserted in the key buffer,
    which will cause an exit from the program.
*/
T3_WIDGET_API void async_safe_exit_main_loop(int exit_code);

/** Free memory used by libt3widget.
    After this function is called, no further calls to libt3widget should be
    made. Normally, there is no reason to call this function at all, because the
    operating system will reclaim all memory. However, if you are trying to
    track down memory leaks in your program, calling this function will free
    all memory allocated in libt3widget, thus allowing you to debug your own
    memory allocations more easily.
*/
T3_WIDGET_API void cleanup(void);

/** Control the color mode.
    libt3widget by default starts in black and white mode, as most terminals
    support the limited attributes required. This function allows switching to
    and from color mode. If the terminal does not support (enough) colors,
    switching to color mode will not do anything and @c false will be returned.
*/
T3_WIDGET_API bool set_color_mode(bool on);
/** Change the setting of a default attribute.
    See the @ref attribute_t enum for possible values.
*/
T3_WIDGET_API void set_attribute(attribute_t attribute, t3_attr_t value);
/** Retrieve the setting of a default attribute.
    See the @ref attribute_t enum for possible values.
*/
T3_WIDGET_API t3_attr_t get_attribute(attribute_t attribute);
/** Retrieve the default setting of a default attribute.
    See the @ref attribute_t enum for possible values.
*/
T3_WIDGET_API t3_attr_t get_default_attribute(attribute_t attribute, bool color_mode);
/** Get the version of the libt3widget library used at runtime. */
T3_WIDGET_API long get_version(void);

/** Get the version of the libt3key library used by libt3widget at runtime. */
T3_WIDGET_API long get_libt3key_version(void);

/** Get the version of the libt3window library used by libt3widget at runtime. */
T3_WIDGET_API long get_libt3window_version(void);

/** Get the dimensions of the screen as used by libt3widget. */
T3_WIDGET_API void get_screen_size(int *height, int *width);

/** Set the handling of the primary selection to on or off.

    When using a clipboard mananger which also tracks the primary selection,
    updating the selection will result in the clipboard mananger requesting
    the data and claiming the primary selection on every key press. This is
    fine when running on the same machine, but not when running remotely. In
    those cases it is desirable to turn off the primary selection.

    This option does _not_ disable pasting the primary selection. */
T3_WIDGET_API void set_primary_selection_mode(bool on);
}; // namespace

#endif
