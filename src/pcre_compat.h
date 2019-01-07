/* Copyright (C) 2018 G.P. Halkes
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

/* This file contains a partial implementation of the PCRE2 API using the PCRE API. Note that a few
   short-cuts have been taken to allow this to work. For example, the general context has been
   replaced by void* because it is not used anywhere. */

#ifndef PCRE_COMPAT_H
#define PCRE_COMPAT_H

#include <t3widget/widget_api.h>

#ifdef PCRE_COMPAT

#include <pcre.h>
#include <stdint.h>

/* Although PCRE2_SIZE is size_t, in the PCRE API it is int. */
#define PCRE2_SIZE int
#define PCRE2_SPTR8 const unsigned char *
#define PCRE2_ZERO_TERMINATED ((PCRE2_SIZE)-1)
#define PCRE2_UTF PCRE_UTF8
#define PCRE2_ERROR_NOMEMORY PCRE_ERROR_NOMEMORY
#define PCRE2_UCHAR8 unsigned char
#define PCRE2_JIT_COMPLETE 1

#define PCRE2_NO_UTF_CHECK PCRE_NO_UTF8_CHECK
#define PCRE2_CASELESS PCRE_CASELESS
#define PCRE2_NOTEOL PCRE_NOTEOL
#define PCRE2_NOTBOL PCRE_NOTBOL

#define PCRE2_ERROR_BADOPTION PCRE_ERROR_BADOPTION

typedef struct {
  pcre *regex;
  pcre_extra *extra;
} pcre2_code_8;

typedef int pcre2_match_data_8;

/* Redefine the symbol names to prevent potential symbol clashes with the actual pcre2 library. */
#define pcre2_compile_8 t3_widget_pcre2_compile
#define pcre2_pattern_info_8 t3_widget_pcre2_pattern_info
#define pcre2_jit_compile_8 t3_widget_pcre2_jit_compile
#define pcre2_match_data_create_8 t3_widget_pcre2_match_data_create
#define pcre2_match_data_free_8 t3_widget_pcre2_match_data_free
#define pcre2_match_data_create_from_pattern_8 t3_widget_pcre2_match_data_create_from_pattern
#define pcre2_code_free_8 t3_widget_pcre2_code_free
#define pcre2_get_ovector_pointer_8 t3_widget_pcre2_get_ovector_pointer
#define pcre2_get_ovector_count_8 t3_widget_pcre2_get_ovector_count
#define pcre2_match_8 t3_widget_pcre2_match
#define pcre2_get_error_message_8 t3_widget_pcre2_get_error_message
#define pcre2_substring_number_from_name_8 t3_widget_pcre2_substring_number_from_name

T3_WIDGET_LOCAL pcre2_code_8 *pcre2_compile_8(PCRE2_SPTR8 pattern, PCRE2_SIZE pattern_size,
                                              uint32_t options, int *errorcode,
                                              PCRE2_SIZE *erroroffset, void *ccontext);

T3_WIDGET_LOCAL int pcre2_jit_compile_8(pcre2_code_8 *code, uint32_t options);
T3_WIDGET_LOCAL pcre2_match_data_8 *pcre2_match_data_create_8(uint32_t ovecsize, void *gcontext);
T3_WIDGET_LOCAL void pcre2_match_data_free_8(pcre2_match_data_8 *match_data);
T3_WIDGET_LOCAL pcre2_match_data_8 *pcre2_match_data_create_from_pattern_8(const pcre2_code_8 *code,
                                                                           void *gcontext);
T3_WIDGET_LOCAL PCRE2_SIZE *pcre2_get_ovector_pointer_8(pcre2_match_data_8 *match_data);
T3_WIDGET_LOCAL uint32_t pcre2_get_ovector_count_8(pcre2_match_data_8 *match_data);
T3_WIDGET_LOCAL void pcre2_code_free_8(pcre2_code_8 *code);
T3_WIDGET_LOCAL int pcre2_match_8(const pcre2_code_8 *code, PCRE2_SPTR8 subject, PCRE2_SIZE length,
                                  PCRE2_SIZE startoffset, uint32_t options,
                                  pcre2_match_data_8 *match_data, void *mcontext);

T3_WIDGET_LOCAL int pcre2_get_error_message_8(int errorcode, PCRE2_UCHAR8 *buffer,
                                              PCRE2_SIZE bufflen);
#else
T3_WIDGET_LOCAL extern int _t3_widget_no_empty_translation_unit;
#endif

#endif  // PCRE_COMPAT_H
