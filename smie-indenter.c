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
  smie_advance_function_t advance_func;
  smie_read_function_t read_func;
  smie_read_char_function_t read_char_func;
};

struct smie_indenter_t *
smie_indenter_new (smie_symbol_pool_t *pool,
		   smie_precs_grammar_t *grammar,
		   smie_advance_function_t advance_func,
		   smie_read_function_t read_func,
		   smie_read_char_function_t read_char_func)
{
  struct smie_indenter_t *result = g_new0 (struct smie_indenter_t, 1);
  result->pool = pool;
  result->grammar = grammar;
  result->advance_func = advance_func;
  result->read_func = read_func;
  result->read_char_func = read_char_func;
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

gint
smie_indenter_calculate (struct smie_indenter_t *indenter,
			 gpointer callback)
{
  gchar *token;
  const smie_symbol_t *symbol;
  gint column = 0;

  indenter->advance_func (SMIE_ADVANCE_LINE_ENDS, 1, callback);
  indenter->advance_func (SMIE_ADVANCE_TOKENS, -1, callback);

  if (indenter->read_func (&token, callback)
      && (symbol = smie_symbol_intern (indenter->pool,
				       token,
				       SMIE_SYMBOL_TERMINAL),
	  smie_precs_grammar_is_closer (indenter->grammar, symbol)))
    smie_backward_sexp (indenter->grammar,
			indenter->advance_func,
			indenter->read_func,
			callback);
  else
    {
      indenter->advance_func (SMIE_ADVANCE_LINES, -1, callback);
      indenter->advance_func (SMIE_ADVANCE_LINE_ENDS, -1, callback);
    }

  indenter->advance_func (SMIE_ADVANCE_LINE_ENDS, -1, callback);
  while (indenter->advance_func (SMIE_ADVANCE_CHARACTERS, 1, callback))
    {
      gunichar uc = indenter->read_char_func (callback);
      if (!(0x20 <= uc && uc <= 0x7F && g_ascii_isspace (uc)))
	break;
      column++;
    }

  return column;
}
