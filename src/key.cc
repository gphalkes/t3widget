/* Copyright (C) 2011-2013,2018 G.P. Halkes
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
#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <map>
#include <t3key/key.h>
#include <thread>
#include <transcript/transcript.h>

#include <t3widget/internal.h>
#include <t3widget/key.h>
#include <t3widget/keybuffer.h>
#include <t3widget/log.h>
#include <t3widget/main.h>
#include <t3widget/util.h>

namespace t3_widget {

#define MAX_SEQUENCE 100

enum {
  WINCH_SIGNAL,
  QUIT_SIGNAL,
  EXIT_MAIN_LOOP_SIGNAL,
};

struct kp_mapping_t {
  key_t kp;
  key_t mapped;
};

struct t3_key_map_deleter {
  void operator()(const t3_key_node_t *node) { t3_key_free_map(node); }
};

static const std::map<std::string, key_t> key_strings{{"insert", EKEY_INS},
                                                      {"delete", EKEY_DEL},
                                                      {"home", EKEY_HOME},
                                                      {"end", EKEY_END},
                                                      {"page_up", EKEY_PGUP},
                                                      {"page_down", EKEY_PGDN},
                                                      {"up", EKEY_UP},
                                                      {"left", EKEY_LEFT},
                                                      {"down", EKEY_DOWN},
                                                      {"right", EKEY_RIGHT},
                                                      {"kp_home", EKEY_KP_HOME},
                                                      {"kp_up", EKEY_KP_UP},
                                                      {"kp_page_up", EKEY_KP_PGUP},
                                                      {"kp_page_down", EKEY_KP_PGDN},
                                                      {"kp_left", EKEY_KP_LEFT},
                                                      {"kp_center", EKEY_KP_CENTER},
                                                      {"kp_right", EKEY_KP_RIGHT},
                                                      {"kp_end", EKEY_KP_END},
                                                      {"kp_down", EKEY_KP_DOWN},
                                                      {"kp_insert", EKEY_KP_INS},
                                                      {"kp_delete", EKEY_KP_DEL},
                                                      {"kp_enter", EKEY_KP_NL},
                                                      {"kp_div", EKEY_KP_DIV},
                                                      {"kp_mul", EKEY_KP_MUL},
                                                      {"kp_minus", EKEY_KP_MINUS},
                                                      {"kp_plus", EKEY_KP_PLUS},
                                                      {"tab", '\t'},
                                                      {"backspace", EKEY_BS}};

static const kp_mapping_t kp_mappings[] = {
    {EKEY_KP_HOME, EKEY_HOME}, {EKEY_KP_PGUP, EKEY_PGUP},   {EKEY_KP_PGDN, EKEY_PGDN},
    {EKEY_KP_LEFT, EKEY_LEFT}, {EKEY_KP_RIGHT, EKEY_RIGHT}, {EKEY_KP_UP, EKEY_UP},
    {EKEY_KP_DOWN, EKEY_DOWN}, {EKEY_KP_DEL, EKEY_DEL},     {EKEY_KP_INS, EKEY_INS},
    {EKEY_KP_NL, EKEY_NL},     {EKEY_KP_DIV, '/'},          {EKEY_KP_MUL, '*'},
    {EKEY_KP_PLUS, '+'},       {EKEY_KP_MINUS, '-'}};

static std::map<std::string, key_t> map;
static key_t map_single[128];

static std::string leave, enter;

static int signal_pipe[2] = {-1, -1};

static key_buffer_t key_buffer;
static std::thread read_key_thread;

char char_buffer[128];
int char_buffer_fill;
static uint32_t unicode_buffer[16];
static int unicode_buffer_fill;
static transcript_t *conversion_handle;

static std::mutex key_timeout_lock;
static int key_timeout = -1;
static bool drop_single_esc = true;

static bool in_bracketed_paste;

static key_t decode_sequence(bool outer);
static key_t bracketed_paste_decode();
static void stop_keys();

static void convert_next_key() {
  const char *char_buffer_ptr;
  uint32_t *unicode_buffer_ptr;

  char_buffer_ptr = char_buffer;
  unicode_buffer_ptr = unicode_buffer;

  while (true) {
    switch (transcript_to_unicode(
        conversion_handle, &char_buffer_ptr, char_buffer + char_buffer_fill,
        reinterpret_cast<char **>(&unicode_buffer_ptr),
        (reinterpret_cast<const char *>(unicode_buffer)) + sizeof(unicode_buffer),
        TRANSCRIPT_ALLOW_FALLBACK | TRANSCRIPT_SINGLE_CONVERSION)) {
      case TRANSCRIPT_SUCCESS:
      case TRANSCRIPT_NO_SPACE:
      case TRANSCRIPT_INCOMPLETE:
        char_buffer_fill -= char_buffer_ptr - char_buffer;
        if (char_buffer_fill != 0) {
          memmove(char_buffer, char_buffer_ptr, char_buffer_fill);
        }
        unicode_buffer_fill = unicode_buffer_ptr - unicode_buffer;
        return;

      case TRANSCRIPT_FALLBACK:  // NOTE: we allow fallbacks, so this should not even occur!!!

      case TRANSCRIPT_UNASSIGNED:
      case TRANSCRIPT_ILLEGAL:
      case TRANSCRIPT_ILLEGAL_END:
      case TRANSCRIPT_INTERNAL_ERROR:
      case TRANSCRIPT_PRIVATE_USE:
        transcript_to_unicode_skip(conversion_handle, &char_buffer_ptr,
                                   char_buffer + char_buffer_fill);
        break;
      default:
        // This shouldn't happen, and we can't really do anything with this.
        return;
    }
  }
}

static key_t get_next_converted_key() {
  if (unicode_buffer_fill == 0) {
    convert_next_key();
  }

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

static int get_next_keychar() {
  if (char_buffer_fill > 0) {
    int c = char_buffer[0];
    char_buffer_fill--;
    memmove(char_buffer, char_buffer + 1, char_buffer_fill);
    return static_cast<unsigned char>(c);
  }
  return -1;
}

static void unget_keychar(char c) {
  // Prevent buffer overflow. This simply drops the oldest character off the buffer.
  if (char_buffer_fill >= ARRAY_SIZE(char_buffer)) {
    char_buffer_fill = ARRAY_SIZE(char_buffer) - 1;
  }
  memmove(char_buffer + 1, char_buffer, sizeof(char_buffer[0]) * char_buffer_fill);
  char_buffer[0] = c;
  char_buffer_fill++;
}

bool read_keychar(int timeout) {
  key_t c;

  if (char_buffer_fill >= ARRAY_SIZE(char_buffer)) {
    return true;
  }

  while ((c = t3_term_get_keychar(timeout)) == T3_WARN_UPDATE_TERMINAL) {
    transcript_t *new_conversion_handle;
    transcript_error_t transcript_error;
    /* Open new conversion handle, but make sure we actually succeed in opening it,
       before we close the old one. */
    if ((new_conversion_handle = transcript_open_converter(t3_term_get_codeset(), TRANSCRIPT_UTF32,
                                                           0, &transcript_error)) != nullptr) {
      transcript_close_converter(conversion_handle);
      conversion_handle = new_conversion_handle;
    } else {
      lprintf("Error opening new convertor '%s': %s\n", t3_term_get_codeset(),
              transcript_strerror(transcript_error));
    }
    lprintf("New codeset: %s\n", t3_term_get_codeset());
    key_buffer.push_back_unique(EKEY_UPDATE_TERMINAL);
  }

  if (c < T3_WARN_MIN) {
    return false;
  }

  char_buffer[char_buffer_fill++] = static_cast<char>(c);
  return true;
}

static void read_keys() {
  int retval;
  key_t c;
  fd_set readset;
  int max_fd;

  while (true) {
    FD_ZERO(&readset);
    FD_SET(0, &readset);
    FD_SET(signal_pipe[0], &readset);
    max_fd = signal_pipe[0];
    fd_set_mouse_fd(&readset, &max_fd);

    retval = select(max_fd + 1, &readset, nullptr, nullptr, nullptr);

    if (retval < 0) {
      continue;
    }

    if (FD_ISSET(signal_pipe[0], &readset)) {
      char command;

      nosig_read(signal_pipe[0], &command, 1);
      switch (command) {
        case QUIT_SIGNAL:
          /* Exit thread */
          close(signal_pipe[0]);
          signal_pipe[0] = -1;
          return;
        case WINCH_SIGNAL:
          key_buffer.push_back_unique(EKEY_RESIZE);
          break;
        case EXIT_MAIN_LOOP_SIGNAL: {
          unsigned char value;
          nosig_read(signal_pipe[0], reinterpret_cast<char *>(&value), 1);
          key_buffer.push_back_unique(EKEY_EXIT_MAIN_LOOP + value);
          break;
        }
        default:
          // This should be impossible, so just ignore
          continue;
      }
    }

    if (check_mouse_fd(&readset)) {
      key_buffer.push_back(EKEY_MOUSE_EVENT);
    }

    if (FD_ISSET(0, &readset)) {
      read_keychar(-1);
    }

    while ((c = get_next_converted_key()) >= 0) {
      if (c == EKEY_ESC) {
        if (in_bracketed_paste) {
          c = bracketed_paste_decode();
          if (c < 0) {
            continue;
          }
        } else {
          key_t modifiers = t3_term_get_modifiers_hack();

          key_timeout_lock.lock();
          c = decode_sequence(true);
          key_timeout_lock.unlock();
          if (c < 0) {
            continue;
          } else if (drop_single_esc && c == (EKEY_ESC | EKEY_META)) {
            c = EKEY_ESC;
          } else if ((c & EKEY_KEY_MASK) < 128 && map_single[c & EKEY_KEY_MASK] != 0) {
            c = (c & ~EKEY_KEY_MASK) | map_single[c & EKEY_KEY_MASK];
          }

          if (c == '\t' || (c >= EKEY_FIRST_SPECIAL && c < 0x111000 && c != EKEY_NL)) {
            c |= modifiers * EKEY_CTRL;
          }
        }
      } else if (!in_bracketed_paste && c > 0 && c < 128 && map_single[c] != 0) {
        c = map_single[c];
      }
      if (c >= 0) {
        if (in_bracketed_paste) {
          // Unfortunately, (some) terminals convert \n in the input into \r when pasting. There
          // seems to be no way to turn this off. So we'll have to pretend that any \r is the same
          // as the user pressing the return key, even though if the actual pasted text contains
          // \r\n as line endings this will double the number of newlines. As this is the same when
          // not using bracketed paste, this is a acceptable strategy.
          key_buffer.push_back(c == '\n' || c == '\r' ? EKEY_NL : EKEY_PROTECT | c);
        } else {
          key_buffer.push_back(c);
        }
      }
    }
  }
}

key_t read_key() { return key_buffer.pop_front(); }

static void unget_key_sequence(const std::string &sequence) {
  for (char c : reverse_view(sequence)) {
    unget_keychar(c);
  }
}

static key_t decode_sequence(bool outer) {
  std::string sequence;
  int c;

  sequence.push_back(EKEY_ESC);

  while (sequence.size() < MAX_SEQUENCE) {
    while ((c = get_next_keychar()) >= 0) {
      if (c == EKEY_ESC) {
        if (sequence.size() == 1 && outer) {
          key_t alted = decode_sequence(false);
          return alted >= 0 ? alted | EKEY_META : (alted == -2 ? EKEY_ESC : -1);
        }
        unget_keychar(c);
        goto unknown_sequence;
      }

      sequence.push_back(c);

      std::map<std::string, key_t>::iterator iter = map.lower_bound(sequence);
      if (iter->first == sequence) {
        return iter->second;
      }
      bool is_prefix = starts_with(iter->first, sequence);

      /* Detect and ignore ANSI CSI sequences, regardless of whether they are recognised.
         An exception is made for mouse events, which also start with CSI. */
      if (sequence[1] == '[' && !is_prefix) {
        if (sequence.size() == 3 && c == 'M' && use_xterm_mouse_reporting()) {
          if (!outer) {
            /* If this is not the outer decode_sequence call, push everything
               back onto the character list, and do nothing. A next call to
               decode_sequence will take care of the mouse handling. */
            unget_key_sequence(sequence);
            return -1;
          }
          return decode_xterm_mouse() ? EKEY_MOUSE_EVENT : -1;
        } else if (sequence.size() > 3 && (c == 'M' || c == 'm') && use_xterm_mouse_reporting()) {
          if (!outer) {
            /* If this is not the outer decode_sequence call, push everything
               back onto the character list, and do nothing. A next call to
               decode_sequence will take care of the mouse handling. */
            unget_key_sequence(sequence);
            return -1;
          }
          return decode_xterm_mouse_sgr_urxvt(sequence.data(), sequence.size()) ? EKEY_MOUSE_EVENT
                                                                                : -1;
        } else if (c == '~') {
          if (sequence.size() != 6 || sequence[2] != '2' || sequence[3] != '0') {
            return -1;
          }
          if (!outer) {
            /* If this is not the outer decode_sequence call, push everything
               back onto the character list, and do nothing. A next call to
               decode_sequence will take care of the mouse handling. */
            unget_key_sequence(sequence);
            return -1;
          }
          if (sequence[4] == '0') {
            in_bracketed_paste = true;
            return EKEY_PASTE_START;
          }
          return -1;
        } else if (sequence.size() > 2 && c >= 0x40 && c < 0x7f) {
          return -1;
        } else if (c < 0x20 || c > 0x7f) {
          /* Drop unknown leading sequence if some non-CSI byte is found. */
          unget_key(c);
          return -1;
        }
        continue;
      }

      if (!is_prefix) {
        goto unknown_sequence;
      }
    }

    if (char_buffer_fill == 0 && !read_keychar(outer ? key_timeout : 50)) {
      break;
    }
  }

unknown_sequence:
  if (sequence.size() == 2) {
    key_t alted_key;
    unget_keychar(sequence[1]);
    /* It is quite possible that we only read a partial character here. So if we haven't
       read a complete character yet (i.e. get_next_converted_key returns -1), we simply
       keep asking to read one more character. We use a one millisecond timeout, to ensure
       we don't get stuck waiting here. If reading the keychar times out, we need to skip
       this input. */
    while ((alted_key = get_next_converted_key()) < 0 && read_keychar(1)) {
    }
    if (alted_key < 0) {
      return -1;
    }
    /* We need to do single key mapping as well here. */
    if ((alted_key & EKEY_KEY_MASK) < 128 && map_single[alted_key & EKEY_KEY_MASK] != 0) {
      alted_key = map_single[alted_key & EKEY_KEY_MASK];
    }
    return alted_key | EKEY_META;
  } else if (sequence.size() == 1) {
    return drop_single_esc ? -2 : EKEY_ESC;
  }

  /* Something unwanted has happened here: the character sequence we encoutered was not
     in our key map. Because it will give some undesired result to just return
     <alt><first character> we ignore the whole sequence */
  return -1;
}

static key_t bracketed_paste_decode() {
  int c;
  char data[6];
  int idx = 1;

  data[0] = EKEY_ESC;

  while (idx < 6) {
    while ((c = get_next_keychar()) >= 0) {
      data[idx++] = c;
      if (strncmp(data, "\033[201~", idx) != 0) {
        for (int i = idx; i > 1; --i) {
          unget_keychar(data[i - 1]);
        }
        return EKEY_ESC;
      }
      if (idx == 6) {
        in_bracketed_paste = false;
        return EKEY_PASTE_END;
      }
    }
    if (char_buffer_fill == 0 && !read_keychar(50)) {
      break;
    }
  }
  for (int i = idx; i > 0; --i) {
    unget_keychar(data[i - 1]);
  }
  return -1;
}

void insert_protected_key(key_t key) {
  if (key >= 0) {
    key_buffer.push_back(key | EKEY_PROTECT);
  }
}

static void sigwinch_handler(int param) {
  char winch_signal = WINCH_SIGNAL;
  int saved_errno = errno;
  (void)param;
  nosig_write(signal_pipe[1], &winch_signal, 1);
  errno = saved_errno;
}

static key_t map_kp(key_t kp) {
  size_t i;
  for (i = 0; i < ARRAY_SIZE(kp_mappings); i++) {
    if (kp_mappings[i].kp == kp) {
      return kp_mappings[i].mapped;
    }
  }
  return kp;
}

static bool is_function_key(const char *str) {
  /* First character must be f, second a digit ... */
  if (str[0] != 'f' || !(str[1] >= '0' && str[1] <= '9')) {
    return false;
  }

  /* ... third either a digit, - or nothing ... */
  if (str[2] == 0 || str[2] == '-') {
    return true;
  }
  if (!(str[2] >= '0' && str[2] <= '9')) {
    return false;
  }

  /* ... fourth either - or nothing. */
  if (str[3] == 0 || str[3] == '-') {
    return true;
  }

  return false;
}

#define RETURN_ERROR(_s, _x)                      \
  do {                                            \
    result.set_error(_s, _x, __FILE__, __LINE__); \
    goto return_error;                            \
  } while (false)

/* Initialize the key map */
complex_error_t init_keys(const optional<std::string> &term, bool separate_keypad) {
  complex_error_t result;
  struct sigaction sa;
  sigset_t sigs;
  std::unique_ptr<const t3_key_node_t, t3_key_map_deleter> keymap;
  const t3_key_node_t *key_node;
  int i, error;
  transcript_error_t transcript_error;
  const char *shiftfn = nullptr;

  /* Start with things most likely to fail */
  if ((conversion_handle = transcript_open_converter(transcript_get_codeset(), TRANSCRIPT_UTF32, 0,
                                                     &transcript_error)) == nullptr) {
    RETURN_ERROR(complex_error_t::SRC_TRANSCRIPT, transcript_error);
  }

  keymap.reset(t3_key_load_map(term.is_valid() ? term.value().c_str() : nullptr, nullptr, &error));
  if (keymap == nullptr) {
    RETURN_ERROR(complex_error_t::SRC_T3_KEY, error);
  }

  if (pipe(signal_pipe) < 0) {
    RETURN_ERROR(complex_error_t::SRC_ERRNO, errno);
  }

  sa.sa_handler = sigwinch_handler;
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, SIGWINCH);
  sa.sa_flags = 0;

  if (sigaction(SIGWINCH, &sa, nullptr) < 0) {
    RETURN_ERROR(complex_error_t::SRC_ERRNO, errno);
  }

  sigemptyset(&sigs);
  sigaddset(&sigs, SIGWINCH);
  if (sigprocmask(SIG_UNBLOCK, &sigs, nullptr) < 0) {
    RETURN_ERROR(complex_error_t::SRC_ERRNO, errno);
  }

  for (i = 1; i <= 26; i++) {
    map_single[i] = EKEY_CTRL | ('a' + i - 1);
  }
  /* "unmap" TAB */
  map_single[static_cast<int>('\t')] = 0;
  /* EKEY_ESC is defined as 27, so no need to map */
  map_single[28] = EKEY_CTRL | '\\';
  map_single[29] = EKEY_CTRL | ']';
  map_single[30] = EKEY_CTRL | '^';
  map_single[31] = EKEY_CTRL | '_';

  /* Add a few default keys which will help on terminals with little information
     available. */
  map_single[8] = EKEY_BS;
  map_single[127] = EKEY_BS;
  map_single[10] = EKEY_NL;
  map_single[13] = EKEY_NL;

  if ((key_node = t3_key_get_named_node(keymap.get(), "_enter")) != nullptr) {
    t3_term_putp(key_node->string);
    enter = key_node->string;
  }
  if ((key_node = t3_key_get_named_node(keymap.get(), "_leave")) != nullptr) {
    leave = key_node->string;
  }
  if ((key_node = t3_key_get_named_node(keymap.get(), "_shiftfn")) != nullptr) {
    shiftfn = key_node->string;
  }

  // Enable bracketed paste.
  t3_term_putp("\033[?2004h");

  init_mouse_reporting(t3_key_get_named_node(keymap.get(), "_xterm_mouse") != nullptr);

  /* Load all the known keys from the terminfo database.
     - find out how many sequences there are
     - allocate an appropriate amount of memory
     - fill the map
     - sort the map for quick searching
  */
  for (key_node = keymap.get(); key_node != nullptr; key_node = key_node->next) {
    if (key_node->key[0] == '_') {
      continue;
    }

    std::string key_name = key_node->key;
    // Remove the modifiers from the name.
    size_t dash = key_name.find('-');
    if (dash != std::string::npos) {
      key_name.resize(dash);
    }
    const auto iter = key_strings.find(key_name);

    if (iter == key_strings.end()) {
      if (is_function_key(key_node->key)) {
        key_t key = EKEY_F1 + atoi(key_node->key + 1) - 1;
        for (size_t j = dash; j != std::string::npos && key_node->key[j] != 0; j++) {
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
        if (shiftfn != nullptr && key >= EKEY_F1 + shiftfn[2] - 1 &&
            key < EKEY_F1 + shiftfn[2] + shiftfn[1] - shiftfn[0]) {
          key -= shiftfn[2] - 1;
          key |= EKEY_SHIFT;
        }
        if (key_node->string[0] == 27) {
          map[key_node->string] = key;
        } else if (strlen(key_node->string) == 1) {
          map_single[static_cast<unsigned char>(key_node->string[0])] = key;
        }
      } else {
        if (key_node->string[0] == 27) {
          map[key_node->string] = EKEY_IGNORE;
        }
      }

    } else {
      if (key_node->string[0] != 27) {
        if (strlen(key_node->string) == 1) {
          map_single[static_cast<unsigned char>(key_node->string[0])] = iter->second;
        }
      } else {
        key_t key = separate_keypad ? iter->second : map_kp(iter->second);
        for (size_t j = dash; j != std::string::npos && key_node->key[j] != 0; j++) {
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
        map[key_node->string] = key;
      }
    }
  }

  read_key_thread = std::thread(read_keys);

#ifdef DEBUG
  /* Using this_thread::yield here results in somewhat more deterministic results. Without it,
     there are sometimes issues wrt the update based on the terminal character set
     detection. However, it does not really matter which order is used, except for
     when we are executing our testsuite. Thus for DEBUG compiles, we add this_thread::yield. */
  std::this_thread::yield();
#endif
  return result;

return_error:
  int saved_errno = errno;
  cleanup_keys();
  if (signal_pipe[0] != -1) {
    close(signal_pipe[0]);
    close(signal_pipe[1]);
    signal_pipe[0] = -1;
    signal_pipe[1] = -1;
  }
  errno = saved_errno;
  return result;
}
#undef RETURN_ERROR

void deinit_keys() {
  deinit_mouse_reporting();
  // Disable bracketed paste.
  t3_term_putp("\033[?2004l");
  if (!leave.empty()) {
    t3_term_putp(leave.c_str());
  }
  in_bracketed_paste = false;
}

void reinit_keys() {
  if (!enter.empty()) {
    t3_term_putp(enter.c_str());
  }
  // Enable bracketed paste.
  t3_term_putp("\033[?2004h");
  reinit_mouse_reporting();
}

void cleanup_keys() {
  stop_keys();
  if (conversion_handle != nullptr) {
    transcript_close_converter(conversion_handle);
    conversion_handle = nullptr;
  }
  map.clear();
  memset(map_single, 0, sizeof(map_single));
  leave.clear();
  enter.clear();
}

static void stop_keys() {
  if (signal_pipe[1] != -1) {
    char quit_signal = QUIT_SIGNAL;
    nosig_write(signal_pipe[1], &quit_signal, 1);
    close(signal_pipe[1]);
    signal_pipe[1] = -1;
  }
  if (read_key_thread.joinable()) {
    read_key_thread.join();
  }
  stop_mouse_reporting();
  t3_term_putp("\033[?2004l");
  if (!leave.empty()) {
    t3_term_putp(leave.c_str());
  }
}

void set_key_timeout(int msec) {
  std::unique_lock<std::mutex> lock(key_timeout_lock);
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

int get_key_timeout() {
  std::unique_lock<std::mutex> lock(key_timeout_lock);
  return key_timeout < 0 ? 0 : (drop_single_esc ? -key_timeout : key_timeout);
}

void signal_update() { key_buffer.push_back_unique(EKEY_EXTERNAL_UPDATE); }

void async_safe_exit_main_loop(int exit_code) {
  char exit_signal[2] = {EXIT_MAIN_LOOP_SIGNAL, static_cast<char>(exit_code & 0xff)};
  int saved_errno = errno;
  nosig_write(signal_pipe[1], exit_signal, 2);
  errno = saved_errno;
}

}  // namespace
