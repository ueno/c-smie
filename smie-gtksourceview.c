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

#include "smie-gtksourceview.h"

#include <gtksourceview/gtksource.h>

/**
 * SECTION:smie-gtksourceview
 * @short_description: adapter to GtkSourceView
 * @title: GtkSourceView adapter
 * @section_id:
 * @stability: Unstable
 * @include: smie-gtksourceview.h
 *
 * The GtkSourceView adapter provides a basic #smie_cursor_functions_t
 * implementation defined using #GtkTextIter and #GtkSourceBuffer.
 */

static gboolean
smie_gtk_source_buffer_forward_char (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  return gtk_text_iter_forward_char (&context->iter);
}

static gboolean
smie_gtk_source_buffer_backward_char (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  return gtk_text_iter_backward_char (&context->iter);
}

static gboolean
smie_gtk_source_buffer_forward_to_line_end (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  /* If we are already on the EOL, do nothing.  */
  if (gtk_text_iter_ends_line (&context->iter))
    return FALSE;
  return gtk_text_iter_forward_to_line_end (&context->iter);
}

static gboolean
smie_gtk_source_buffer_backward_to_line_start (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  GtkTextIter start_iter;
  gtk_text_iter_assign (&start_iter, &context->iter);
  while (!gtk_text_iter_starts_line (&context->iter))
    gtk_text_iter_backward_char (&context->iter);
  return !gtk_text_iter_equal (&context->iter, &start_iter);
}

static gboolean
smie_gtk_source_buffer_forward_line (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  return gtk_text_iter_forward_line (&context->iter);
}

static gboolean
smie_gtk_source_buffer_backward_line (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  return gtk_text_iter_backward_line (&context->iter);
}

static gboolean
smie_gtk_source_buffer_forward_comment (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  GtkTextIter start_iter;

  gtk_text_iter_assign (&start_iter, &context->iter);
  while (!gtk_text_iter_is_end (&context->iter)
	 && (gtk_source_buffer_iter_has_context_class (context->buffer,
						       &context->iter,
						       "comment")
	     || g_unichar_isspace (gtk_text_iter_get_char (&context->iter)))
	 && gtk_text_iter_forward_char (&context->iter))
    ;
  return !gtk_text_iter_equal (&context->iter, &start_iter);
}

static gboolean
smie_gtk_source_buffer_backward_comment (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  GtkTextIter end_iter;

  gtk_text_iter_assign (&end_iter, &context->iter);
  while (!gtk_text_iter_is_start (&context->iter)
	 && (gtk_source_buffer_iter_has_context_class (context->buffer,
						       &context->iter,
						       "comment")
	     || g_unichar_isspace (gtk_text_iter_get_char (&context->iter)))
	 && gtk_text_iter_backward_char (&context->iter))
    ;
  return !gtk_text_iter_equal (&context->iter, &end_iter);
}

static gchar *
smie_gtk_source_buffer_forward_token (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  GtkTextIter iter;

  /* Skip comments and whitespaces.  */
  while (!gtk_text_iter_is_end (&context->iter)
	 && (gtk_source_buffer_iter_has_context_class (context->buffer,
						       &context->iter,
						       "comment")
	     || g_unichar_isspace (gtk_text_iter_get_char (&context->iter))))
    gtk_text_iter_forward_char (&context->iter);

  if (gtk_text_iter_is_end (&context->iter))
    return NULL;

  gtk_text_iter_assign (&iter, &context->iter);
  if (gtk_source_buffer_iter_has_context_class (context->buffer,
						&context->iter,
						"string"))
    {
      /* Read a string literal.  */
      while (!gtk_text_iter_is_end (&context->iter)
	     && gtk_source_buffer_iter_has_context_class (context->buffer,
							  &context->iter,
							  "string"))
	gtk_text_iter_forward_char (&context->iter);
    }
  else if (g_unichar_ispunct (gtk_text_iter_get_char (&context->iter)))
    {
      /* Read a punctuation.  */
      while (!gtk_text_iter_is_end (&context->iter)
	     && g_unichar_ispunct (gtk_text_iter_get_char (&context->iter)))
	gtk_text_iter_forward_char (&context->iter);
    }
  else
    {
      /* Read a normal token.  */
      while (!gtk_text_iter_is_end (&context->iter)
	     && !(gtk_source_buffer_iter_has_context_class (context->buffer,
							    &context->iter,
							    "comment")
		  || gtk_source_buffer_iter_has_context_class (context->buffer,
							       &context->iter,
							       "string")
		  || g_unichar_ispunct (gtk_text_iter_get_char (&context->iter))
		  || g_unichar_isspace (gtk_text_iter_get_char (&context->iter))))
	gtk_text_iter_forward_char (&context->iter);
    }

  return gtk_text_iter_get_slice (&iter, &context->iter);
}

static gchar *
smie_gtk_source_buffer_backward_token (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  GtkTextIter iter, start_iter;

  if (gtk_text_iter_is_start (&context->iter))
    return NULL;

  /* Skip comments and whitespaces.  */
  gtk_text_iter_backward_char (&context->iter);
  while (!gtk_text_iter_is_start (&context->iter)
	 && (gtk_source_buffer_iter_has_context_class (context->buffer,
						       &context->iter,
						       "comment")
	     || g_unichar_isspace (gtk_text_iter_get_char (&context->iter))))
    gtk_text_iter_backward_char (&context->iter);

  gtk_text_iter_assign (&iter, &context->iter);
  if (gtk_source_buffer_iter_has_context_class (context->buffer,
						&context->iter,
						"string"))
    {
      /* Read a string literal.  */
      while (!gtk_text_iter_is_start (&context->iter)
	     && gtk_source_buffer_iter_has_context_class (context->buffer,
							  &context->iter,
							  "string"))
	gtk_text_iter_backward_char (&context->iter);
    }
  else if (g_unichar_ispunct (gtk_text_iter_get_char (&context->iter)))
    {
      /* Read a punctuation.  */
      while (!gtk_text_iter_is_start (&context->iter)
	     && g_unichar_ispunct (gtk_text_iter_get_char (&context->iter)))
	gtk_text_iter_backward_char (&context->iter);
    }
  else
    {
      /* Read a normal token.  */
      while (!gtk_text_iter_is_start (&context->iter)
	     && !(gtk_source_buffer_iter_has_context_class (context->buffer,
							    &context->iter,
							    "comment")
		  || gtk_source_buffer_iter_has_context_class (context->buffer,
							       &context->iter,
							       "string")
		  || g_unichar_ispunct (gtk_text_iter_get_char (&context->iter))
		  || g_unichar_isspace (gtk_text_iter_get_char (&context->iter))))
	gtk_text_iter_backward_char (&context->iter);
    }

  gtk_text_iter_assign (&start_iter, &context->iter);
  if (!gtk_text_iter_is_start (&start_iter))
    gtk_text_iter_forward_char (&start_iter);
  gtk_text_iter_forward_char (&iter);
  return gtk_text_iter_get_slice (&start_iter, &iter);
}

static gboolean
smie_gtk_source_buffer_is_start (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  return gtk_text_iter_is_start (&context->iter);
}

static gboolean
smie_gtk_source_buffer_is_end (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  return gtk_text_iter_is_end (&context->iter);
}

static gboolean
smie_gtk_source_buffer_starts_line (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  return gtk_text_iter_starts_line (&context->iter);
}

static gboolean
smie_gtk_source_buffer_ends_line (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  return gtk_text_iter_ends_line (&context->iter);
}

static gint
smie_gtk_source_buffer_get_offset (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  return gtk_text_iter_get_offset (&context->iter);
}

static gint
smie_gtk_source_buffer_get_line_offset (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  return gtk_text_iter_get_line_offset (&context->iter);
}

static gunichar
smie_gtk_source_buffer_get_char (gpointer user_data)
{
  struct smie_gtk_source_buffer_context_t *context = user_data;
  return gtk_text_iter_get_char (&context->iter);
}

static void
smie_gtk_source_buffer_push_context (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  context->stack = g_list_prepend (context->stack,
				   gtk_text_iter_copy (&context->iter));
}

static void
smie_gtk_source_buffer_pop_context (gpointer data)
{
  struct smie_gtk_source_buffer_context_t *context = data;
  g_return_if_fail (context->stack);
  gtk_text_iter_assign (&context->iter, context->stack->data);
  gtk_text_iter_free (context->stack->data);
  context->stack = g_list_delete_link (context->stack, context->stack);
}

smie_cursor_functions_t smie_gtk_source_buffer_cursor_functions =
  {
    smie_gtk_source_buffer_forward_char,
    smie_gtk_source_buffer_backward_char,
    smie_gtk_source_buffer_forward_line,
    smie_gtk_source_buffer_backward_line,
    smie_gtk_source_buffer_forward_to_line_end,
    smie_gtk_source_buffer_backward_to_line_start,
    smie_gtk_source_buffer_forward_comment,
    smie_gtk_source_buffer_backward_comment,
    smie_gtk_source_buffer_forward_token,
    smie_gtk_source_buffer_backward_token,
    smie_gtk_source_buffer_is_start,
    smie_gtk_source_buffer_is_end,
    smie_gtk_source_buffer_starts_line,
    smie_gtk_source_buffer_ends_line,
    smie_gtk_source_buffer_get_offset,
    smie_gtk_source_buffer_get_line_offset,
    smie_gtk_source_buffer_get_char,
    smie_gtk_source_buffer_push_context,
    smie_gtk_source_buffer_pop_context
  };
