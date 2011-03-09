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
#include <csignal>
#include <cstring>
#include <climits>
#include <stdint.h>
#include <langinfo.h>
#include <charconv.h>

#include <window/window.h>

#include "keys.h"
#include "util.h"
#include "main.h"
#include "key.h"
#include "keybuffer.h"

namespace t3_widget {

#define MAX_SEQUENCE 100

enum {
	WINCH_SIGNAL,
	QUIT_SIGNAL
};

typedef struct {
	const char *string;
	key_t code;
} key_string_t;

typedef struct {
	const char *string;
	size_t string_length;
	key_t key;
} mapping_t;

key_string_t key_strings[] = {
	{"home", EKEY_HOME},
	{"end", EKEY_END},
	{"page_up", EKEY_PGUP},
	{"page_down", EKEY_PGDN},
	{"left", EKEY_LEFT},
	{"down", EKEY_DOWN},
	{"right", EKEY_RIGHT},
	{"up", EKEY_UP},

	{"home+s", EKEY_HOME | EKEY_SHIFT},
	{"end+s", EKEY_END | EKEY_SHIFT},
	{"page_up+s", EKEY_PGUP | EKEY_SHIFT},
	{"page_down+s", EKEY_PGDN | EKEY_SHIFT},
	{"left+s", EKEY_LEFT | EKEY_SHIFT},
	{"down+s", EKEY_DOWN | EKEY_SHIFT},
	{"right+s", EKEY_RIGHT | EKEY_SHIFT},
	{"up+s", EKEY_UP | EKEY_SHIFT},

	{"home+c", EKEY_HOME | EKEY_CTRL},
	{"end+c", EKEY_END | EKEY_CTRL},
	{"page_up+c", EKEY_PGUP | EKEY_CTRL},
	{"page_down+c", EKEY_PGDN | EKEY_CTRL},
	{"left+c", EKEY_LEFT | EKEY_CTRL},
	{"down+c", EKEY_DOWN | EKEY_CTRL},
	{"right+c", EKEY_RIGHT | EKEY_CTRL},
	{"up+c", EKEY_UP | EKEY_CTRL},

	{"home+cs", EKEY_HOME | EKEY_CTRL | EKEY_SHIFT},
	{"end+cs", EKEY_END | EKEY_CTRL | EKEY_SHIFT},
	{"page_up+cs", EKEY_PGUP | EKEY_CTRL | EKEY_SHIFT},
	{"page_down+cs", EKEY_PGDN | EKEY_CTRL | EKEY_SHIFT},
	{"left+cs", EKEY_LEFT | EKEY_CTRL | EKEY_SHIFT},
	{"down+cs", EKEY_DOWN | EKEY_CTRL | EKEY_SHIFT},
	{"right+cs", EKEY_RIGHT | EKEY_CTRL | EKEY_SHIFT},
	{"up+cs", EKEY_UP | EKEY_CTRL | EKEY_SHIFT},

	{"delete", EKEY_DEL},
	{"insert", EKEY_INS},
	{"f1", EKEY_F1},
	{"f2", EKEY_F2},
	{"f3", EKEY_F3},
	{"f4", EKEY_F4},
	{"f5", EKEY_F5},
	{"f6", EKEY_F6},
	{"f7", EKEY_F7},
	{"f8", EKEY_F8},
	{"f9", EKEY_F9},
	{"f10", EKEY_F10},
	{"f11", EKEY_F11},
	{"f12", EKEY_F12},

	{"f3+s", EKEY_F3 | EKEY_SHIFT},
	{"f6+s", EKEY_F6 | EKEY_SHIFT},
	{"f8+s", EKEY_F8 | EKEY_SHIFT},
	{"backspace", EKEY_BS},

	{"kp_home", EKEY_HOME},
	{"kp_end", EKEY_END},
	{"kp_page_up", EKEY_PGUP},
	{"kp_page_down", EKEY_PGDN},
	{"kp_left", EKEY_LEFT},
	{"kp_down", EKEY_DOWN},
	{"kp_right", EKEY_RIGHT},
	{"kp_up", EKEY_UP},

	{"kp_home+s", EKEY_HOME | EKEY_SHIFT},
	{"kp_end+s", EKEY_END | EKEY_SHIFT},
	{"kp_page_up+s", EKEY_PGUP | EKEY_SHIFT},
	{"kp_page_down+s", EKEY_PGDN | EKEY_SHIFT},
	{"kp_left+s", EKEY_LEFT | EKEY_SHIFT},
	{"kp_down+s", EKEY_DOWN | EKEY_SHIFT},
	{"kp_right+s", EKEY_RIGHT | EKEY_SHIFT},
	{"kp_up+s", EKEY_UP | EKEY_SHIFT},

	{"kp_delete", EKEY_DEL},
	{"kp_insert", EKEY_INS},

	{"insert+s", EKEY_INS | EKEY_SHIFT},
	{"kp_insert+s", EKEY_INS | EKEY_SHIFT},
	{"delete+s", EKEY_DEL | EKEY_SHIFT},
	{"kp_delete+s", EKEY_INS | EKEY_SHIFT},

	{"insert+c", EKEY_INS | EKEY_CTRL},
	{"kp_insert+c", EKEY_INS | EKEY_CTRL},

	{"tab", '\t'},
	{"tab+s", '\t' | EKEY_SHIFT},

	{"kp_enter", EKEY_NL},
	{NULL, 0}
};

static mapping_t *map;
static int map_count;
static key_t map_single[128];

static const char *leave, *enter;
static const t3_key_node_t *keymap;
static int signal_pipe[2] = { -1, -1 };
static key_t protected_insert;

static key_buffer_t key_buffer;
static pthread_t read_key_thread;

static char char_buffer[32];
static int char_buffer_fill;
static uint32_t unicode_buffer[16];
static int unicode_buffer_fill;
static charconv_t *conversion_handle;

static int key_timeout = -1;

static key_t decode_sequence(bool outer);
static void stop_keys(void);

static key_t get_next_converted_key(void) {
	if (unicode_buffer_fill > 0) {
		key_t c = unicode_buffer[0];
		unicode_buffer_fill--;
		memmove(unicode_buffer, unicode_buffer + 1, sizeof(unicode_buffer[0]) * unicode_buffer_fill);
		return c;
	}
	return /* FIXME */ -1;
}

static void unget_key(key_t c) {
	memmove(unicode_buffer + 1, unicode_buffer, sizeof(unicode_buffer[0]) * unicode_buffer_fill);
	unicode_buffer[0] = c;
	unicode_buffer_fill++;
}

static int read_and_convert_keys(int timeout) {
	const char *char_buffer_ptr;
	uint32_t *unicode_buffer_ptr;
	key_t c;

	c = t3_term_get_keychar(timeout);
	if (c < 0)
		return -1;

	char_buffer[char_buffer_fill++] = (char) c;
	char_buffer_ptr = char_buffer;
	unicode_buffer_ptr = unicode_buffer;

	while (1) {
		switch (charconv_to_unicode(conversion_handle, &char_buffer_ptr, char_buffer + char_buffer_fill,
				(char **) &unicode_buffer_ptr, (const char *) (&unicode_buffer) + sizeof(unicode_buffer), CHARCONV_ALLOW_FALLBACK))
		{
			case CHARCONV_SUCCESS:
			case CHARCONV_NO_SPACE:
			case CHARCONV_INCOMPLETE:
				char_buffer_fill -= char_buffer_ptr - char_buffer;
				if (char_buffer_fill != 0)
					memmove(char_buffer, char_buffer_ptr, char_buffer_fill);
				unicode_buffer_fill = unicode_buffer_ptr - unicode_buffer;
				return 0;

			case CHARCONV_FALLBACK: // NOTE: we allow fallbacks, so this should not even occur!!!

			case CHARCONV_UNASSIGNED:
			case CHARCONV_ILLEGAL:
			case CHARCONV_ILLEGAL_END:
			case CHARCONV_INTERNAL_ERROR:
			case CHARCONV_PRIVATE_USE:
				charconv_to_unicode_skip(conversion_handle, &char_buffer_ptr, char_buffer + char_buffer_fill);
				break;
			default:
				// This shouldn't happen, and we can't really do anything with this.
				return 0;
		}
	}
}


static void *read_keys(void *arg) {
	int retval;
	key_t c;
	fd_set readset;

	(void) arg;

	while (1) {
		FD_ZERO(&readset);
		FD_SET(0, &readset);
		FD_SET(signal_pipe[0], &readset);

		retval = select(signal_pipe[0] + 1, &readset, NULL, NULL, NULL);

		if (retval < 0) //FIXME: prevent infinite looping on error condition
			continue;

		if (FD_ISSET(signal_pipe[0], &readset)) {
			char command;

			nosig_read(signal_pipe[0], &command, 1);
			switch (command) {
				case QUIT_SIGNAL:
					/* Exit thread */
					return NULL;
				case WINCH_SIGNAL:
					key_buffer.push_back(EKEY_RESIZE);
					break;
				default:
					// This should be impossible, so just ignore
					continue;
			}
		}

		if (FD_ISSET(0, &readset))
			read_and_convert_keys(-1);

		while ((c = get_next_converted_key()) >= 0) {
			if (c == EKEY_ESC) {
				c = decode_sequence(true);
			} else if (c > 0 && c < 128 && map_single[c] != 0) {
				c = map_single[c];
			}
			key_buffer.push_back(c);
		}
	}
}

key_t read_key(void) {
	if (protected_insert) {
		int c = protected_insert;
		protected_insert = 0;
		return c;
	}
	return key_buffer.pop_front();
}

static key_t decode_sequence(bool outer) {
	int sequence_index = 1;
	key_t sequence[MAX_SEQUENCE];
	int c, i, j;
	bool is_prefix;

	sequence[0] = EKEY_ESC;

	while (sequence_index < MAX_SEQUENCE) {
		while ((c = get_next_converted_key()) >= 0) {
			if (c == EKEY_ESC) {
				if (key_timeout < 0) {
					if (sequence_index == 1)
						return EKEY_ESC;
				} else {
					if (sequence_index == 1 && outer) {
						key_t alted = decode_sequence(false);
						return alted >= 0 ? alted | EKEY_META : EKEY_ESC;
					}
				}
				unget_key(c);
				break;
			}

			sequence[sequence_index++] = c;

			is_prefix = false;
			for (i = 0; i < map_count; i++) {
				if (sequence_index > (int) map[i].string_length || (key_timeout >= 0 && sequence_index != (int) map[i].string_length))
					continue;

				for (j = 0; j < sequence_index; j++)
					if (sequence[j] != map[i].string[j])
						goto check_next_sequence;

				if (j == (int) map[i].string_length)
					return map[i].key;

				if (j == sequence_index)
					is_prefix = true;

		check_next_sequence:;
			}
			if (key_timeout < 0 && !is_prefix)
				goto ignore_sequence;
		}

		if (read_and_convert_keys(key_timeout) < 0)
			break;
	}

ignore_sequence:
	if (sequence_index == 2)
		return sequence[1] | EKEY_META;
	else if (sequence_index == 1)
		return EKEY_ESC;

	/* Something unwanted has happened here: the character sequence we encoutered was not
	   in our key map. Because it will give some undesired result to just return
	   <alt><first character> we ignore the whole sequence */
	return -1;
}

void insert_protected_key(key_t key) {
	protected_insert = key | EKEY_PROTECT;
}

static void sigwinch_handler(int param) {
	char winch_signal = WINCH_SIGNAL;
	(void) param;
	nosig_write(signal_pipe[1], &winch_signal, 1);
}

//FIXME: return value can not be a simple int, because of the different ranges of values it needs to support
#define RETURN_ERROR(_x) do { error = (_x); goto return_error; } while (0)
/* Initialize the key map */
int init_keys(void) {
	struct sigaction sa;
	sigset_t sigs;
	const t3_key_node_t *key_node;
	int i, error, idx;
	charconv_error_t charconv_error;

	/* Start with things most likely to fail */
	if ((conversion_handle = charconv_open_convertor(nl_langinfo(CODESET), CHARCONV_UTF32, 0, &charconv_error)) == NULL)
		RETURN_ERROR(charconv_error); //FIXME: may overlap with t3_key_load_map errors

	if ((keymap = t3_key_load_map(NULL, NULL, &error)) == NULL)
		RETURN_ERROR(error); //FIXME: may overlap with charconv error codes!

	if (pipe(signal_pipe) < 0)
		RETURN_ERROR(-1); //FIXME: proper error code please

	sa.sa_handler = sigwinch_handler;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGWINCH);
	sa.sa_flags = 0;

	if (sigaction(SIGWINCH, &sa, NULL) < 0)
		RETURN_ERROR(-1); //FIXME: proper error code please

	sigemptyset(&sigs);
	sigaddset(&sigs, SIGWINCH);
	if (sigprocmask(SIG_UNBLOCK, &sigs, NULL) < 0)
		RETURN_ERROR(-1); //FIXME: proper error code please

	for (i = 1; i < 26; i++)
		map_single[i] = EKEY_CTRL | ('a' + i - 1);
	/* "unmap" TAB */
	map_single[(int) '\t'] = 0;
	/* EKEY_ESC is defined as 27, so no need to map */
	map_single[28] = EKEY_CTRL | '\\';
	map_single[29] = EKEY_CTRL | ']';
	map_single[30] = EKEY_CTRL | '_';
	map_single[31] = EKEY_CTRL | '^';

	/* Add a few default keys which will help on terminals with little information
	   available. */
	map_single[8] = EKEY_BS;
	map_single[127] = EKEY_BS;
	map_single[10] = EKEY_NL;
	map_single[13] = EKEY_NL;

	if ((key_node = t3_key_get_named_node(keymap, "%enter")) != NULL) {
		t3_term_putp(key_node->string);
		enter = key_node->string;
	}
	if ((key_node = t3_key_get_named_node(keymap, "%leave")) != NULL)
		leave = key_node->string;


	/* Load all the needed keys from the terminfo database */
	for (i = 0; key_strings[i].code != 0; i++) {
		for (key_node = t3_key_get_named_node(keymap, key_strings[i].string);
				key_node != NULL; key_node = t3_key_get_named_node(key_node, NULL))
		{
			if (key_node->string[0] == 27)
				map_count++;
		}
	}
	if ((map = (mapping_t *) malloc(sizeof(mapping_t) * map_count)) == NULL)
		RETURN_ERROR(T3_ERR_OUT_OF_MEMORY);

	for (i = 0, idx = 0; key_strings[i].code != 0; i++) {
		for (key_node = t3_key_get_named_node(keymap, key_strings[i].string);
				key_node != NULL; key_node = t3_key_get_named_node(key_node, NULL))
		{
			if (key_node->string[0] == 27) {
				map[idx].string = key_node->string;
				map[idx].string_length = key_node->string_length;
				map[idx++].key = key_strings[i].code;
			} else if (strlen(key_node->string) == 1) {
				map_single[(unsigned char) key_node->string[0]] = key_strings[i].code;
			}
		}
	}

	if ((error = pthread_create(&read_key_thread, NULL, read_keys, NULL)) != 0)
		RETURN_ERROR(error);

	if (leave != NULL)
		atexit(stop_keys);
	return T3_ERR_SUCCESS;

return_error:
	if (conversion_handle != NULL)
		charconv_close_convertor(conversion_handle);
	if (keymap != NULL)
		t3_key_free_map(keymap);
	if (signal_pipe[0] != -1) {
		close(signal_pipe[0]);
		close(signal_pipe[1]);
	}

	return error;
}
#undef RETURN_ERROR

void deinit_keys(void) {
	t3_term_putp(leave);
}

void reinit_keys(void) {
	t3_term_putp(enter);
}

static void stop_keys(void) {
	void *retval;
	char quit_signal = QUIT_SIGNAL;
	nosig_write(signal_pipe[1], &quit_signal, 1);
	pthread_join(read_key_thread, &retval);
	t3_term_putp(leave);
#ifdef DEBUG
	t3_key_free_map(keymap);
#endif
}

void set_key_timeout(int msec) {
	key_timeout = msec;
}

}; // namespace
