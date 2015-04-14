#include "config.h"
#include "smie-grammar.h"
#include <string.h>

static smie_bnf_grammar_t *
populate_bnf_grammar (void)
{
  smie_bnf_grammar_t *grammar = smie_bnf_grammar_alloc ();
  GList *rule;

  /* S -> "#" E "#" */
  rule = g_list_append (NULL, smie_symbol_alloc ("S", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("#", SMIE_SYMBOL_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("E", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("#", SMIE_SYMBOL_TERMINAL));
  smie_bnf_grammar_add_rule (grammar, rule);

  /* E -> E "+" T */
  rule = g_list_append (NULL, smie_symbol_alloc ("E", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("E", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("+", SMIE_SYMBOL_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("T", SMIE_SYMBOL_NON_TERMINAL));
  smie_bnf_grammar_add_rule (grammar, rule);

  /* E -> T */
  rule = g_list_append (NULL, smie_symbol_alloc ("E", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("T", SMIE_SYMBOL_NON_TERMINAL));
  smie_bnf_grammar_add_rule (grammar, rule);

  /* T -> T "x" F */
  rule = g_list_append (NULL, smie_symbol_alloc ("T", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("T", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("x", SMIE_SYMBOL_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("F", SMIE_SYMBOL_NON_TERMINAL));
  smie_bnf_grammar_add_rule (grammar, rule);

  /* T -> F */
  rule = g_list_append (NULL, smie_symbol_alloc ("T", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("F", SMIE_SYMBOL_NON_TERMINAL));
  smie_bnf_grammar_add_rule (grammar, rule);

  /* F -> n */
  rule = g_list_append (NULL, smie_symbol_alloc ("F", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("n", SMIE_SYMBOL_TERMINAL_VAR));
  smie_bnf_grammar_add_rule (grammar, rule);

  /* F -> "(" E ")" */
  rule = g_list_append (NULL, smie_symbol_alloc ("F", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("(", SMIE_SYMBOL_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc ("E", SMIE_SYMBOL_NON_TERMINAL));
  rule = g_list_append (rule, smie_symbol_alloc (")", SMIE_SYMBOL_TERMINAL));
  smie_bnf_grammar_add_rule (grammar, rule);

  return grammar;
}

static gboolean
test_next_token (gchar **tokenp, goffset *offsetp, gpointer user_data)
{
  goffset offset = *offsetp;
  gchar *input = user_data;
  gsize length = strlen (input);
  for (; offset < length; offset++)
    if (!g_ascii_isspace (input[offset]))
      break;
  if (offset < length)
    {
      goffset start_offset = offset;
      for (; offset < length; offset++)
	if (g_ascii_isspace (input[offset]))
	  break;
      *tokenp = g_strndup (&input[start_offset], offset - start_offset);
      *offsetp = offset;
      return TRUE;
    }
  return FALSE;
}

static gboolean
test_previous_token (gchar **tokenp, goffset *offsetp, gpointer user_data)
{
  goffset offset = *offsetp;
  gchar *input = user_data;
  gsize length = strlen (input);
  for (; offset >= 0; offset--)
    if (!g_ascii_isspace (input[offset]))
      break;
  if (offset >= 0)
    {
      goffset end_offset = offset;
      for (; offset >= 0; offset--)
	if (g_ascii_isspace (input[offset]))
	  break;
      *tokenp = g_strndup (&input[offset + 1], end_offset - offset);
      *offsetp = offset;
      return TRUE;
    }
  return FALSE;
}

int
main (void)
{
  // smie_bnf_grammar_t *bnf = populate_bnf_grammar ();
  smie_prec2_grammar_t *prec2 = smie_prec2_grammar_alloc ();
  smie_precs_grammar_t *precs = smie_precs_grammar_alloc ();
  goffset offset;

  smie_bnf_grammar_t *bnf2;
  bnf2 = smie_bnf_grammar_alloc_from_string ("\
s: \"#\" e \"#\";\n\
e: e \"+\" t | t;\n\
t: t \"x\" f | f;\n\
f: N | \"(\" e \")\";");
  smie_bnf_to_prec2 (bnf2, prec2);
  smie_debug_dump_prec2_grammar (prec2);
  smie_prec2_to_precs (prec2, precs);
  smie_debug_dump_precs_grammar (precs);

#if 0
  offset = 0;
  smie_forward_sexp (precs, &offset, test_next_token, "# 4 + 5 x 6 + 8 #");
  g_printf ("%d\n", offset);
#endif

  offset = 1;
  smie_forward_sexp (precs, &offset, test_next_token, "# ( 4 + ( 5 x 6 ) + 7 ) + 8 #");
  g_printf ("%d\n", offset);

  offset = 23;
  smie_backward_sexp (precs, &offset, test_previous_token, "# ( 4 + ( 5 x 6 ) + 7 ) + 8 #");
  g_printf ("%d\n", offset);

  smie_precs_grammar_free (precs);
  return 0;
}
