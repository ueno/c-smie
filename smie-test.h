/*
 * Copyright (C) 2015 Daiki Ueno
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef __SMIE_TEST_H__
#define __SMIE_TEST_H__ 1

#include "smie-indenter.h"
#include "smie-private.h"

G_BEGIN_DECLS

gboolean smie_test_bnf_grammar_equal (smie_bnf_grammar_t *a,
				      smie_bnf_grammar_t *b);
gboolean smie_test_prec2_grammar_equal (smie_prec2_grammar_t *a,
					smie_prec2_grammar_t *b);
gboolean smie_test_precs_grammar_equal (smie_precs_grammar_t *a,
					smie_precs_grammar_t *b);

typedef struct smie_test_context_t smie_test_context_t;
struct smie_test_context_t
{
  const gchar *input;
  goffset offset;
};

gboolean smie_test_advance_func (smie_advance_step_t step, gint count,
				 gpointer user_data);
gboolean smie_test_read_token_func (gchar **token, gpointer user_data);
gunichar smie_test_read_char_func (gpointer user_data);

smie_cursor_functions_t smie_test_cursor_functions;

G_END_DECLS

#endif	/* __SMIE_TEST_H__ */
