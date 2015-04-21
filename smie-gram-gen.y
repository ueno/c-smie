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

#include "smie-grammar.h"
#include "smie-gram-gen.h"
static int yylex (YYSTYPE *lval,
		  smie_symbol_pool_t *pool,
		  smie_bnf_grammar_t *grammar,
		  const gchar **input,
		  GError **error);
static void yyerror (smie_symbol_pool_t *pool,
		     smie_bnf_grammar_t *grammar,
		     const gchar **input,
		     GError **error,
		     const char *string);
%}
%param {smie_symbol_pool_t *pool} {smie_bnf_grammar_t *grammar} {const gchar **input} {GError **error}
%define api.pure full

%union {
  const smie_symbol_t *sval;
  GList *lval;
  gchar *tval;
}

%token <tval> NONTERMINAL
%token <tval> TERMINAL
%token <tval> TERMINALVAR
%type <sval> symbol
%type <lval> sentences symbols

%%

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
		= smie_symbol_intern (pool, nonterminal,
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
	  $$ = smie_symbol_intern (pool, $1, SMIE_SYMBOL_NON_TERMINAL);
	  g_free ($1);
	}
	| TERMINAL
	{
	  $$ = smie_symbol_intern (pool, $1, SMIE_SYMBOL_TERMINAL);
	  g_free ($1);
	}
	| TERMINALVAR
	{
	  $$ = smie_symbol_intern (pool, $1, SMIE_SYMBOL_TERMINAL_VAR);
	  g_free ($1);
	}
	;

%%

static int
yylex (YYSTYPE *lval, smie_symbol_pool_t *pool, smie_bnf_grammar_t *grammar,
       const gchar **input, GError **error)
{
  const char *cp = *input;

  for (; *cp != '\0'; cp++)
    if (!g_ascii_isspace (*cp))
	break;

  if (*cp == '\0')
    return YYEOF;

  if (*cp == '#')
    for (; *cp != '\0' && *cp != '\n'; cp++)
      break;

  if (*cp == '"')
    {
      GString *buffer = g_string_new ("");
      cp++;
      for (; *cp != '\0'; cp++)
	{
	  switch (*cp)
	    {
	    case '"':
	      cp++;
	      break;
	    case '\\':
	      cp++;
	      if (*cp == '\0')
		break;
	      /* FALLTHROUGH */
	    default:
	      g_string_append_c (buffer, *cp);
	      continue;
	    }
	  break;
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
yyerror (smie_symbol_pool_t *pool, smie_bnf_grammar_t *grammar,
	 const gchar **input, GError **error, const char *string)
{
  g_set_error_literal (error, SMIE_ERROR, SMIE_ERROR_GRAMMAR, string);
}
