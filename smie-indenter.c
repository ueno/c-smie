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

#include "smie-indenter.h"

struct smie_indenter_t
{
  volatile gint ref_count;

  smie_grammar_t *grammar;
  smie_cursor_functions_t *functions;
  gint step;
};

struct smie_indenter_t *
smie_indenter_new (smie_grammar_t *grammar,
		   gint step,
		   smie_cursor_functions_t *functions)
{
  struct smie_indenter_t *result;

  g_return_val_if_fail (grammar, NULL);
  g_return_val_if_fail (step >= 0, NULL);
  g_return_val_if_fail (functions, NULL);

  result = g_new0 (struct smie_indenter_t, 1);
  result->ref_count = 1;
  result->grammar = grammar;
  result->step = step;
  result->functions = functions;
  return result;
}

static void
smie_indenter_free (struct smie_indenter_t *indenter)
{
  smie_grammar_free (indenter->grammar);
  g_free (indenter);
}

struct smie_indenter_t *
smie_indenter_ref (struct smie_indenter_t *indenter)
{
  g_return_val_if_fail (indenter, NULL);
  g_return_val_if_fail (indenter->ref_count > 0, NULL);
  g_atomic_int_inc (&indenter->ref_count);
  return indenter;
}

void
smie_indenter_unref (struct smie_indenter_t *indenter)
{
  g_return_if_fail (indenter);
  g_return_if_fail (indenter->ref_count > 0);
  if (g_atomic_int_dec_and_test (&indenter->ref_count))
    smie_indenter_free (indenter);
}

static gint
smie_indenter_calculate_bol_column (struct smie_indenter_t *indenter,
				    gpointer context)
{
  gint column = 0;
  indenter->functions->backward_to_line_start (context);
  do
    {
      gunichar uc = indenter->functions->read_char (context);
      if (!(0x20 <= uc && uc <= 0x7F && g_ascii_isspace (uc)))
	break;
      column++;
    }
  while (indenter->functions->forward_char (context));
  return column;
}

static gboolean
smie_indent_starts_line (struct smie_indenter_t *indenter,
			 gpointer context)
{
  indenter->functions->push_context (context);
  if (indenter->functions->starts_line (context))
    {
      indenter->functions->pop_context (context);
      return TRUE;
    }

  while (indenter->functions->backward_char (context)
	 && !indenter->functions->starts_line (context))
    {
      gunichar uc = indenter->functions->read_char (context);
      if (!(uc == ' ' || uc == '\t'))
	{
	  indenter->functions->pop_context (context);
	  return FALSE;
	}
    }

  indenter->functions->pop_context (context);
  return TRUE;
}

typedef gint (* smie_indent_function_t) (struct smie_indenter_t *, gpointer);

static gint
smie_indent_virtual (struct smie_indenter_t *indenter,
		     gpointer context)
{
  if (smie_indent_starts_line (indenter, context))
    return indenter->functions->get_line_offset (context);
  return smie_indenter_calculate (indenter, context);
}

static gint
smie_indent_bob (struct smie_indenter_t *indenter,
		 gpointer context)
{
  gboolean result;

  indenter->functions->push_context (context);
  indenter->functions->backward_comment (context);
  result = indenter->functions->is_start (context);
  indenter->functions->pop_context (context);
  if (result)
    return 0;

  return -1;
}

static gint
smie_indent_keyword (struct smie_indenter_t *indenter,
		     gpointer context)
{
  gchar *token;
  gboolean result;
  const smie_symbol_t *symbol;
  smie_symbol_class_t symbol_class;
  smie_symbol_pool_t *pool;

  /* If the line starts with a closer, move backward to the the
     matching opener and use the indentation.  */
  indenter->functions->backward_to_line_start (context);
  if (!indenter->functions->read_token (context, NULL))
    indenter->functions->forward_token (context, FALSE);
  if (!indenter->functions->read_token (context, &token))
    return 0;

  pool = smie_grammar_get_symbol_pool (indenter->grammar);
  symbol = smie_symbol_intern (pool, token, SMIE_SYMBOL_TERMINAL);
  g_free (token);
  if (smie_grammar_is_pair_end (indenter->grammar, symbol))
    {
      indenter->functions->push_context (context);
      if (smie_backward_sexp (indenter->grammar,
			      indenter->functions->backward_token,
			      indenter->functions->read_token,
			      context))
	{
	  gint indent = smie_indenter_calculate_bol_column (indenter, context);
	  indenter->functions->pop_context (context);
	  return indent;
	}

      if (indenter->functions->read_token (context, &token))
	{
	  symbol = smie_symbol_intern (pool, token, SMIE_SYMBOL_TERMINAL);
	  symbol_class = smie_grammar_get_symbol_class (indenter->grammar,
							symbol);
	  g_free (token);
	  if (symbol_class == SMIE_SYMBOL_CLASS_OPENER)
	    {
	      gint indent = smie_indenter_calculate_bol_column (indenter,
								context);
	      indenter->functions->pop_context (context);
	      return indent;
	    }
	}
      indenter->functions->pop_context (context);
    }

  /* If the previous line starts with a last closer, align to it.  */
  indenter->functions->backward_line (context);
  indenter->functions->backward_to_line_start (context);
  if (!indenter->functions->read_token (context, NULL))
    indenter->functions->forward_token (context, FALSE);
  if (indenter->functions->read_token (context, &token))
    {
      symbol = smie_symbol_intern (pool, token, SMIE_SYMBOL_TERMINAL);
      symbol_class = smie_grammar_get_symbol_class (indenter->grammar, symbol);
      g_free (token);
      if (symbol_class == SMIE_SYMBOL_CLASS_CLOSER)
	return smie_indenter_calculate_bol_column (indenter, context);
    }

  /* Otherwise increment the indentation by indenter->step.  */
  return indenter->step
    + smie_indenter_calculate_bol_column (indenter, context);
}

static smie_indent_function_t functions[] =
  {
    smie_indent_bob,
    smie_indent_keyword
  };

gint
smie_indenter_calculate (struct smie_indenter_t *indenter,
			 gpointer context)
{
  gint i;
  for (i = 0; i < G_N_ELEMENTS (functions); i++)
    {
      gint indent = functions[i] (indenter, context);
      if (indent >= 0)
	return indent;
    }
  return -1;
}
