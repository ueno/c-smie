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
      g_hash_table_iter_init (&iter, permutations[i].from->classes);
      while (g_hash_table_iter_next (&iter, &key, &value))
	{
	  gpointer key1, value1;
	  if (!g_hash_table_lookup_extended (permutations[i].to->classes, key,
					     &key1, &value1)
	      || key != key1
	      || value != value1)
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
      g_hash_table_iter_init (&iter, permutations[i].from->levels);
      while (g_hash_table_iter_next (&iter, &key, &value))
	{
	  gpointer key1, value1;
	  if (!g_hash_table_lookup_extended (permutations[i].to->levels, key,
					     &key1, &value1)
	      || key != key1
	      || memcmp (value, value1, sizeof (struct smie_level_t)) != 0)
	    return FALSE;
	}

      /* FIXME: Check opener_closer.  */
    }
  return TRUE;
}

static gboolean
smie_test_forward_char (gpointer data)
{
  struct smie_test_context_t *context = data;
  if (context->input[context->offset] != '\0')
    {
      context->offset++;
      return TRUE;
    }
  return FALSE;
}

static gboolean
smie_test_backward_char (gpointer data)
{
  struct smie_test_context_t *context = data;
  if (context->offset > 0)
    {
      context->offset--;
      return TRUE;
    }
  return FALSE;
}

static gboolean
smie_test_forward_to_line_end (gpointer data)
{
  struct smie_test_context_t *context = data;
  goffset offset = context->offset;
  while (context->input[context->offset] != '\0'
	 && context->input[context->offset] != '\n')
    context->offset++;
  return offset != context->offset;
}

static gboolean
smie_test_backward_to_line_start (gpointer data)
{
  struct smie_test_context_t *context = data;
  goffset offset = context->offset;
  while (context->offset > 0
	 && context->input[context->offset - 1] != '\n')
    context->offset--;
  return offset != context->offset;
}

static gboolean
smie_test_forward_line (gpointer data)
{
  struct smie_test_context_t *context = data;
  goffset offset = context->offset;
  smie_test_forward_to_line_end (data);
  if (context->input[context->offset] != '\0'
      && context->input[context->offset] == '\n')
    context->offset++;
  return offset != context->offset;
}

static gboolean
smie_test_backward_line (gpointer data)
{
  struct smie_test_context_t *context = data;
  goffset offset = context->offset;
  smie_test_backward_to_line_start (data);
  if (context->offset > 0 && context->input[context->offset - 1] == '\n')
    context->offset--;
  return offset != context->offset;
}

static gboolean
smie_test_forward_token (gpointer data, gboolean move_lines)
{
  struct smie_test_context_t *context = data;
  goffset offset = context->offset;
  while (context->input[context->offset] != '\0'
	 && (move_lines || context->input[context->offset] != '\n')
	 && !g_ascii_isspace (context->input[context->offset]))
    context->offset++;
  while (context->input[context->offset] != '\0'
	 && (move_lines || context->input[context->offset] != '\n')
	 && g_ascii_isspace (context->input[context->offset]))
    context->offset++;
  return offset != context->offset;
}

static gboolean
smie_test_backward_token (gpointer data, gboolean move_lines)
{
  struct smie_test_context_t *context = data;
  goffset offset = context->offset;
  while (context->offset > 0
	 && (move_lines || context->input[context->offset] != '\n')
	 && !g_ascii_isspace (context->input[context->offset]))
    context->offset--;
  while (context->offset > 0
	 && (move_lines || context->input[context->offset] != '\n')
	 && g_ascii_isspace (context->input[context->offset]))
    context->offset--;
  return offset != context->offset;
}

static gboolean
smie_test_starts_line (gpointer data)
{
  struct smie_test_context_t *context = data;
  return context->offset == 0
    || context->input[context->offset - 1] == '\n';
}

static gboolean
smie_test_ends_line (gpointer data)
{
  struct smie_test_context_t *context = data;
  return context->input[context->offset] == '\0'
    || context->input[context->offset] == '\n';
}

static gboolean
smie_test_is_start (gpointer data)
{
  struct smie_test_context_t *context = data;
  return context->offset == 0;
}

static gboolean
smie_test_is_end (gpointer data)
{
  struct smie_test_context_t *context = data;
  return context->input[context->offset] == '\0';
}

static gboolean
smie_test_read_token (gpointer data, gchar **token)
{
  struct smie_test_context_t *context = data;
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

static gunichar
smie_test_read_char (gpointer user_data)
{
  struct smie_test_context_t *context = user_data;
  if (context->offset < 0
      || context->input[context->offset] == '\0')
    return (gunichar) -1;
  return g_utf8_get_char_validated (&context->input[context->offset], -1);
}

static void
smie_test_push_context (gpointer data)
{
  struct smie_test_context_t *context = data;
  context->saved_offset = context->offset;
}

static void
smie_test_pop_context (gpointer data)
{
  struct smie_test_context_t *context = data;
  context->offset = context->saved_offset;
}

smie_cursor_functions_t smie_test_cursor_functions =
  {
    smie_test_forward_char,
    smie_test_backward_char,
    smie_test_forward_line,
    smie_test_backward_line,
    smie_test_forward_to_line_end,
    smie_test_backward_to_line_start,
    smie_test_forward_token,
    smie_test_backward_token,
    smie_test_is_start,
    smie_test_is_end,
    smie_test_starts_line,
    smie_test_ends_line,
    smie_test_read_token,
    smie_test_read_char,
    smie_test_push_context,
    smie_test_pop_context
  };
