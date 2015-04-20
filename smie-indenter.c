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

  smie_precs_grammar_t *grammar;
  smie_advance_function_t advance_func;
  smie_read_function_t read_func;
};

struct smie_indenter_t *
smie_indenter_new (smie_precs_grammar_t *grammar,
		   smie_advance_function_t advance_func,
		   smie_read_function_t read_func)
{
  struct smie_indenter_t *result = g_new0 (struct smie_indenter_t, 1);
  result->grammar = grammar;
  result->advance_func = advance_func;
  result->read_func = read_func;
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
  return 0;
}
