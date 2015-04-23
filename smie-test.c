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

#include "smie-test.h"

#include <string.h>

static gboolean
smie_rule_equal (struct smie_rule_t *a,
		 struct smie_rule_t *b)
{
  GList *al, *bl;
  for (al = a->symbols, bl = b->symbols; al && bl; al = al->next, bl = bl->next)
    if (al->data != bl->data)
      return FALSE;
  return al == NULL && bl == NULL;
}

static gboolean
smie_rule_list_equal (struct smie_rule_list_t *a,
		      struct smie_rule_list_t *b)
{
  struct { GList *from, *to; } permutations[2];
  gint i;

  permutations[0].from = a->rules;
  permutations[0].to = b->rules;
  permutations[1].from = b->rules;
  permutations[1].to = a->rules;

  for (i = 0; i < G_N_ELEMENTS (permutations); i++)
    {
      GList *l;
      for (l = permutations[i].from; l; l = l->next)
	{
	  GList *l2;
	  gboolean found = FALSE;
	  for (l2 = permutations[i].to; l2; l2 = l2->next)
	    {
	      if (smie_rule_equal (l->data, l2->data))
		{
		  found = TRUE;
		  break;
		}
	    }
	  if (!found)
	    return FALSE;
	}
    }
  return TRUE;
}

gboolean
smie_test_bnf_grammar_equal (struct smie_bnf_grammar_t *a,
			     struct smie_bnf_grammar_t *b)
{
  struct { GHashTable *from, *to; } permutations[2];
  gint i;

  permutations[0].from = a->rules;
  permutations[0].to = b->rules;
  permutations[1].from = b->rules;
  permutations[1].to = a->rules;

  for (i = 0; i < G_N_ELEMENTS (permutations); i++)
    {
      GHashTableIter iter;
      gpointer key, value;
      g_hash_table_iter_init (&iter, permutations[i].from);
      while (g_hash_table_iter_next (&iter, &key, &value))
	{
	  gpointer key1, value1;
	  if (!g_hash_table_lookup_extended (permutations[i].to, key, &key1,
					     &value1)
	      || key != key1
	      || !smie_rule_list_equal (value, value1))
	    return FALSE;
	}
    }
  return TRUE;
}

gboolean
smie_test_prec2_grammar_equal (struct smie_prec2_grammar_t *a,
			       struct smie_prec2_grammar_t *b)
{
  struct { struct smie_prec2_grammar_t *from, *to; } permutations[2];
  gint i;

  permutations[0].from = a;
  permutations[0].to = b;
  permutations[1].from = b;
  permutations[1].to = a;

  for (i = 0; i < G_N_ELEMENTS (permutations); i++)
    {
      GHashTableIter iter;
      gpointer key, value;
      g_hash_table_iter_init (&iter, permutations[i].from->prec2);
      while (g_hash_table_iter_next (&iter, &key, &value))
	{
	  gpointer key1, value1;
	  if (!g_hash_table_lookup_extended (permutations[i].to->prec2, key,
					     &key1, &value1))
	    return FALSE;
	}
      g_hash_table_iter_init (&iter, permutations[i].from->first);
      while (g_hash_table_iter_next (&iter, &key, &value))
	{
	  gpointer key1, value1;
	  if (!g_hash_table_lookup_extended (permutations[i].to->first, key,
					     &key1, &value1)
	      || key != key1)
	    return FALSE;
	}
      g_hash_table_iter_init (&iter, permutations[i].from->last);
      while (g_hash_table_iter_next (&iter, &key, &value))
	{
	  gpointer key1, value1;
	  if (!g_hash_table_lookup_extended (permutations[i].to->last, key,
					     &key1, &value1)
	      || key != key1)
	    return FALSE;
	}
      g_hash_table_iter_init (&iter, permutations[i].from->closer);
      while (g_hash_table_iter_next (&iter, &key, &value))
	{
	  gpointer key1, value1;
	  if (!g_hash_table_lookup_extended (permutations[i].to->closer, key,
					     &key1, &value1)
	      || key != key1)
	    return FALSE;
	}
      g_hash_table_iter_init (&iter, permutations[i].from->opener_closer);
      while (g_hash_table_iter_next (&iter, &key, &value))
	if (!g_hash_table_contains (permutations[i].to->opener_closer, key))
	  return FALSE;
    }
  return TRUE;
}

gboolean
smie_test_grammar_equal (struct smie_grammar_t *a,
			 struct smie_grammar_t *b)
{
  struct { struct smie_grammar_t *from, *to; } permutations[2];
  gint i;

  permutations[0].from = a;
  permutations[0].to = b;
  permutations[1].from = b;
  permutations[1].to = a;

  for (i = 0; i < G_N_ELEMENTS (permutations); i++)
    {
      GHashTableIter iter;
      gpointer key, value;
      g_hash_table_iter_init (&iter, permutations[i].from->precs);
      while (g_hash_table_iter_next (&iter, &key, &value))
	{
	  gpointer key1, value1;
	  if (!g_hash_table_lookup_extended (permutations[i].to->precs, key,
					     &key1, &value1)
	      || key != key1
	      || memcmp (value, value1, sizeof (struct smie_prec_t)) != 0)
	    return FALSE;
	}

      /* FIXME: Check opener_closer.  */
    }
  return TRUE;
}

static goffset
smie_test_forward_char (goffset offset, const gchar *input)
{
  if (input[offset] != '\0')
    offset++;
  return offset;
}

static goffset
smie_test_backward_char (goffset offset, const gchar *input)
{
  if (offset > 0)
    offset--;
  return offset;
}

static goffset
smie_test_end_of_line (goffset offset, const gchar *input)
{
  while (input[offset] != '\0' && input[offset] != '\n')
    offset++;
  return offset;
}

static goffset
smie_test_beginning_of_line (goffset offset, const gchar *input)
{
  while (offset > 0 && input[offset] != '\n')
    offset--;
  if (input[offset] == '\n')
    offset++;
  return offset;
}

static goffset
smie_test_forward_line (goffset offset, const gchar *input)
{
  offset = smie_test_end_of_line (offset, input);
  if (input[offset] != '\0' && input[offset + 1] == '\n')
    offset++;
  return offset;
}

static goffset
smie_test_backward_line (goffset offset, const gchar *input)
{
  offset = smie_test_beginning_of_line (offset, input);
  if (offset > 1 && input[offset - 1] == '\n')
    {
      offset = smie_test_backward_char (offset, input);
      offset = smie_test_backward_char (offset, input);
    }
  return offset;
}

static goffset
smie_test_forward_token (goffset offset, const gchar *input)
{
  while (input[offset] != '\0' && !g_ascii_isspace (input[offset]))
    offset++;
  while (input[offset] != '\0' && g_ascii_isspace (input[offset]))
    offset++;
  return offset;
}

static goffset
smie_test_backward_token (goffset offset, const gchar *input)
{
  while (offset > 0 && !g_ascii_isspace (input[offset]))
    offset = smie_test_backward_char (offset, input);
  while (offset > 0 && g_ascii_isspace (input[offset]))
    offset = smie_test_backward_char (offset, input);
  return offset;
}

typedef goffset (*smie_test_movement_function_t) (goffset, const gchar *);

gboolean
smie_test_advance_func (smie_advance_step_t step, gint count,
			gpointer user_data)
{
  struct smie_test_context_t *context = user_data;
  smie_test_movement_function_t forward_func, backward_func;
  goffset offset = context->offset;
  gboolean result;
  switch (step)
    {
    case SMIE_ADVANCE_CHARACTERS:
      forward_func = smie_test_forward_char;
      backward_func = smie_test_backward_char;
      break;

    case SMIE_ADVANCE_LINES:
      forward_func = smie_test_forward_line;
      backward_func = smie_test_backward_line;
      break;

    case SMIE_ADVANCE_LINE_ENDS:
      forward_func = smie_test_end_of_line;
      backward_func = smie_test_beginning_of_line;
      break;

    case SMIE_ADVANCE_TOKENS:
      forward_func = smie_test_forward_token;
      backward_func = smie_test_backward_token;
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  if (count > 0)
    for (; count > 0; count--)
      offset = forward_func (offset, context->input);
  else
    for (; count < 0; count++)
      offset = backward_func (offset, context->input);
  result = offset != context->offset;
  context->offset = offset;
  return result;
}

gboolean
smie_test_inspect_func (smie_inspect_request_t request, gpointer user_data)
{
  struct smie_test_context_t *context = user_data;
  goffset offset2;
  switch (request)
    {
    case SMIE_INSPECT_HAS_PREVIOUS_LINE:
      offset2 = smie_test_beginning_of_line (context->offset, context->input);
      return offset2 > 0 && context->input[offset2 - 1] == '\n';

    default:
      g_return_val_if_reached (FALSE);
    }
}

gboolean
smie_test_read_token_func (gchar **token, gpointer user_data)
{
  struct smie_test_context_t *context = user_data;
  goffset start_offset, end_offset;
  if (context->offset < 0
      || context->input[context->offset] == '\0'
      || g_ascii_isspace (context->input[context->offset]))
    return FALSE;
  start_offset = context->offset;
  while (start_offset - 1 >= 0
	 && !g_ascii_isspace (context->input[start_offset - 1]))
    start_offset--;
  end_offset = context->offset;
  while (context->input[end_offset] != '\0'
	 && !g_ascii_isspace (context->input[end_offset]))
    end_offset++;
  if (token)
    *token = g_strndup (&context->input[start_offset],
			end_offset - (start_offset));
  return TRUE;
}

gunichar
smie_test_read_char_func (gpointer user_data)
{
  struct smie_test_context_t *context = user_data;
  if (context->offset < 0
      || context->input[context->offset] == '\0')
    return (gunichar) -1;
  return g_utf8_get_char_validated (&context->input[context->offset], -1);
}

smie_cursor_functions_t smie_test_cursor_functions =
  {
    smie_test_advance_func,
    smie_test_inspect_func,
    smie_test_read_token_func,
    smie_test_read_char_func
  };
