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
#include <key/key.h>

#include "util.h"
#include "main.h"
#include "key.h"
#include "keybuffer.h"
#include "log.h"

using namespace std;
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

typedef struct {
	key_t data[MAX_SEQUENCE];
	size_t idx;
} key_sequence_t;

key_string_t key_strings[] = {
	{ "insert", EKEY_INS },
	{ "delete", EKEY_DEL },
	{ "home", EKEY_HOME },
	{ "end", EKEY_END },
	{ "page_up", EKEY_PGUP },
	{ "page_down", EKEY_PGDN },
	{ "up", EKEY_UP },
	{ "left", EKEY_LEFT },
	{ "down", EKEY_DOWN },
	{ "right", EKEY_RIGHT },
	{ "kp_home", EKEY_HOME },
	{ "kp_up", EKEY_UP },
	{ "kp_page_up", EKEY_PGUP },
	{ "kp_page_down", EKEY_PGDN },
	{ "kp_left", EKEY_LEFT },
	{ "kp_center", EKEY_CENTER },
	{ "kp_right", EKEY_RIGHT },
	{ "kp_end", EKEY_END },
	{ "kp_down", EKEY_DOWN },
	{ "kp_insert", EKEY_INS },
	{ "kp_delete", EKEY_DEL },
	{ "kp_enter", EKEY_NL },
	{ "kp_div", '+' },
	{ "kp_mul", '*' },
	{ "kp_minus", '-' },
	{ "kp_plus", '+' },
	{ "tab", '\t' },
	{ "backspace", EKEY_BS }
};

#define KEY_STRINGS_SIZE ((int) (sizeof(key_strings) / sizeof(key_strings[0])))

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
	return -1;
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
					key_buffer.push_back_unique(EKEY_RESIZE);
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
				if (c < 0)
					continue;
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

//FIXME: move to top
static bool is_prefix;

static int compare_sequence_with_mapping(const void *key, const void *mapping) {
	const key_sequence_t *_key;
	const mapping_t *_mapping;
	size_t i;

	_key = (const key_sequence_t *) key;
	_mapping = (const mapping_t *) mapping;

	for (i = 0; i < _key->idx && i < _mapping->string_length; i++) {
		if (_key->data[i] != _mapping->string[i]) {
			if ((char) _key->data[i] < _mapping->string[i])
				return -1;
			return 1;
		}
	}

	if (i < _mapping->string_length) {
		is_prefix = true;
		return -1;
	}

	if (i < _key->idx)
		return 1;
	return 0;
}

static key_t decode_sequence(bool outer) {
	key_sequence_t sequence;
	mapping_t *matched;
	int c;

	sequence.idx = 1;
	sequence.data[0] = EKEY_ESC;

	while (sequence.idx < MAX_SEQUENCE) {
		while ((c = get_next_converted_key()) >= 0) {
			if (c == EKEY_ESC) {
				if (key_timeout < 0) {
					if (sequence.idx == 1)
						return EKEY_ESC;
				} else {
					if (sequence.idx == 1 && outer) {
						key_t alted = decode_sequence(false);
						return alted >= 0 ? alted | EKEY_META : EKEY_ESC;
					}
				}
				unget_key(c);
				break;
			}

			sequence.data[sequence.idx++] = c;

			is_prefix = false;
			if ((matched = (mapping_t *) bsearch(&sequence, map, map_count, sizeof(mapping_t), compare_sequence_with_mapping)) != NULL)
				return matched->key;

			/* Detect and ignore ANSI CSI sequences, regardless of whether they are recognised. */
			if (sequence.data[1] == '[') {
				if (sequence.idx > 2 && c >= 0x40 && c < 0x7f)
					return -1;
				continue;
			}

			if (key_timeout < 0 && !is_prefix)
				goto ignore_sequence;
		}

		if (read_and_convert_keys(key_timeout) < 0)
			break;
	}

ignore_sequence:
	if (sequence.idx == 2)
		return sequence.data[1] | EKEY_META;
	else if (sequence.idx == 1)
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

static int compare_mapping(const void *a, const void *b) {
	const mapping_t *_a, *_b;
	int result;

	_a = (const mapping_t *) a;
	_b = (const mapping_t *) b;

	if ((result = memcmp(_a->string, _b->string, min(_a->string_length, _b->string_length))) != 0)
		return result;
	if (_a->string_length < _b->string_length)
		return -1;
	else if (_a->string_length > _b->string_length)
		return 1;
	return 0;
}

//FIXME: return value can not be a simple int, because of the different ranges of values it needs to support
#define RETURN_ERROR(_x) do { error = (_x); goto return_error; } while (0)
/* Initialize the key map */
int init_keys(void) {
	struct sigaction sa;
	sigset_t sigs;
	const t3_key_node_t *key_node;
	int i, j, error, idx;
	charconv_error_t charconv_error;
	struct sched_param sched_param;

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


	/* Load all the known keys from the terminfo database.
	   - find out how many we actually know
	   - allocate an appropriate amount of memory
	   - fill the map
	   - [TODO] sort the map for quick searching
	*/
	for (key_node = keymap; key_node != NULL; key_node = key_node->next) {
		for (i = 0; i < KEY_STRINGS_SIZE; i++) {
			if (key_node->string[0] != 27)
				continue;

			for (j = 0; key_strings[i].string[j] == key_node->key[j] &&
					key_strings[i].string[j] != 0 && key_node->key[j] != 0; j++)
			{}

			if (key_strings[i].string[j] == 0 && (key_node->key[j] == '+' || key_node->key[j] == 0)) {
				map_count++;
				break;
			}
		}

		if (i == KEY_STRINGS_SIZE) {
			/* First character must be f, second a digit ... */
			if (key_node->key[0] != 'f' || !isdigit(key_node->key[1]) ||
					/* ... third either a digit, + or nothing ... */
					(!isdigit(key_node->key[2]) && key_node->key[2] != 0 && key_node->key[2] != '+') ||
					/* ... if there is a second char, then fourth must be a digit. */
					(key_node->key[2] != 0 && key_node->key[3] != 0 && key_node->key[3] != '+'))
				continue;
			map_count++;
		}
	}

	if ((map = (mapping_t *) malloc(sizeof(mapping_t) * map_count)) == NULL)
		RETURN_ERROR(T3_ERR_OUT_OF_MEMORY);

	for (key_node = keymap, idx = 0; key_node != NULL; key_node = key_node->next) {
		for (i = 0; i < KEY_STRINGS_SIZE; i++) {
			for (j = 0; key_strings[i].string[j] == key_node->key[j] &&
					key_strings[i].string[j] != 0 && key_node->key[j] != 0; j++)
			{}

			if (!(key_strings[i].string[j] == 0 && (key_node->key[j] == '+' || key_node->key[j] == 0)))
				continue;

			if (key_node->string[0] != 27) {
				map_single[(unsigned char) key_node->string[0]] = key_strings[i].code;
				continue;
			}

			map[idx].string = key_node->string;
			map[idx].string_length = key_node->string_length;
			map[idx].key = key_strings[i].code;
			for (; key_node->key[j] != 0; j++) {
				switch (key_node->key[j]) {
					case 'c':
						map[idx].key |= EKEY_CTRL;
						break;
					case 'm':
						map[idx].key |= EKEY_META;
						break;
					case 's':
						map[idx].key |= EKEY_SHIFT;
						break;
					default:
						break;
				}
			}
			idx++;
			break;
		}
		if (i == KEY_STRINGS_SIZE) {
			/* First character must be f, second a digit ... */
			if (key_node->key[0] != 'f' || !isdigit(key_node->key[1]) ||
					/* ... third either a digit, + or nothing ... */
					(!isdigit(key_node->key[2]) && key_node->key[2] != 0 && key_node->key[2] != '+') ||
					/* ... if there is a second char, then fourth must be a digit. */
					(key_node->key[2] != 0 && key_node->key[3] != 0 && key_node->key[3] != '+'))
				continue;
			map[idx].string = key_node->string;
			map[idx].string_length = key_node->string_length;
			map[idx].key = EKEY_F1 + atoi(key_node->key + 1) - 1;
			for (j = 3; key_node->key[j] != 0; j++) {
				switch (key_node->key[j]) {
					case 'c':
						map[idx].key |= EKEY_CTRL;
						break;
					case 'm':
						map[idx].key |= EKEY_META;
						break;
					case 's':
						map[idx].key |= EKEY_SHIFT;
						break;
					default:
						break;
				}
			}
			idx++;
		}
	}
	qsort(map, map_count, sizeof(mapping_t), compare_mapping);

	if ((error = pthread_create(&read_key_thread, NULL, read_keys, NULL)) != 0)
		RETURN_ERROR(error);

	/* Set the priority for the key reading thread to max, such that we can be sure
	   that when a key is available it will be able to get it. */
	sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if (sched_param.sched_priority == -1)
		sched_param.sched_priority = 0;
	pthread_setschedparam(read_key_thread, SCHED_FIFO, &sched_param);

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

void signal_update(void) {
	key_buffer.push_back_unique(EKEY_EXTERNAL_UPDATE);
}

}; // namespace
