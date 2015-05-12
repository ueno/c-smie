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
#ifndef __SMIE_INDENTER_H__
#define __SMIE_INDENTER_H__ 1

#include <smie-grammar.h>

G_BEGIN_DECLS

typedef struct smie_indenter_t smie_indenter_t;

typedef gunichar (*smie_get_char_function_t) (gpointer);

typedef struct smie_cursor_functions_t smie_cursor_functions_t;
struct smie_cursor_functions_t
{
  gboolean (* forward_char) (gpointer);
  gboolean (* backward_char) (gpointer);
  gboolean (* forward_line) (gpointer);
  gboolean (* backward_line) (gpointer);
  gboolean (* forward_to_line_end) (gpointer);
  gboolean (* backward_to_line_start) (gpointer);
  gboolean (* forward_comment) (gpointer);
  gboolean (* backward_comment) (gpointer);
  gchar * (* forward_token) (gpointer);
  gchar * (* backward_token) (gpointer);
  gboolean (* is_start) (gpointer);
  gboolean (* is_end) (gpointer);
  gboolean (* starts_line) (gpointer);
  gboolean (* ends_line) (gpointer);
  gint (* get_offset) (gpointer);
  gint (* get_line_offset) (gpointer);
  gunichar (* get_char) (gpointer);
  void (* push_context) (gpointer);
  void (* pop_context) (gpointer);
};

smie_indenter_t *smie_indenter_new (smie_grammar_t *grammar,
				    gint step,
				    smie_cursor_functions_t *functions);
smie_indenter_t *smie_indenter_ref (smie_indenter_t *indenter);
void smie_indenter_unref (smie_indenter_t *indenter);
gint smie_indenter_calculate (smie_indenter_t *indenter,
			      gpointer context);

G_END_DECLS

#endif	/* __SMIE_INDENTER_H__ */
