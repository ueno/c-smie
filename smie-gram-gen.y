%{
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

#include "smie-private.h"
#include "smie-gram-gen.h"
static int yylex (YYSTYPE *lval,
		  smie_bnf_grammar_t *grammar,
		  smie_precs_grammar_t *precs,
		  const gchar **input,
		  GError **error);
static void yyerror (smie_bnf_grammar_t *grammar,
		     smie_precs_grammar_t *precs,
		     const gchar **input,
		     GError **error,
		     const char *string);
%}
%param {smie_bnf_grammar_t *grammar} {smie_precs_grammar_t *precs} {const gchar **input} {GError **error}
%define api.pure full

%union {
  const smie_symbol_t *sval;
  smie_prec_type_t pval;
  GList *lval;
  gchar *tval;
}

%token <tval> NONTERMINAL
%token <tval> TERMINAL
%token <tval> TERMINALVAR
%token SEPARATOR
%token <pval> LEFT RIGHT ASSOC NONASSOC
%type <sval> symbol terminal
%type <lval> sentences symbols terminals
%type <pval> prectype

%%

grammar: header SEPARATOR rules
	;

header:	precs
	;

precs:	%empty
	| precs prec
	;

prec:	prectype terminals ';'
	{
	  GList *terminals = $2, *l = terminals;
	  const smie_symbol_t **symbols
	    = g_malloc0_n (g_list_length (terminals) + 1, sizeof (smie_symbol_t *));
	  const smie_symbol_t **p = symbols;
	  for (; l; l = l->next)
	    *p++ = l->data;
	  smie_precs_grammar_add_prec (precs, $1, symbols);
	  g_free (symbols);
	}
	;

prectype: LEFT
	| RIGHT
	| ASSOC
	| NONASSOC
	;

rules:	%empty
	| rules rule
	;

rule:	NONTERMINAL ':' sentences ';'
	{
	  gchar *nonterminal = $1;
	  GList *sentences = $3, *l = sentences;
	  for (; l; l = l->next)
	    {
	      GList *l2 = l->data;
	      const smie_symbol_t *symbol
		= smie_symbol_intern (grammar->pool, nonterminal,
				      SMIE_SYMBOL_NON_TERMINAL);
	      GList *rule = g_list_prepend (l2, (gpointer) symbol);
	      smie_bnf_grammar_add_rule (grammar, rule);
	    }
	  g_free (nonterminal);
	  g_list_free (sentences);
	}
	;

sentences:	symbols
	{
	  $$ = g_list_prepend (NULL, $1);
	}
	| sentences '|' symbols
	{
	  $$ = g_list_prepend ($1, $3);
	}
	;

symbols:	symbol
	{
	  $$ = g_list_append (NULL, (gpointer) $1);
	}
	| symbols symbol
	{
	  $$ = g_list_append ($1, (gpointer) $2);
	}
	;

symbol:	NONTERMINAL
	{
	  $$ = smie_symbol_intern (grammar->pool, $1, SMIE_SYMBOL_NON_TERMINAL);
	  g_free ($1);
	}
	| terminal
	| TERMINALVAR
	{
	  $$ = smie_symbol_intern (grammar->pool, $1, SMIE_SYMBOL_TERMINAL_VARIABLE);
	  g_free ($1);
	}
	;

terminals:	terminal
	{
	  $$ = g_list_append (NULL, (gpointer) $1);
	}
	| terminals terminal
	{
	  $$ = g_list_append ($1, (gpointer) $2);
	}
	;

terminal:	TERMINAL
	{
	  $$ = smie_symbol_intern (grammar->pool, $1, SMIE_SYMBOL_TERMINAL);
	  g_free ($1);
	}
	;

%%

static int
yylex (YYSTYPE *lval, smie_bnf_grammar_t *grammar, smie_precs_grammar_t *precs,
       const gchar **input, GError **error)
{
  const char *cp = *input;

  for (; *cp != '\0'; cp++)
    {
      if (*cp == '#')
	for (; *cp != '\0' && *cp != '\n'; cp++)
	  ;
      if (!g_ascii_isspace (*cp))
	break;
    }

  if (*cp == '\0')
    return YYEOF;

  if (*cp == '%')
    {
      if (*(cp + 1) == '%')
	{
	  *input = cp + 2;
	  return SEPARATOR;
	}
      else if (g_str_has_prefix (cp + 1, "left"))
	{
	  *input = cp + 5;
	  lval->pval = SMIE_PREC_LEFT;
	  return LEFT;
	}
      else if (g_str_has_prefix (cp + 1, "right"))
	{
	  *input = cp + 6;
	  lval->pval = SMIE_PREC_RIGHT;
	  return RIGHT;
	}
      else if (g_str_has_prefix (cp + 1, "assoc"))
	{
	  *input = cp + 6;
	  lval->pval = SMIE_PREC_ASSOC;
	  return ASSOC;
	}
      else if (g_str_has_prefix (cp + 1, "nonassoc"))
	{
	  *input = cp + 9;
	  lval->pval = SMIE_PREC_NON_ASSOC;
	  return NONASSOC;
	}
    }

  if (*cp == '"' || *cp == '\'')
    {
      GString *buffer = g_string_new ("");
      gchar delimiter = *cp;

      cp++;
      for (; *cp != '\0'; cp++)
	{
	  if (*cp == delimiter)
	    {
	      cp++;
	      break;
	    }
	  else if (*cp == '\\')
	    {
	      cp++;
	      if (*cp == '\0')
		break;
	    }
	  g_string_append_c (buffer, *cp);
	}
      lval->tval = g_string_free (buffer, FALSE);
      *input = cp;
      return TERMINAL;
    }
  else if (g_ascii_isalpha (*cp) && g_ascii_isupper (*cp))
    {
      GString *buffer = g_string_new ("");
      for (; *cp != '\0'; cp++)
	{
	  if (!(*cp == '_' || g_ascii_isalnum (*cp)))
	    break;
	  g_string_append_c (buffer, *cp);
	}
      lval->tval = g_string_free (buffer, FALSE);
      *input = cp;
      return TERMINALVAR;
    }
  else if (g_ascii_isalpha (*cp) && g_ascii_islower (*cp))
    {
      GString *buffer = g_string_new ("");
      for (; *cp != '\0'; cp++)
	{
	  if (!(*cp == '_' || g_ascii_isalnum (*cp)))
	    break;
	  g_string_append_c (buffer, *cp);
	}
      lval->tval = g_string_free (buffer, FALSE);
      *input = cp;
      return NONTERMINAL;
    }

  *input = cp + 1;
  return *cp;
}

static void
yyerror (smie_bnf_grammar_t *grammar, smie_precs_grammar_t *precs,
	 const gchar **input, GError **error, const char *string)
{
  g_set_error_literal (error, SMIE_ERROR, SMIE_ERROR_GRAMMAR, string);
}
