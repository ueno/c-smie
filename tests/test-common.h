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
#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__ 1

#include <smie/smie-indenter.h>
#include <smie/smie-private.h>

G_BEGIN_DECLS

gboolean test_common_bnf_grammar_equal (smie_bnf_grammar_t *a,
				      smie_bnf_grammar_t *b);
gboolean test_common_prec2_grammar_equal (smie_prec2_grammar_t *a,
					smie_prec2_grammar_t *b);
gboolean test_common_grammar_equal (smie_grammar_t *a,
				  smie_grammar_t *b);

typedef struct test_common_context_t test_common_context_t;
struct test_common_context_t
{
  const gchar *input;
  goffset offset;
  GList *stack;
};

smie_cursor_functions_t test_common_cursor_functions;

G_END_DECLS

#endif	/* __TEST_COMMON_H__ */
