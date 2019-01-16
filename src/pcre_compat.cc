/* Copyright (C) 2019 G.P. Halkes
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

#include "pcre_compat.h"

#ifdef PCRE_COMPAT
#include <string.h>

static const char *last_error_message = NULL;
static int last_error_code = 0;

pcre2_code_8 *pcre2_compile_8(PCRE2_SPTR8 pattern, PCRE2_SIZE pattern_size, uint32_t options,
                              int *errorcode, PCRE2_SIZE *erroroffset, void *) {
  const char *error_message;
  if (pattern_size != PCRE2_ZERO_TERMINATED && pattern[pattern_size] != 0) {
    abort();
  }
  pcre2_code_8 *result = new pcre2_code_8;
  result->regex = NULL;
  result->extra = NULL;

  result->regex = pcre_compile2(reinterpret_cast<const char *>(pattern), options, errorcode,
                                &error_message, erroroffset, NULL);
  if (result->regex == NULL) {
    last_error_message = error_message;
    last_error_code = *errorcode;
    delete result;
    return NULL;
  }
  return result;
}

int pcre2_jit_compile_8(pcre2_code_8 *code, uint32_t) {
  const char *error_message;

  code->extra = pcre_study(code->regex, 0, &error_message);
  return 0;
}

pcre2_match_data_8 *pcre2_match_data_create_8(uint32_t ovecsize, void *) {
  int *result;

  if (ovecsize == 0) {
    ++ovecsize;
  }
  result = new int[1 + 2 * ovecsize];
  *result = 2 * ovecsize;
  return result;
}

void pcre2_match_data_free_8(pcre2_match_data_8 *match_data) { delete[] match_data; }

PCRE2_SIZE *pcre2_get_ovector_pointer_8(pcre2_match_data_8 *match_data) { return match_data + 1; }
uint32_t pcre2_get_ovector_count_8(pcre2_match_data_8 *match_data) { return match_data[0] / 2; }

pcre2_match_data_8 *pcre2_match_data_create_from_pattern_8(const pcre2_code_8 *, void *) {
  return pcre2_match_data_create_8(15, NULL);
}

void pcre2_code_free_8(pcre2_code_8 *code) {
  if (code == NULL) {
    return;
  }
  pcre_free(code->regex);
  pcre_free_study(code->extra);
  delete code;
}

int pcre2_match_8(const pcre2_code_8 *code, PCRE2_SPTR8 subject, PCRE2_SIZE length,
                  PCRE2_SIZE startoffset, uint32_t options, pcre2_match_data_8 *match_data,
                  void *) {
  return pcre_exec(
      code->regex, code->extra, reinterpret_cast<const char *>(subject),
      length == PCRE2_ZERO_TERMINATED ? strlen(reinterpret_cast<const char *>(subject)) : length,
      startoffset, options, match_data + 1, *match_data);
}

int pcre2_get_error_message_8(int errorcode, PCRE2_UCHAR8 *buffer, PCRE2_SIZE bufflen) {
  char *copy_end;
  if (errorcode == last_error_code) {
    copy_end = strncpy(reinterpret_cast<char *>(buffer), last_error_message, bufflen);
  } else {
    copy_end = strncpy(reinterpret_cast<char *>(buffer), "unknown error", bufflen);
  }
  buffer[bufflen - 1] = 0;
  if (copy_end < reinterpret_cast<char *>(buffer) + bufflen) {
    return copy_end - reinterpret_cast<char *>(buffer);
  }
  return PCRE2_ERROR_NOMEMORY;
}
#else
int _t3_highlight_no_empty_translation_unit;
#endif
