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

/**
 * SECTION:smie-indenter
 * @short_description: entry point of indentation engine
 * @title: Indenter
 * @section_id:
 * @stability: Unstable
 * @include: smie/smie.h
 *
 * The indenter object provides generic indentatin computation.
 */

struct _smie_indenter_t
{
  volatile gint ref_count;

  smie_grammar_t *grammar;
  smie_cursor_functions_t *functions;
  gint step;
};

/**
 * smie_indenter_new:
 * @grammar: a #smie_grammar_t object
 * @step: a unit of a single indentation
 * @functions: a #smie_cursor_functions_t
 *
 * Create a new indenter.
 * Returns: a new #smie_indenter_t object
 */
smie_indenter_t *
smie_indenter_new (smie_grammar_t *grammar,
		   gint step,
		   smie_cursor_functions_t *functions)
{
  smie_indenter_t *result;

  g_return_val_if_fail (grammar, NULL);
  g_return_val_if_fail (step >= 0, NULL);
  g_return_val_if_fail (functions, NULL);

  result = g_new0 (smie_indenter_t, 1);
  result->ref_count = 1;
  result->grammar = grammar;
  result->step = step;
  result->functions = functions;
  return result;
}

static void
smie_indenter_free (smie_indenter_t *indenter)
{
  smie_grammar_free (indenter->grammar);
  g_free (indenter);
}

/**
 * smie_indenter_ref:
 * @indenter: a #smie_indenter_t object
 *
 * Increment reference count of @indenter.
 * Returns: @indenter
 */
smie_indenter_t *
smie_indenter_ref (smie_indenter_t *indenter)
{
  g_return_val_if_fail (indenter, NULL);
  g_return_val_if_fail (indenter->ref_count > 0, NULL);
  g_atomic_int_inc (&indenter->ref_count);
  return indenter;
}

/**
 * smie_indenter_unref:
 * @indenter: a #smie_indenter_t object
 *
 * Decrement reference count of @indenter.  If the count reaches zero,
 * the memory allocated for @indenter will be released.
 */
void
smie_indenter_unref (smie_indenter_t *indenter)
{
  g_return_if_fail (indenter);
  g_return_if_fail (indenter->ref_count > 0);
  if (g_atomic_int_dec_and_test (&indenter->ref_count))
    smie_indenter_free (indenter);
}

static gboolean
smie_indent_starts_line (smie_indenter_t *indenter,
			 gpointer context)
{
  if (indenter->functions->starts_line (context))
    return TRUE;

  indenter->functions->push_context (context);
  while (indenter->functions->backward_char (context)
	 && !indenter->functions->starts_line (context))
    {
      gunichar uc = indenter->functions->get_char (context);
      if (!(uc == ' ' || uc == '\t'))
	{
	  indenter->functions->pop_context (context);
	  return FALSE;
	}
    }
  indenter->functions->pop_context (context);
  return TRUE;
}

typedef gint (* smie_indent_function_t) (smie_indenter_t *, gpointer);

static gint
smie_indent_virtual (smie_indenter_t *indenter,
		     gpointer context)
{
  if (smie_indent_starts_line (indenter, context))
    return indenter->functions->get_line_offset (context);
  return smie_indenter_calculate (indenter, context);
}

static gint
smie_indent_bob (smie_indenter_t *indenter, gpointer context)
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
smie_indent_keyword (smie_indenter_t *indenter, gpointer context)
{
  gint offset = indenter->functions->get_offset (context), offset2;
  gchar *token, *parent_token;
  smie_symbol_pool_t *pool;
  const smie_symbol_t *symbol, *parent_symbol;
  smie_symbol_class_t symbol_class;
  gint left_prec, parent_left_prec;
  gint indent;

  indenter->functions->push_context (context);
  token = indenter->functions->forward_token (context);
  indenter->functions->pop_context (context);
  if (!token)
    return -1;

  pool = smie_grammar_get_symbol_pool (indenter->grammar);
  symbol = smie_symbol_intern (pool, token, SMIE_SYMBOL_TERMINAL);
  g_free (token);

  if (!smie_grammar_is_keyword (indenter->grammar, symbol))
    return -1;

  symbol_class = smie_grammar_get_symbol_class (indenter->grammar, symbol);
  if (symbol_class == SMIE_SYMBOL_CLASS_OPENER)
    {
      /* FIXME: Skip comments.  */
      if (smie_indent_starts_line (indenter, context))
	return -1;

      /* FIXME: Check if the token is hanging.  */
      indent = indenter->functions->get_line_offset (context);
      return indent;
    }

  offset2 = indenter->functions->get_offset (context);
  indenter->functions->push_context (context);
  smie_backward_sexp (indenter->grammar,
		      indenter->functions->backward_token,
		      symbol,
		      context);
  if (offset2 == indenter->functions->get_offset (context))
    {
      indenter->functions->pop_context (context);
      return -1;
    }

  indenter->functions->push_context (context);
  parent_token = indenter->functions->forward_token (context);
  indenter->functions->pop_context (context);
  if (!parent_token)
    {
      indenter->functions->pop_context (context);
      return -1;
    }
  parent_symbol = smie_symbol_intern (pool, parent_token, SMIE_SYMBOL_TERMINAL);
  g_free (parent_token);

  /* For later calls to smie_indent_virtual, place the cursor at the
     beginnning of the first token on the line.  */
  if (indenter->functions->ends_line (context))
    indenter->functions->forward_char (context);
  indenter->functions->forward_comment (context);

  left_prec
    = smie_grammar_get_left_prec (indenter->grammar, symbol);
  parent_left_prec
    = smie_grammar_get_left_prec (indenter->grammar, parent_symbol);

  if (left_prec == parent_left_prec)
    {
      if (offset != indenter->functions->get_offset (context)
	  && smie_indent_starts_line (indenter, context))
	{
	  indenter->functions->pop_context (context);
	  return indenter->functions->get_line_offset (context);
	}

      indent = smie_indent_virtual (indenter, context);
      indenter->functions->pop_context (context);
      return indent;
    }

  if (offset == indenter->functions->get_offset (context)
      && smie_indent_starts_line (indenter, context))
    {
      indenter->functions->pop_context (context);
      return -1;
    }

  if (smie_grammar_is_keyword (indenter->grammar, parent_symbol))
    {
      indent = indenter->functions->get_line_offset (context);
      indenter->functions->pop_context (context);
      return indent;
    }

  indent = smie_indent_virtual (indenter, context);
  indenter->functions->pop_context (context);
  return indent;
}

static gint
smie_indent_after_keyword (smie_indenter_t *indenter, gpointer context)
{
  gchar *token;
  smie_symbol_pool_t *pool;
  const smie_symbol_t *symbol;
  smie_symbol_class_t symbol_class;
  gint indent;

  indenter->functions->push_context (context);
  token = indenter->functions->backward_token (context);
  if (!token)
    {
      indenter->functions->pop_context (context);
      return -1;
    }

  pool = smie_grammar_get_symbol_pool (indenter->grammar);
  symbol = smie_symbol_intern (pool, token, SMIE_SYMBOL_TERMINAL);
  g_free (token);
  if (!smie_grammar_is_keyword (indenter->grammar, symbol))
    {
      indenter->functions->pop_context (context);
      return -1;
    }

  symbol_class = smie_grammar_get_symbol_class (indenter->grammar, symbol);
  if (symbol_class == SMIE_SYMBOL_CLASS_CLOSER)
    {
      indenter->functions->pop_context (context);
      return -1;
    }

  /* For later calls to smie_indent_virtual, place the cursor at the
     beginnning of the first token on the line.  */
  if (indenter->functions->ends_line (context))
    indenter->functions->forward_char (context);
  indenter->functions->forward_comment (context);

  if (symbol_class == SMIE_SYMBOL_CLASS_OPENER
      || smie_grammar_is_pair_end (indenter->grammar, symbol))
    {
      indent = smie_indent_virtual (indenter, context) + indenter->step;
      indenter->functions->pop_context (context);
      return indent;
    }
  indent = smie_indent_virtual (indenter, context);
  indenter->functions->pop_context (context);
  return indent;
}

static smie_indent_function_t functions[] =
  {
    smie_indent_bob,
    smie_indent_keyword,
    smie_indent_after_keyword
    /* FIXME: implement smie_indent_exps */
  };

/**
 * smie_indenter_calculate:
 * @indenter: a #smie_indenter_t object
 * @context: cursor context
 *
 * Calculate the indentation level of the current line.
 * Returns: an indent value, or -1 if it is not determined.
 */
gint
smie_indenter_calculate (smie_indenter_t *indenter, gpointer context)
{
  gint i;

  indenter->functions->backward_to_line_start (context);
  for (i = 0; i < G_N_ELEMENTS (functions); i++)
    {
      gint indent = functions[i] (indenter, context);
      if (indent >= 0)
	return indent;
    }
  return -1;
}
