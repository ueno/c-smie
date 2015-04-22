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
#ifndef __SMIE_GRAMMAR_H__
#define __SMIE_GRAMMAR_H__ 1

#include <glib.h>

G_BEGIN_DECLS

typedef struct smie_symbol_pool_t smie_symbol_pool_t;
typedef struct smie_symbol_t smie_symbol_t;
typedef struct smie_bnf_grammar_t smie_bnf_grammar_t;
typedef struct smie_prec2_grammar_t smie_prec2_grammar_t;
typedef struct smie_grammar_t smie_grammar_t;

typedef enum smie_symbol_type_t smie_symbol_type_t;
enum smie_symbol_type_t
  {
    SMIE_SYMBOL_TERMINAL,
    SMIE_SYMBOL_TERMINAL_VAR,
    SMIE_SYMBOL_NON_TERMINAL
  };

typedef enum smie_prec2_type_t smie_prec2_type_t;
enum smie_prec2_type_t
  {
    SMIE_PREC2_EQ,
    SMIE_PREC2_LT,
    SMIE_PREC2_GT
  };

smie_symbol_pool_t *smie_symbol_pool_alloc (void);
void smie_symbol_pool_free (smie_symbol_pool_t *pool);
void smie_symbol_pool_unref (smie_symbol_pool_t *pool);

const smie_symbol_t *smie_symbol_intern (smie_symbol_pool_t *pool,
					 const gchar *name,
					 smie_symbol_type_t type);

#define SMIE_ERROR smie_error_quark ()
GQuark smie_error_quark (void);

enum smie_error_code_t
  {
    SMIE_ERROR_GRAMMAR
  };

smie_bnf_grammar_t *smie_bnf_grammar_alloc (smie_symbol_pool_t *pool);
void smie_bnf_grammar_free (smie_bnf_grammar_t *grammar);

gboolean smie_bnf_grammar_load (smie_bnf_grammar_t *grammar,
				const gchar *input,
				GError **error);
gboolean smie_bnf_grammar_add_rule (smie_bnf_grammar_t *grammar,
				    GList *symbols);

smie_prec2_grammar_t *smie_prec2_grammar_alloc (smie_symbol_pool_t *pool);
void smie_prec2_grammar_free (smie_prec2_grammar_t *grammar);
gboolean smie_prec2_grammar_add_rule (smie_prec2_grammar_t *grammar,
				      const smie_symbol_t *a,
				      const smie_symbol_t *b,
				      smie_prec2_type_t type);
gboolean smie_prec2_grammar_add_opener (smie_prec2_grammar_t *grammar,
					const gchar *token);
gboolean smie_prec2_grammar_add_closer (smie_prec2_grammar_t *grammar,
					const gchar *token);

smie_grammar_t *smie_grammar_alloc (smie_symbol_pool_t *pool);
void smie_grammar_free (smie_grammar_t *grammar);
smie_grammar_t *smie_grammar_ref (smie_grammar_t *grammar);
void smie_grammar_unref (smie_grammar_t *grammar);
gboolean smie_grammar_add_rule (smie_grammar_t *grammar,
				const smie_symbol_t *symbol,
				gint left_prec,
				gboolean left_is_parenthesis,
				gint right_prec,
				gboolean right_is_parenthesis);
gboolean smie_grammar_is_opener (smie_grammar_t *grammar,
				 const gchar *token);
gboolean smie_grammar_is_closer (smie_grammar_t *grammar,
				 const gchar *token);

gboolean smie_bnf_to_prec2 (smie_bnf_grammar_t *bnf,
			    smie_prec2_grammar_t *prec2,
			    GError **error);
gboolean smie_prec2_to_precs (smie_prec2_grammar_t *prec2,
			      smie_grammar_t *precs,
			      GError **error);

typedef enum smie_advance_step_t smie_advance_step_t;
enum smie_advance_step_t
  {
    SMIE_ADVANCE_CHARACTERS,
    SMIE_ADVANCE_LINES,
    SMIE_ADVANCE_LINE_ENDS,
    SMIE_ADVANCE_TOKENS
  };

typedef enum smie_inspect_request_t smie_inspect_request_t;
enum smie_inspect_request_t
  {
    SMIE_INSPECT_IS_START,
    SMIE_INSPECT_IS_END,
    SMIE_INSPECT_STARTS_LINE,
    SMIE_INSPECT_ENDS_LINE,
    SMIE_INSPECT_HAS_NEXT,
    SMIE_INSPECT_HAS_PREVIOUS_LINE
  };

typedef gboolean (*smie_advance_function_t) (smie_advance_step_t, gint,
					     gpointer);
typedef gboolean (*smie_inspect_function_t) (smie_inspect_request_t,
					     gpointer);
typedef gboolean (*smie_read_token_function_t) (gchar **, gpointer);
typedef gunichar (*smie_read_char_function_t) (gpointer);

gboolean smie_forward_sexp (smie_grammar_t *grammar,
			    smie_advance_function_t advance_func,
			    smie_read_token_function_t read_func,
			    gpointer context);

gboolean smie_backward_sexp (smie_grammar_t *grammar,
			     smie_advance_function_t advance_func,
			     smie_read_token_function_t read_func,
			     gpointer context);

G_END_DECLS

#endif	/* __SMIE_GRAMMAR_H__ */
