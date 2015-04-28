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
typedef struct smie_precs_grammar_t smie_precs_grammar_t;
typedef struct smie_grammar_t smie_grammar_t;

typedef enum smie_symbol_type_t smie_symbol_type_t;
enum smie_symbol_type_t
  {
    SMIE_SYMBOL_TERMINAL,
    SMIE_SYMBOL_TERMINAL_VARIABLE,
    SMIE_SYMBOL_NON_TERMINAL
  };

typedef enum smie_prec2_type_t smie_prec2_type_t;
enum smie_prec2_type_t
  {
    SMIE_PREC2_EQ,
    SMIE_PREC2_LT,
    SMIE_PREC2_GT
  };

typedef enum smie_prec_type_t smie_prec_type_t;
enum smie_prec_type_t
  {
    SMIE_PREC_LEFT,
    SMIE_PREC_RIGHT,
    SMIE_PREC_ASSOC,
    SMIE_PREC_NON_ASSOC
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
void smie_bnf_grammar_free (smie_bnf_grammar_t *bnf);

gboolean smie_bnf_grammar_load (smie_bnf_grammar_t *bnf,
				const gchar *input,
				GError **error);
gboolean smie_bnf_grammar_add_rule (smie_bnf_grammar_t *bnf,
				    GList *symbols);

smie_precs_grammar_t *smie_precs_grammar_alloc (smie_symbol_pool_t *pool);
void smie_precs_grammar_free (smie_precs_grammar_t *precs);
void smie_precs_grammar_add_prec (struct smie_precs_grammar_t *precs,
				  smie_prec_type_t type,
				  const smie_symbol_t **symbols);

smie_prec2_grammar_t *smie_prec2_grammar_alloc (smie_symbol_pool_t *pool);
void smie_prec2_grammar_free (smie_prec2_grammar_t *prec2);
gboolean smie_prec2_grammar_add_rule (smie_prec2_grammar_t *prec2,
				      const smie_symbol_t *left,
				      const smie_symbol_t *right,
				      smie_prec2_type_t type,
				      smie_prec2_grammar_t *override);
gboolean smie_prec2_grammar_add_pair (smie_prec2_grammar_t *prec2,
				      const gchar *opener_token,
				      const gchar *closer_token,
				      gboolean is_last);

smie_grammar_t *smie_grammar_alloc (smie_symbol_pool_t *pool);
void smie_grammar_free (smie_grammar_t *grammar);
smie_grammar_t *smie_grammar_ref (smie_grammar_t *grammar);
void smie_grammar_unref (smie_grammar_t *grammar);
gboolean smie_grammar_add_rule (smie_grammar_t *grammar,
				const smie_symbol_t *symbol,
				gint left_prec,
				gboolean left_is_first,
				gint right_prec,
				gboolean right_is_last,
				gboolean right_is_closer);
gboolean smie_grammar_is_first (smie_grammar_t *grammar,
				const gchar *token);
gboolean smie_grammar_is_last (smie_grammar_t *grammar,
			       const gchar *token);
gboolean smie_grammar_is_closer (smie_grammar_t *grammar,
				 const gchar *token);
gboolean smie_grammar_is_pair (smie_grammar_t *grammar,
			       const gchar *opener_token,
			       const gchar *closer_token);

gboolean smie_bnf_to_prec2 (smie_bnf_grammar_t *bnf,
			    smie_prec2_grammar_t *prec2,
			    GList *resolvers,
			    GError **error);
gboolean smie_prec2_to_grammar (smie_prec2_grammar_t *prec2,
				smie_grammar_t *grammar,
				GError **error);

typedef gboolean (*smie_next_token_function_t) (gpointer, gboolean);
typedef gboolean (*smie_read_token_function_t) (gpointer, gchar **);

gboolean smie_forward_sexp (smie_grammar_t *grammar,
			    smie_next_token_function_t next_token_func,
			    smie_read_token_function_t read_token_func,
			    gpointer context);

gboolean smie_backward_sexp (smie_grammar_t *grammar,
			     smie_next_token_function_t next_token_func,
			     smie_read_token_function_t read_token_func,
			     gpointer context);

G_END_DECLS

#endif	/* __SMIE_GRAMMAR_H__ */
