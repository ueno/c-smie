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

  smie_symbol_pool_t *pool;
  smie_precs_grammar_t *grammar;
  smie_cursor_functions_t *functions;
  gint step;
};

struct smie_indenter_t *
smie_indenter_new (smie_symbol_pool_t *pool,
		   smie_precs_grammar_t *grammar,
		   gint step,
		   smie_cursor_functions_t *functions)
{
  struct smie_indenter_t *result;

  g_return_val_if_fail (pool, NULL);
  g_return_val_if_fail (grammar, NULL);
  g_return_val_if_fail (step >= 0, NULL);
  g_return_val_if_fail (functions
			&& functions->advance
			&& functions->read_token
			&& functions->read_char, NULL);

  result = g_new0 (struct smie_indenter_t, 1);
  result->ref_count = 1;
  result->pool = pool;
  result->grammar = grammar;
  result->step = step;
  result->functions = functions;
  return result;
}

static void
smie_indenter_free (struct smie_indenter_t *indenter)
{
  smie_precs_grammar_free (indenter->grammar);
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
				    gpointer callback)
{
  gint column = 0;
  indenter->functions->advance (SMIE_ADVANCE_LINE_ENDS, -1, callback);
  while (indenter->functions->advance (SMIE_ADVANCE_CHARACTERS, 1, callback))
    {
      gunichar uc = indenter->functions->read_char (callback);
      if (!(0x20 <= uc && uc <= 0x7F && g_ascii_isspace (uc)))
	break;
      column++;
    }
  return column;
}

gint
smie_indenter_calculate (struct smie_indenter_t *indenter,
			 gpointer callback)
{
  gchar *token;
  const smie_symbol_t *symbol;

  /* Check if this is the first line.  */
  indenter->functions->advance (SMIE_ADVANCE_LINE_ENDS, -1, callback);
  if (!indenter->functions->advance (SMIE_ADVANCE_CHARACTERS, -1, callback))
    return 0;

  indenter->functions->advance (SMIE_ADVANCE_CHARACTERS, 1, callback);
  indenter->functions->advance (SMIE_ADVANCE_LINE_ENDS, 1, callback);
  indenter->functions->advance (SMIE_ADVANCE_TOKENS, -1, callback);

  if (!indenter->functions->read_token (&token, callback))
    return 0;

  symbol = smie_symbol_intern (indenter->pool, token,
			       SMIE_SYMBOL_TERMINAL);

  if (smie_precs_grammar_is_closer (indenter->grammar, symbol))
    {
      if (smie_backward_sexp (indenter->grammar,
			      indenter->functions->advance,
			      indenter->functions->read_token,
			      callback))
	return smie_indenter_calculate_bol_column (indenter, callback);
    }

  indenter->functions->advance (SMIE_ADVANCE_LINES, -1, callback);
  return indenter->step
    + smie_indenter_calculate_bol_column (indenter, callback);
}
