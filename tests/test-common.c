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

#include "tests/test-common.h"

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
test_common_bnf_grammar_equal (smie_bnf_grammar_t *a,
			     smie_bnf_grammar_t *b)
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
test_common_prec2_grammar_equal (smie_prec2_grammar_t *a,
			       smie_prec2_grammar_t *b)
{
  struct { smie_prec2_grammar_t *from, *to; } permutations[2];
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
      g_hash_table_iter_init (&iter, permutations[i].from->pairs);
      while (g_hash_table_iter_next (&iter, &key, &value))
	if (!g_hash_table_contains (permutations[i].to->pairs, key))
	  return FALSE;
    }
  return TRUE;
}

gboolean
test_common_grammar_equal (smie_grammar_t *a,
			 smie_grammar_t *b)
{
  struct { smie_grammar_t *from, *to; } permutations[2];
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

      /* FIXME: Check pairs.  */
    }
  return TRUE;
}

static gboolean
test_common_forward_char (gpointer data)
{
  struct test_common_context_t *context = data;
  if (context->input[context->offset] != '\0')
    {
      context->offset++;
      return TRUE;
    }
  return FALSE;
}

static gboolean
test_common_backward_char (gpointer data)
{
  struct test_common_context_t *context = data;
  if (context->offset > 0)
    {
      context->offset--;
      return TRUE;
    }
  return FALSE;
}

static gboolean
test_common_forward_to_line_end (gpointer data)
{
  struct test_common_context_t *context = data;
  goffset offset = context->offset;
  while (context->input[context->offset] != '\0'
	 && context->input[context->offset] != '\n')
    context->offset++;
  return offset != context->offset;
}

static gboolean
test_common_backward_to_line_start (gpointer data)
{
  struct test_common_context_t *context = data;
  goffset offset = context->offset;
  while (context->offset > 0
	 && context->input[context->offset - 1] != '\n')
    context->offset--;
  return offset != context->offset;
}

static gboolean
test_common_forward_line (gpointer data)
{
  struct test_common_context_t *context = data;
  goffset offset = context->offset;
  test_common_forward_to_line_end (data);
  if (context->input[context->offset] != '\0'
      && context->input[context->offset] == '\n')
    context->offset++;
  return offset != context->offset;
}

static gboolean
test_common_backward_line (gpointer data)
{
  struct test_common_context_t *context = data;
  goffset offset = context->offset;
  test_common_backward_to_line_start (data);
  if (context->offset > 0 && context->input[context->offset - 1] == '\n')
    context->offset--;
  return offset != context->offset;
}

static gboolean
test_common_forward_comment (gpointer data)
{
  struct test_common_context_t *context = data;
  goffset offset = context->offset;
  while (context->input[context->offset] != '\0'
	 && context->input[context->offset] != '\n'
	 && g_ascii_isspace (context->input[context->offset]))
    context->offset++;
  return offset != context->offset;
}

static gboolean
test_common_backward_comment (gpointer data)
{
  struct test_common_context_t *context = data;
  goffset offset = context->offset;
  while (context->offset > 0
	 && context->input[context->offset] != '\n'
	 && g_ascii_isspace (context->input[context->offset]))
    context->offset--;
  return offset != context->offset;
}

static gchar *
test_common_forward_token (gpointer data)
{
  struct test_common_context_t *context = data;
  goffset offset;
  while (context->input[context->offset] != '\0'
	 && g_ascii_isspace (context->input[context->offset]))
    context->offset++;
  if (context->input[context->offset] == '\0')
    return NULL;
  offset = context->offset;
  while (context->input[context->offset] != '\0'
	 && !g_ascii_isspace (context->input[context->offset]))
    context->offset++;
  return g_strndup (&context->input[offset], context->offset - offset);
}

static gchar *
test_common_backward_token (gpointer data)
{
  struct test_common_context_t *context = data;
  goffset offset;
  while (context->offset > 0
	 && g_ascii_isspace (context->input[context->offset]))
    context->offset--;
  if (context->offset == 0)
    return NULL;
  offset = context->offset;
  while (context->offset > 0
	 && !g_ascii_isspace (context->input[context->offset]))
    context->offset--;
  return g_strndup (&context->input[context->offset + !!context->offset],
		    offset - context->offset + !context->offset);
}

static gboolean
test_common_starts_line (gpointer data)
{
  struct test_common_context_t *context = data;
  return context->offset == 0
    || context->input[context->offset - 1] == '\n';
}

static gboolean
test_common_ends_line (gpointer data)
{
  struct test_common_context_t *context = data;
  return context->input[context->offset] == '\0'
    || context->input[context->offset] == '\n';
}

static gboolean
test_common_is_start (gpointer data)
{
  struct test_common_context_t *context = data;
  return context->offset == 0;
}

static gboolean
test_common_is_end (gpointer data)
{
  struct test_common_context_t *context = data;
  return context->input[context->offset] == '\0';
}

static gint
test_common_get_offset (gpointer data)
{
  struct test_common_context_t *context = data;
  return context->offset;
}

static gint
test_common_get_line_offset (gpointer data)
{
  struct test_common_context_t *context = data;
  goffset offset = context->offset;
  while (offset - 1 >= 0 && context->input[offset - 1] != '\n')
    offset--;
  return context->offset - offset;
}

static gunichar
test_common_get_char (gpointer user_data)
{
  struct test_common_context_t *context = user_data;
  if (context->offset < 0
      || context->input[context->offset] == '\0')
    return (gunichar) -1;
  return g_utf8_get_char_validated (&context->input[context->offset], -1);
}

static void
test_common_push_context (gpointer data)
{
  struct test_common_context_t *context = data;
  context->stack = g_list_prepend (context->stack,
				   GINT_TO_POINTER (context->offset));
}

static void
test_common_pop_context (gpointer data)
{
  struct test_common_context_t *context = data;
  g_return_if_fail (context->stack);
  context->offset = GPOINTER_TO_INT (context->stack->data);
  context->stack = g_list_delete_link (context->stack, context->stack);
}

smie_cursor_functions_t test_common_cursor_functions =
  {
    test_common_forward_char,
    test_common_backward_char,
    test_common_forward_line,
    test_common_backward_line,
    test_common_forward_to_line_end,
    test_common_backward_to_line_start,
    test_common_forward_comment,
    test_common_backward_comment,
    test_common_forward_token,
    test_common_backward_token,
    test_common_is_start,
    test_common_is_end,
    test_common_starts_line,
    test_common_ends_line,
    test_common_get_offset,
    test_common_get_line_offset,
    test_common_get_char,
    test_common_push_context,
    test_common_pop_context
  };
