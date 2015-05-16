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

#include <smie/smie-grammar.h>

G_BEGIN_DECLS

/**
 * smie_indenter_t:
 *
 * An indenter.
 */
typedef struct _smie_indenter_t smie_indenter_t;
typedef struct _smie_cursor_functions_t smie_cursor_functions_t;

/**
 * smie_cursor_functions_t:
 * @forward_char: Move the cursor forward by a character, and return
 *   %TRUE if the cursor moves, otherwise %FALSE.
 * @backward_char: Move the cursor backward by a character, and return
 *   %TRUE if the cursor moves, otherwise %FALSE.
 * @forward_line: Move the cursor forward by a line, and return %TRUE
 *   if the cursor moves, otherwise %FALSE.
 * @backward_line: Move the cursor forward by a line, and return %TRUE
 *   if the cursor moves, otherwise %FALSE.
 * @forward_to_line_end: Move the cursor to the end of line.  If the
 *   line ends with a newline character, place the cursor on that
 *   character.  Return %TRUE if the cursor moves, otherwise %FALSE.
 * @backward_to_line_start: Move the cursor to the beginning of line,
 *   and return %TRUE if the cursor moves, otherwise %FALSE.
 * @forward_comment: Move the cursor forward by skipping comments and
 *   whitespace characters.  Place the cursor on the first non-comment
 *   / non-whitespace character if any.  Otherwise move to the end of
 *   buffer.  Return %TRUE if the cursor moves, otherwise %FALSE.
 * @backward_comment: Move the cursor backward by skipping comments
 *   and whitespace characters.  Place the cursor on the first
 *   non-comment / non-whitespace character, if any.  Otherwise move to
 *   the beginning of buffer.  Return %TRUE if the cursor moves,
 *   otherwise %FALSE.
 * @forward_token: Move the cursor to the start of next token, if any,
 *   and returns the token between the previous cursor position and the
 *   new cursor position.  If the cursor is already on the middle of a
 *   token, return the partial string of the token.
 * @backward_token: Move the cursor to the end of previous token, if any,
 *   and returns the token between the previous cursor position and the
 *   new cursor position.  If the cursor is already on the middle of a
 *   token, return the partial string of the token.
 * @is_start: Return %TRUE if the cursor is at the beginning of
 *   buffer, otherwise %FALSE.
 * @is_end: Return %TRUE if the cursor is at the end of buffer,
 *   otherwise %FALSE.
 * @starts_line: Return %TRUE if the cursor is at the beginning of
 *   line, otherwise %FALSE.
 * @ends_line: Return %TRUE if the cursor is at the end of line,
 *   otherwise %FALSE.
 * @get_offset: Return the integer offset of the current cursor.
 * @get_line_offset: Return the integer offset of the current cursor,
 *   counting from the beginning of line.
 * @get_char: Return the character at the cursor position, if any.
 *   Otherwise return (gunichar) -1.
 * @push_context: Save the current cursor position to a stack.
 * @pop_context: Restore the previous cursor position from a stack.
 *
 * Set of callback functions used by the indenter.  All those
 * functions take a context object passed to smie_indenter_calculate().
 */
struct _smie_cursor_functions_t
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

typedef struct _smie_rule_functions_t smie_rule_functions_t;

/**
 * smie_rule_functions_t:
 * @before: Return the offset to use to indent TOKEN itself, -1 if
 *   undetermined
 * @after: Return the offset to use for indentation after TOKEN, -1 if
 *   undetermined
 * @arg_elem: Return the offset to use to indent function arguments,
 *   -1 if undetermined
 * @basic: Return the basic indentation step
 * @list_intro: Return %TRUE if TOKEN is followed by a list
 *   expressions, %FALSE otherwise
 * @close_all: Return %TRUE if TOKEN should be aligned with the opener
 *   of the last close-paren token on the same line, if there are
 *   multiple, %FALSE otherwise
 *
 * Set of configuration functions used by the indenter.  All those
 * functions except @basic are optional and can be set to %NULL.
 */
struct _smie_rule_functions_t
{
  gint (* before) (const gchar *token);
  gint (* after) (const gchar *token);
  gint (* arg_elem) (void);
  gint (* basic) (void);
  gboolean (* list_intro) (const gchar *token);
  gboolean (* close_all) (const gchar *token);
};

smie_indenter_t *smie_indenter_new (smie_grammar_t *grammar,
				    const smie_cursor_functions_t *functions,
				    const smie_rule_functions_t *rules);
smie_indenter_t *smie_indenter_ref (smie_indenter_t *indenter);
void smie_indenter_unref (smie_indenter_t *indenter);
gint smie_indenter_calculate (smie_indenter_t *indenter,
			      gpointer context);

G_END_DECLS

#endif	/* __SMIE_INDENTER_H__ */
