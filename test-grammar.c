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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "smie-grammar.h"
#include <glib/gprintf.h>
#include <string.h>

extern void
smie_debug_dump_prec2_grammar (struct smie_prec2_grammar_t *grammar);
extern void
smie_debug_dump_precs_grammar (struct smie_precs_grammar_t *grammar);

static smie_bnf_grammar_t *
populate_bnf_grammar (smie_symbol_pool_t *pool)
{
  smie_bnf_grammar_t *grammar = smie_bnf_grammar_alloc ();
  GList *rule;

  /* S -> "#" E "#" */
  rule = g_list_append (NULL, (gpointer) smie_symbol_intern (pool, "S", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "#", SMIE_SYMBOL_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "E", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "#", SMIE_SYMBOL_TERMINAL));
  smie_bnf_grammar_add_rule (grammar, rule);

  /* E -> E "+" T */
  rule = g_list_append (NULL, (gpointer) smie_symbol_intern (pool, "E", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "E", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "+", SMIE_SYMBOL_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "T", SMIE_SYMBOL_NON_TERMINAL));
  smie_bnf_grammar_add_rule (grammar, rule);

  /* E -> T */
  rule = g_list_append (NULL, (gpointer) smie_symbol_intern (pool, "E", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "T", SMIE_SYMBOL_NON_TERMINAL));
  smie_bnf_grammar_add_rule (grammar, rule);

  /* T -> T "x" F */
  rule = g_list_append (NULL, (gpointer) smie_symbol_intern (pool, "T", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "T", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "x", SMIE_SYMBOL_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "F", SMIE_SYMBOL_NON_TERMINAL));
  smie_bnf_grammar_add_rule (grammar, rule);

  /* T -> F */
  rule = g_list_append (NULL, (gpointer) smie_symbol_intern (pool, "T", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "F", SMIE_SYMBOL_NON_TERMINAL));
  smie_bnf_grammar_add_rule (grammar, rule);

  /* F -> n */
  rule = g_list_append (NULL, (gpointer) smie_symbol_intern (pool, "F", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "n", SMIE_SYMBOL_TERMINAL_VAR));
  smie_bnf_grammar_add_rule (grammar, rule);

  /* F -> "(" E ")" */
  rule = g_list_append (NULL, (gpointer) smie_symbol_intern (pool, "F", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "(", SMIE_SYMBOL_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, "E", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, (gpointer) smie_symbol_intern (pool, ")", SMIE_SYMBOL_TERMINAL));
  smie_bnf_grammar_add_rule (grammar, rule);

  return grammar;
}

static goffset
forward_token (goffset offset, const gchar *input)
{
  while (input[offset] != '\0' && g_ascii_isspace (input[offset]))
    offset++;
  while (input[offset] != '\0' && !g_ascii_isspace (input[offset]))
    offset++;
  return offset;
}

static goffset
backward_token (goffset offset, const gchar *input)
{
  while (offset >= 0 && g_ascii_isspace (input[offset]))
    offset--;
  while (offset >= 0 && !g_ascii_isspace (input[offset]))
    offset--;
  return offset;
}

static gboolean
advance_func (smie_advance_step_t step, gint count, goffset *offsetp,
	      gpointer user_data)
{
  gchar *input = user_data;
  goffset offset = *offsetp;
  switch (step)
    {
    case SMIE_ADVANCE_TOKEN:
      if (count > 0)
	for (; count > 0; count--)
	  offset = forward_token (offset, input);
      else
	for (; count < 0; count++)
	  offset = backward_token (offset, input);
      return offset != *offsetp;
      break;
    default:
      g_return_val_if_reached (FALSE);
    }
}

static gboolean
read_func (gchar **token, goffset offset, gpointer user_data)
{
  gchar *input = user_data;
  goffset end_offset;
  if (input[offset] == '\0' || g_ascii_isspace (input[offset]))
    return FALSE;
  end_offset = offset;
  while (input[end_offset] != '\0' && !g_ascii_isspace (input[end_offset]))
    end_offset++;
  *token = g_strndup (&input[offset], end_offset - offset);
  return TRUE;
}

int
main (void)
{
  smie_symbol_pool_t *pool = smie_symbol_pool_alloc ();
  smie_bnf_grammar_t *bnf = populate_bnf_grammar (pool);
  smie_prec2_grammar_t *prec2 = smie_prec2_grammar_alloc ();
  smie_precs_grammar_t *precs = smie_precs_grammar_alloc ();
  goffset offset;

  smie_bnf_grammar_t *bnf2;
  bnf2 = smie_bnf_grammar_from_string (pool, "\
s: \"#\" e \"#\";\n\
e: e \"+\" t | t;\n\
t: t \"x\" f | f;\n\
f: N | \"(\" e \")\";");
  smie_bnf_to_prec2 (bnf2, prec2);
  smie_bnf_grammar_free (bnf2);
  smie_debug_dump_prec2_grammar (prec2);
  smie_prec2_to_precs (prec2, precs);
  smie_prec2_grammar_free (prec2);
  smie_debug_dump_precs_grammar (precs);

#if 0
  offset = 0;
  smie_forward_sexp (precs, &offset, advance_func, read_func,
		     "# 4 + 5 x 6 + 8 #");
  g_printf ("%d\n", offset);
#endif

  offset = 1;
  smie_forward_sexp (precs, &offset, advance_func, read_func,
		     "# ( 4 + ( 5 x 6 ) + 7 ) + 8 #");
  g_printf ("%zd\n", offset);

  offset = 23;
  smie_backward_sexp (precs, &offset, advance_func, read_func,
		      "# ( 4 + ( 5 x 6 ) + 7 ) + 8 #");
  g_printf ("%zd\n", offset);

  smie_precs_grammar_free (precs);
  smie_symbol_pool_free (pool);
  return 0;
}
