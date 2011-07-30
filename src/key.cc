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
#include <transcript/transcript.h>
#include <cerrno>

#ifdef HAS_SCHED_FUNCS
#include <sched.h>
#endif

#include <t3window/window.h>
#include <t3key/key.h>

#include "util.h"
#include "main.h"
#include "key.h"
#include "keybuffer.h"
#include "internal.h"
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

typedef struct {
	key_t kp;
	key_t mapped;
} kp_mapping_t;

static const key_string_t key_strings[] = {
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
	{ "kp_home", EKEY_KP_HOME },
	{ "kp_up", EKEY_KP_UP },
	{ "kp_page_up", EKEY_KP_PGUP },
	{ "kp_page_down", EKEY_KP_PGDN },
	{ "kp_left", EKEY_KP_LEFT },
	{ "kp_center", EKEY_KP_CENTER },
	{ "kp_right", EKEY_KP_RIGHT },
	{ "kp_end", EKEY_KP_END },
	{ "kp_down", EKEY_KP_DOWN },
	{ "kp_insert", EKEY_KP_INS },
	{ "kp_delete", EKEY_KP_DEL },
	{ "kp_enter", EKEY_KP_NL },
	{ "kp_div", EKEY_KP_DIV },
	{ "kp_mul", EKEY_KP_MUL },
	{ "kp_minus", EKEY_KP_MINUS },
	{ "kp_plus", EKEY_KP_PLUS },
	{ "tab", '\t' },
	{ "backspace", EKEY_BS }
};

static const kp_mapping_t kp_mappings[] = {
	{ EKEY_KP_HOME, EKEY_HOME },
	{ EKEY_KP_PGUP, EKEY_PGUP },
	{ EKEY_KP_PGDN, EKEY_PGDN },
	{ EKEY_KP_LEFT, EKEY_LEFT },
	{ EKEY_KP_RIGHT, EKEY_RIGHT },
	{ EKEY_KP_UP, EKEY_UP },
	{ EKEY_KP_DOWN, EKEY_DOWN },
	{ EKEY_KP_DEL, EKEY_DEL },
	{ EKEY_KP_INS, EKEY_INS },
	{ EKEY_KP_NL, EKEY_NL },
	{ EKEY_KP_DIV, '/' },
	{ EKEY_KP_MUL, '*' },
	{ EKEY_KP_PLUS, '+' },
	{ EKEY_KP_MINUS, '-' }
};

#define ARRAY_SIZE(_x) ((int) (sizeof(_x) / sizeof(_x[0])))

static mapping_t *map;
static int map_count;
static key_t map_single[128];

static const char *leave, *enter;
static const t3_key_node_t *keymap;
static int signal_pipe[2] = { -1, -1 };

static key_buffer_t key_buffer;
static pthread_t read_key_thread;

static char char_buffer[32];
static int char_buffer_fill;
static uint32_t unicode_buffer[16];
static int unicode_buffer_fill;
static transcript_t *conversion_handle;

static int key_timeout = -1;
static bool drop_single_esc = true;

/* Used in decode_sequence and comparison routine to communicate whether the current
   sequence is a prefix of any known sequence. */
static bool is_prefix;


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

	while ((c = t3_term_get_keychar(timeout)) == T3_WARN_UPDATE_TERMINAL) {
		transcript_t *new_conversion_handle;
		transcript_error_t transcript_error;
		/* Open new conversion handle, but make sure we actually succeed in opening it,
		   before we close the old one. */
		if ((new_conversion_handle = transcript_open_converter(t3_term_get_codeset(), TRANSCRIPT_UTF32, 0, &transcript_error)) != NULL) {
			transcript_close_converter(conversion_handle);
			conversion_handle = new_conversion_handle;
		} else {
			lprintf("Error opening new convertor '%s': %s\n", t3_term_get_codeset(), transcript_strerror(transcript_error));
		}
		lprintf("New codeset: %s\n", t3_term_get_codeset());
		key_buffer.push_back_unique(EKEY_UPDATE_TERMINAL);
	}

	if (c < T3_WARN_MIN)
		return -1;

	char_buffer[char_buffer_fill++] = (char) c;
	char_buffer_ptr = char_buffer;
	unicode_buffer_ptr = unicode_buffer;

	while (1) {
		switch (transcript_to_unicode(conversion_handle, &char_buffer_ptr, char_buffer + char_buffer_fill,
				(char **) &unicode_buffer_ptr, ((const char *) unicode_buffer) + sizeof(unicode_buffer), TRANSCRIPT_ALLOW_FALLBACK))
		{
			case TRANSCRIPT_SUCCESS:
			case TRANSCRIPT_NO_SPACE:
			case TRANSCRIPT_INCOMPLETE:
				char_buffer_fill -= char_buffer_ptr - char_buffer;
				if (char_buffer_fill != 0)
					memmove(char_buffer, char_buffer_ptr, char_buffer_fill);
				unicode_buffer_fill = unicode_buffer_ptr - unicode_buffer;
				return 0;

			case TRANSCRIPT_FALLBACK: // NOTE: we allow fallbacks, so this should not even occur!!!

			case TRANSCRIPT_UNASSIGNED:
			case TRANSCRIPT_ILLEGAL:
			case TRANSCRIPT_ILLEGAL_END:
			case TRANSCRIPT_INTERNAL_ERROR:
			case TRANSCRIPT_PRIVATE_USE:
				transcript_to_unicode_skip(conversion_handle, &char_buffer_ptr, char_buffer + char_buffer_fill);
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

		if (retval < 0)
			continue;

		if (FD_ISSET(signal_pipe[0], &readset)) {
			char command;

			nosig_read(signal_pipe[0], &command, 1);
			switch (command) {
				case QUIT_SIGNAL:
					/* Exit thread */
					close(signal_pipe[0]);
					close(signal_pipe[1]);
					signal_pipe[0] = -1;
					signal_pipe[1] = -1;
					leave = NULL;
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
				else if (drop_single_esc && c == (EKEY_ESC | EKEY_META))
					c = EKEY_ESC;
				else if ((c & EKEY_KEY_MASK) < 128 && map_single[c & EKEY_KEY_MASK] != 0)
					c = (c & ~EKEY_KEY_MASK) | map_single[c & EKEY_KEY_MASK];
			} else if (c > 0 && c < 128 && map_single[c] != 0) {
				c = map_single[c];
			}
			key_buffer.push_back(c);
		}
	}
}

key_t read_key(void) {
	return key_buffer.pop_front();
}

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
				if (sequence.idx == 1 && outer) {
					key_t alted = decode_sequence(false);
					return alted >= 0 ? alted | EKEY_META : EKEY_ESC;
				}
				unget_key(c);
				goto unknown_sequence;
			}

			sequence.data[sequence.idx++] = c;

			is_prefix = false;
			if ((matched = (mapping_t *) bsearch(&sequence, map, map_count,
					sizeof(mapping_t), compare_sequence_with_mapping)) != NULL)
				return matched->key;

			/* Detect and ignore ANSI CSI sequences, regardless of whether they are recognised. */
			if (sequence.data[1] == '[') {
				if (sequence.idx > 2 && c >= 0x40 && c < 0x7f) {
					return -1;
				} else if (c < 0x20 || c > 0x7f) {
					/* Drop unknown leading sequence if some non-CSI byte is found. */
					unget_key(c);
					return -1;
				}
				continue;
			}

			if (!is_prefix)
				goto unknown_sequence;
		}

		if (read_and_convert_keys(outer ? key_timeout : 50) < 0)
			break;
	}

unknown_sequence:
	if (sequence.idx == 2)
		return sequence.data[1] | EKEY_META;
	else if (sequence.idx == 1 && !drop_single_esc)
		return EKEY_ESC;

	/* Something unwanted has happened here: the character sequence we encoutered was not
	   in our key map. Because it will give some undesired result to just return
	   <alt><first character> we ignore the whole sequence */
	return -1;
}

void insert_protected_key(key_t key) {
	key_buffer.push_back(key | EKEY_PROTECT);
}

static void sigwinch_handler(int param) {
	char winch_signal = WINCH_SIGNAL;
	(void) param;
	nosig_write(signal_pipe[1], &winch_signal, 1);
}

static key_t map_kp(key_t kp) {
	size_t i;
	for (i = 0; i < ARRAY_SIZE(kp_mappings); i++)
		if (kp_mappings[i].kp == kp)
			return kp_mappings[i].mapped;
	return kp;
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

static bool is_function_key(const char *str) {
	/* First character must be f, second a digit ... */
	if (str[0] != 'f' || !(str[1] >= '0' && str[1] <= '9'))
		return false;

	/* ... third either a digit, + or nothing ... */
	if (str[2] == 0 || str[2] == '+')
		return true;
	if (!(str[2] >= '0' && str[2] <= '9'))
		return false;

	/* ... fourth either + or nothing. */
	if (str[3] == 0 || str[3] == '+')
		return true;

	return false;
}

#define RETURN_ERROR(_s, _x) do { result.set_error(_s, _x, __FILE__, __LINE__); goto return_error; } while (0)
/* Initialize the key map */
complex_error_t init_keys(const char *term, bool separate_keypad) {
	complex_error_t result;
	struct sigaction sa;
	sigset_t sigs;
	const t3_key_node_t *key_node;
	int i, j, error, idx;
	transcript_error_t transcript_error;
#ifdef HAS_SCHED_FUNCS
	struct sched_param sched_param;
#endif
	const char *shiftfn = NULL;

	/* Start with things most likely to fail */
	if ((conversion_handle = transcript_open_converter(transcript_get_codeset(), TRANSCRIPT_UTF32, 0, &transcript_error)) == NULL)
		RETURN_ERROR(complex_error_t::SRC_TRANSCRIPT, transcript_error);

	if ((keymap = t3_key_load_map(term, NULL, &error)) == NULL)
		RETURN_ERROR(complex_error_t::SRC_T3_KEY, error);

	if (pipe(signal_pipe) < 0)
		RETURN_ERROR(complex_error_t::SRC_ERRNO, errno);

	sa.sa_handler = sigwinch_handler;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGWINCH);
	sa.sa_flags = 0;

	if (sigaction(SIGWINCH, &sa, NULL) < 0)
		RETURN_ERROR(complex_error_t::SRC_ERRNO, errno);

	sigemptyset(&sigs);
	sigaddset(&sigs, SIGWINCH);
	if (sigprocmask(SIG_UNBLOCK, &sigs, NULL) < 0)
		RETURN_ERROR(complex_error_t::SRC_ERRNO, errno);

	for (i = 1; i <= 26; i++)
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
	if ((key_node = t3_key_get_named_node(keymap, "%shiftfn")) != NULL)
		shiftfn = key_node->string;

	/* Load all the known keys from the terminfo database.
	   - find out how many sequences there are
	   - allocate an appropriate amount of memory
	   - fill the map
	   - sort the map for quick searching
	*/
	for (key_node = keymap; key_node != NULL; key_node = key_node->next) {
		if (key_node->key[0] == '%')
			continue;
		if (key_node->string[0] == 27)
			map_count++;
	}

	if ((map = (mapping_t *) malloc(sizeof(mapping_t) * map_count)) == NULL)
		RETURN_ERROR(complex_error_t::SRC_ERRNO, ENOMEM);

	for (key_node = keymap, idx = 0; key_node != NULL; key_node = key_node->next) {
		if (key_node->key[0] == '%')
			continue;

		if (key_node->string[0] == 27) {
			map[idx].string = key_node->string;
			map[idx].string_length = key_node->string_length;
		}

		for (i = 0; i < ARRAY_SIZE(key_strings); i++) {
			/* Check if this is a sequence we know. */
			for (j = 0; key_strings[i].string[j] == key_node->key[j] &&
					key_strings[i].string[j] != 0 && key_node->key[j] != 0; j++)
			{}

			if (!(key_strings[i].string[j] == 0 && (key_node->key[j] == '+' || key_node->key[j] == 0)))
				continue;

			if (key_node->string[0] != 27) {
				map_single[(unsigned char) key_node->string[0]] = key_strings[i].code;
				break;
			}

			map[idx].key = separate_keypad ? key_strings[i].code : map_kp(key_strings[i].code);
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
			break;
		}

		if (i == ARRAY_SIZE(key_strings)) {
			if (is_function_key(key_node->key)) {
				key_t key = EKEY_F1 + atoi(key_node->key + 1) - 1;
				for (j = 2; key_node->key[j] != 0; j++) {
					switch (key_node->key[j]) {
						case 'c':
							key |= EKEY_CTRL;
							break;
						case 'm':
							key |= EKEY_META;
							break;
						case 's':
							key |= EKEY_SHIFT;
							break;
						default:
							break;
					}
				}
				if (shiftfn != NULL && key >= EKEY_F1 + shiftfn[2] - 1 && key < EKEY_F1 + shiftfn[2] + shiftfn[1] - shiftfn[0]) {
					key -= shiftfn[2] - 1;
					key |= EKEY_SHIFT;
				}
				if (key_node->string[0] == 27)
					map[idx].key = key;
				else
					map_single[(unsigned char) key_node->string[0]] = key;
			} else {
				if (key_node->string[0] == 27)
					map[idx].key = EKEY_IGNORE;
			}
		}
		if (key_node->string[0] == 27)
			idx++;
	}
	qsort(map, map_count, sizeof(mapping_t), compare_mapping);

	if ((error = pthread_create(&read_key_thread, NULL, read_keys, NULL)) != 0)
		RETURN_ERROR(complex_error_t::SRC_ERRNO, error);

#ifdef HAS_SCHED_FUNCS
	/* Set the priority for the key reading thread to max, such that we can be sure
	   that when a key is available it will be able to get it. */
	sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if (sched_param.sched_priority == -1)
		sched_param.sched_priority = 0;
	pthread_setschedparam(read_key_thread, SCHED_FIFO, &sched_param);
#endif

	if (leave != NULL)
		atexit(stop_keys);
	return result;

return_error:
	cleanup_keys();
	leave = NULL;
	if (signal_pipe[0] != -1) {
		close(signal_pipe[0]);
		close(signal_pipe[1]);
		signal_pipe[0] = -1;
		signal_pipe[1] = -1;
	}
	return result;
}
#undef RETURN_ERROR

void deinit_keys(void) {
	t3_term_putp(leave);
}

void reinit_keys(void) {
	t3_term_putp(enter);
}

void cleanup_keys(void) {
	if (conversion_handle != NULL) {
		transcript_close_converter(conversion_handle);
		conversion_handle = NULL;
	}
	if (keymap != NULL) {
		t3_key_free_map(keymap);
		keymap = NULL;
	}
	if (map != NULL) {
		free(map);
		map = NULL;
	}
	memset(map_single, 0, sizeof(map_single));
	leave = NULL;
	enter = NULL;
}

static void stop_keys(void) {
	void *retval;
	char quit_signal = QUIT_SIGNAL;
	nosig_write(signal_pipe[1], &quit_signal, 1);
	pthread_join(read_key_thread, &retval);
	t3_term_putp(leave);
}

void set_key_timeout(int msec) {
	if (msec == 0) {
		key_timeout = -1;
		drop_single_esc = true;
	} else if (msec < 0) {
		key_timeout = -msec;
		drop_single_esc = true;
	} else {
		key_timeout = msec;
		drop_single_esc = false;
	}
}

int get_key_timeout(void) {
	return key_timeout < 0 ? 0 : (drop_single_esc ? -key_timeout : key_timeout);
}

void signal_update(void) {
	key_buffer.push_back_unique(EKEY_EXTERNAL_UPDATE);
}

}; // namespace
