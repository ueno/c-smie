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

static gboolean
smie_gtk_source_buffer_forward_char (GtkTextIter *iter,
				     GtkSourceBuffer *buffer)
{
  return gtk_text_iter_forward_char (iter);
}

static gboolean
smie_gtk_source_buffer_backward_char (GtkTextIter *iter,
				      GtkSourceBuffer *buffer)
{
  return gtk_text_iter_backward_char (iter);
}

static gboolean
smie_gtk_source_buffer_end_of_line (GtkTextIter *iter,
				    GtkSourceBuffer *buffer)
{
  /* If we are already on the EOL, do nothing.  */
  if (gtk_text_iter_ends_line (iter))
    return FALSE;
  return gtk_text_iter_forward_to_line_end (iter);
}

static gboolean
smie_gtk_source_buffer_beginning_of_line (GtkTextIter *iter,
					  GtkSourceBuffer *buffer)
{
  GtkTextIter start_iter;
  gtk_text_iter_assign (&start_iter, iter);
  while (!gtk_text_iter_starts_line (iter)
	 && gtk_text_iter_backward_char (iter))
    ;
  return !gtk_text_iter_equal (iter, &start_iter);
}

static gboolean
smie_gtk_source_buffer_forward_line (GtkTextIter *iter,
				     GtkSourceBuffer *buffer)
{
  return gtk_text_iter_forward_line (iter);
}

static gboolean
smie_gtk_source_buffer_backward_line (GtkTextIter *iter,
				      GtkSourceBuffer *buffer)
{
  return gtk_text_iter_backward_line (iter);
}

static gboolean
smie_gtk_source_buffer_forward_token (GtkTextIter *iter,
				      GtkSourceBuffer *buffer)
{
  /* Empty buffer.  */
  if (gtk_text_iter_is_start (iter) && gtk_text_iter_is_end (iter))
    return FALSE;
  /* If ITER is on a comment or a whitespace, skip them.  */
  else if (gtk_source_buffer_iter_has_context_class (buffer, iter, "comment")
	   || g_unichar_isspace (gtk_text_iter_get_char (iter)))
    {
      GtkTextIter start_iter;
      gtk_text_iter_assign (&start_iter, iter);
      while ((gtk_source_buffer_iter_has_context_class (buffer, iter, "comment")
	      || g_unichar_isspace (gtk_text_iter_get_char (iter)))
	     && gtk_text_iter_forward_char (iter))
	;
      return !gtk_text_iter_equal (iter, &start_iter);
    }
  /* If ITER is on a string, skip it and any following comments and
     whitespaces.  */
  else if (gtk_source_buffer_iter_has_context_class (buffer, iter, "string"))
    {
      while (gtk_source_buffer_iter_has_context_class (buffer, iter, "string")
	     && gtk_text_iter_forward_char (iter))
	;
      while ((gtk_source_buffer_iter_has_context_class (buffer, iter, "comment")
	      || g_unichar_isspace (gtk_text_iter_get_char (iter)))
	     && gtk_text_iter_forward_char (iter))
	;
    }
  /* Otherwise, if ITER is on a normal token.  Skip it and any
     following comments and whitespaces.  */
  else
    {
      while (!(gtk_source_buffer_iter_has_context_class (buffer,
							 iter,
							 "comment")
	       || gtk_source_buffer_iter_has_context_class (buffer,
							    iter,
							    "string")
	       || g_unichar_isspace (gtk_text_iter_get_char (iter)))
	     && gtk_text_iter_forward_char (iter))
	;
      while ((gtk_source_buffer_iter_has_context_class (buffer, iter, "comment")
	      || g_unichar_isspace (gtk_text_iter_get_char (iter)))
	     && gtk_text_iter_forward_char (iter))
	;
    }

  return TRUE;
}

static gboolean
smie_gtk_source_buffer_backward_token (GtkTextIter *iter,
				       GtkSourceBuffer *buffer)
{
  /* Empty buffer.  */
  if (gtk_text_iter_is_start (iter) && gtk_text_iter_is_end (iter))
    return FALSE;
  /* If ITER is on a comment or a whitespace, skip them.  If ITER is
     at the end of buffer, think as if there is a whitespace and ITER
     points to it.  */
  else if (gtk_source_buffer_iter_has_context_class (buffer, iter, "comment")
	   || g_unichar_isspace (gtk_text_iter_get_char (iter))
	   || gtk_text_iter_is_end (iter))
    {
      GtkTextIter end_iter;
      gtk_text_iter_assign (&end_iter, iter);
      while ((gtk_source_buffer_iter_has_context_class (buffer, iter, "comment")
	      || g_unichar_isspace (gtk_text_iter_get_char (iter))
	      || gtk_text_iter_is_end (iter))
	     && gtk_text_iter_backward_char (iter))
	;
      return !gtk_text_iter_equal (iter, &end_iter);
    }
  /* If ITER is on a string, skip it and any preceding comments and
     whitespaces.  */
  else if (gtk_source_buffer_iter_has_context_class (buffer, iter, "string"))
    {
      while (gtk_source_buffer_iter_has_context_class (buffer, iter, "string")
	     && gtk_text_iter_backward_char (iter))
	;
      while ((gtk_source_buffer_iter_has_context_class (buffer, iter, "comment")
	      || g_unichar_isspace (gtk_text_iter_get_char (iter)))
	     && gtk_text_iter_backward_char (iter))
	;
    }
  /* Otherwise, ITER is on a normal token.  Skip it and any preceding
     comments and whitespaces.  */
  else
    {
      while (!(gtk_source_buffer_iter_has_context_class (buffer,
							 iter,
							 "comment")
	       || g_unichar_isspace (gtk_text_iter_get_char (iter))
	       || gtk_source_buffer_iter_has_context_class (buffer,
							    iter,
							    "string"))
	     && gtk_text_iter_backward_char (iter))
	;
      while ((gtk_source_buffer_iter_has_context_class (buffer, iter, "comment")
	      || g_unichar_isspace (gtk_text_iter_get_char (iter)))
	     && gtk_text_iter_backward_char (iter))
	;
    }

  return TRUE;
}

typedef gboolean (*smie_movement_function_t) (GtkTextIter *, GtkSourceBuffer *);

gboolean
smie_gtk_source_buffer_advance_func (smie_advance_step_t step, gint count,
				     gpointer user_data)
{
  struct smie_gtk_source_buffer_context_t *context = user_data;
  smie_movement_function_t forward_func, backward_func;
  GtkTextIter iter;
  gboolean result;
  switch (step)
    {
    case SMIE_ADVANCE_CHARACTERS:
      forward_func = smie_gtk_source_buffer_forward_char;
      backward_func = smie_gtk_source_buffer_backward_char;
      break;

    case SMIE_ADVANCE_LINES:
      forward_func = smie_gtk_source_buffer_forward_line;
      backward_func = smie_gtk_source_buffer_backward_line;
      break;

    case SMIE_ADVANCE_LINE_ENDS:
      forward_func = smie_gtk_source_buffer_end_of_line;
      backward_func = smie_gtk_source_buffer_beginning_of_line;
      break;

    case SMIE_ADVANCE_TOKENS:
      forward_func = smie_gtk_source_buffer_forward_token;
      backward_func = smie_gtk_source_buffer_backward_token;
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  gtk_text_iter_assign (&iter, &context->iter);
  if (count > 0)
    {
      for (; count > 0; count--)
	if (!forward_func (&iter, context->buffer))
	  break;
    }
  else
    {
      for (; count < 0; count++)
	if (!backward_func (&iter, context->buffer))
	  break;
    }
  result = !gtk_text_iter_equal (&iter, &context->iter);
  gtk_text_iter_assign (&context->iter, &iter);
  return result;
}

gboolean
smie_gtk_source_buffer_inspect_func (smie_inspect_request_t request,
				     gpointer user_data)
{
  struct smie_gtk_source_buffer_context_t *context = user_data;
  GtkTextIter iter;
  gtk_text_iter_assign (&iter, &context->iter);
  switch (request)
    {
    case SMIE_INSPECT_HAS_PREVIOUS_LINE:
      smie_gtk_source_buffer_beginning_of_line (&iter, context->buffer);
      return gtk_text_iter_backward_line (&iter);

    default:
      g_return_val_if_reached (FALSE);
    }
}

gboolean
smie_gtk_source_buffer_read_token_func (gchar **token, gpointer user_data)
{
  struct smie_gtk_source_buffer_context_t *context = user_data;
  if (gtk_text_iter_is_start (&context->iter)
      &&  gtk_text_iter_is_end (&context->iter))
    return FALSE;
  else if (gtk_source_buffer_iter_has_context_class (context->buffer,
						     &context->iter,
						     "comment")
	   || g_unichar_isspace (gtk_text_iter_get_char (&context->iter)))
    return FALSE;
  else if (gtk_source_buffer_iter_has_context_class (context->buffer,
						     &context->iter,
						     "string"))
    {
      GtkTextIter start_iter, end_iter;
      gtk_text_iter_assign (&start_iter, &context->iter);
      while (!gtk_text_iter_is_start (&start_iter)
	     && gtk_source_buffer_iter_has_context_class (context->buffer,
							  &start_iter,
							  "string"))
	gtk_text_iter_backward_char (&start_iter);

      if (!gtk_source_buffer_iter_has_context_class (context->buffer,
						     &start_iter,
						     "string"))
	gtk_text_iter_forward_char (&start_iter);

      gtk_text_iter_assign (&end_iter, &context->iter);
      while (!gtk_text_iter_is_end (&end_iter)
	     && !gtk_source_buffer_iter_has_context_class (context->buffer,
							   &end_iter,
							   "string"))
	gtk_text_iter_forward_char (&end_iter);

      if (token)
	*token = gtk_text_iter_get_slice (&start_iter, &end_iter);
      return TRUE;
    }
  else
    {
      GtkTextIter start_iter, end_iter;
      gtk_text_iter_assign (&start_iter, &context->iter);
      while (!gtk_text_iter_is_start (&start_iter)
	     && !g_unichar_isspace (gtk_text_iter_get_char (&start_iter)))
	gtk_text_iter_backward_char (&start_iter);

      if (g_unichar_isspace (gtk_text_iter_get_char (&start_iter)))
	gtk_text_iter_forward_char (&start_iter);

      gtk_text_iter_assign (&end_iter, &context->iter);
      while (!gtk_text_iter_is_end (&end_iter)
	     && !g_unichar_isspace (gtk_text_iter_get_char (&end_iter)))
	gtk_text_iter_forward_char (&end_iter);

      if (token)
	*token = gtk_text_iter_get_slice (&start_iter, &end_iter);
      return TRUE;
    }
}

gunichar
smie_gtk_source_buffer_read_char_func (gpointer user_data)
{
  struct smie_gtk_source_buffer_context_t *context = user_data;
  return gtk_text_iter_get_char (&context->iter);
}

smie_cursor_functions_t smie_gtk_source_buffer_cursor_functions =
  {
    smie_gtk_source_buffer_advance_func,
    smie_gtk_source_buffer_inspect_func,
    smie_gtk_source_buffer_read_token_func,
    smie_gtk_source_buffer_read_char_func
  };
