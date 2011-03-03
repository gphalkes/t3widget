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

#include "keys.h"
#include "util.h"
#include "main.h"
#include "key.h"
#ifdef THREADED_KEYS
#include "keybuffer.h"
#endif

#define MAX_SEQUENCE 100

typedef struct {
	const char *string;
	key_t code;
} key_string_t;

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

typedef struct {
	const char *string;
	size_t string_length;
	key_t key;
} mapping_t;

static mapping_t *map;
static int map_count;
static key_t map_single[UCHAR_MAX];

static const char *leave, *enter;
static const t3_key_node_t *keymap;
static int signal_pipe[2];
static key_t protected_insert;

enum {
	WINCH_SIGNAL,
	QUIT_SIGNAL
};

static key_t decode_sequence(bool outer);
static void stop_keys(void);

void sigwinchhandler(int param) {
	char winch_signal = WINCH_SIGNAL;
	(void) param;
	nosig_write(signal_pipe[1], &winch_signal, 1);
}

static key_t wreadkey_internal(void) {
	key_t c = -1;

	do {
		int retval;
		fd_set readset;
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
#ifdef THREADED_KEYS
				case QUIT_SIGNAL:
					pthread_exit(NULL);
					continue;
#endif
				case WINCH_SIGNAL:
					return EKEY_RESIZE;
				default:
					// This should be impossible, but just ignore for now
					continue;
			}
		}

		if (FD_ISSET(0, &readset)) {
			c = t3_term_get_keychar(-1);

			if (c == EKEY_ESC) {
				c = decode_sequence(true);
#warning FIXME: decode UTF-8 and other encodings
/* 			} else if (UTF8mode && c > 0 && (c & 0x80)) {
				UTF8TermInput input;
				t3_term_unget_keychar(c);
				c = getNextUTF8InputChar(&input); */
			} else if (c > 0 && c < UCHAR_MAX && map_single[c] != 0) {
				c = map_single[c];
			}
		}
		// FIXME: check for error conditions other than timeout!
	} while (c < 0);

	return c;
}

#ifdef THREADED_KEYS
static KeyBuffer key_buffer;
static pthread_t read_key_thread;

static void *read_keys(void *arg) {
	(void) arg;
	while (1)
		key_buffer.push_back(wreadkey_internal());
	return NULL;
}
#endif

key_t wreadkey(void) {
	if (protected_insert) {
		int c = protected_insert;
		protected_insert = 0;
		return c;
	}
#ifdef THREADED_KEYS
	return key_buffer.pop_front();
#else
	return wreadkey_internal();
#endif
}

static key_t decode_sequence(bool outer) {
	size_t sequence_index = 1;
	int c, i;
	char sequence[MAX_SEQUENCE];

	sequence[0] = EKEY_ESC;

	while (sequence_index < MAX_SEQUENCE) {
		c = t3_term_get_keychar(100);
		/* Check for timeout */
		if (c < 0) {
			break;
		} else if (c == EKEY_ESC) {
			/* FIXME: we need to change this so we can use alt on UTF-8, and use the
			   sequences in the library for key+m */
			if (sequence_index == 1 && outer) {
				key_t alted = decode_sequence(false);
				return alted >= 0 ? alted | EKEY_META : EKEY_ESC;
			}
			t3_term_unget_keychar(c);
			break;
		}

		sequence[sequence_index++] = c;

		for (i = 0; i < map_count; i++) {
			if (sequence_index == map[i].string_length && memcmp(sequence, map[i].string, sequence_index) == 0)
				return map[i].key;
		}
	}

	if (sequence_index == 2)
		return ((key_t) sequence[1]) | EKEY_META;
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


/* Initialize the key map */
int init_keys(void) {
	struct sigaction sa;
	sigset_t sigs;
	const t3_key_node_t *key_node;
	int i, error, idx;

	if (pipe(signal_pipe) < 0)
		return -1;

	sa.sa_handler = sigwinchhandler;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGWINCH);
	sa.sa_flags = 0;

	if (sigaction(SIGWINCH, &sa, NULL) < 0)
		return -1;
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGWINCH);
	if (sigprocmask(SIG_UNBLOCK, &sigs, NULL) < 0)
		return -1;

	if ((keymap = t3_key_load_map(NULL, NULL, &error)) == NULL)
		return error;

	/* Add a few default keys which will help on terminals with little information
	   available. */
	map_single[8] = EKEY_BS;
	map_single[127] = EKEY_BS;
	map_single[10] = EKEY_NL;
	map_single[13] = EKEY_NL;

	/* Add important control keys */
	map_single[1] = EKEY_CTRL | 'a';
	map_single[3] = EKEY_CTRL | 'c';
	map_single[5] = EKEY_META | EKEY_ESC;
	map_single[6] = EKEY_CTRL | 'f';
	map_single[7] = EKEY_CTRL | 'g';
	map_single[14] = EKEY_CTRL | 'n';
	map_single[15] = EKEY_CTRL | 'o';
	map_single[17] = EKEY_CTRL | 'q';
	map_single[18] = EKEY_CTRL | 'r';
	map_single[19] = EKEY_CTRL | 's';
	map_single[22] = EKEY_CTRL | 'v';
	map_single[23] = EKEY_CTRL | 'w';
	map_single[24] = EKEY_CTRL | 'x';
	map_single[25] = EKEY_CTRL | 'y';
	map_single[26] = EKEY_CTRL | 'z';

	if ((key_node = t3_key_get_named_node(keymap, "%enter")) != NULL) {
		t3_term_putp(key_node->string);
		enter = key_node->string;
	}
	if ((key_node = t3_key_get_named_node(keymap, "%leave")) != NULL) {
		leave = key_node->string;
		atexit(stop_keys);
	}

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
		return T3_ERR_OUT_OF_MEMORY;

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

#ifdef THREADED_KEYS
	//FIXME: Check return value!
	pthread_create(&read_key_thread, NULL, read_keys, NULL);
#endif
	return T3_ERR_SUCCESS;
}

void deinit_keys(void) {
	t3_term_putp(leave);
}

void reinit_keys(void) {
	t3_term_putp(enter);
}

static void stop_keys(void) {
#ifdef THREADED_KEYS
	void *retval;
	char quit_signal = QUIT_SIGNAL;
	nosig_write(signal_pipe[1], &quit_signal, 1);
	pthread_join(read_key_thread, &retval);
#endif
	t3_term_putp(leave);
#ifdef DEBUG
	t3_key_free_map(keymap);
#endif
}
