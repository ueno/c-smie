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
#include "smie-test.h"
#include <glib/gprintf.h>
#include <string.h>

#ifdef DEBUG
extern void
smie_debug_dump_prec2_grammar (smie_prec2_grammar_t *grammar);
extern void
smie_debug_dump_precs_grammar (smie_precs_grammar_t *grammar);
#endif

static smie_bnf_grammar_t *
populate_bnf_grammar (smie_symbol_pool_t *pool)
{
  smie_bnf_grammar_t *grammar = smie_bnf_grammar_alloc ();
  GList *rule;

#define NT(x) \
  smie_symbol_intern (pool, (x), SMIE_SYMBOL_NON_TERMINAL)
#define T(x) \
  smie_symbol_intern (pool, (x), SMIE_SYMBOL_TERMINAL)
#define TV(x) \
  smie_symbol_intern (pool, (x), SMIE_SYMBOL_TERMINAL_VAR)
#define CONS(car,cdr)				\
  g_list_prepend ((cdr), (gpointer) (car))

  rule = CONS (NT ("s"),
	       CONS (T ("#"), CONS (NT ("e"), CONS (T ("#"), NULL))));
  smie_bnf_grammar_add_rule (grammar, rule);

  rule = CONS (NT ("e"),
	       CONS (NT ("e"), CONS (T ("+"), CONS (NT ("t"), NULL))));
  smie_bnf_grammar_add_rule (grammar, rule);

  rule = CONS (NT ("e"),
	       CONS (NT ("t"), NULL));
  smie_bnf_grammar_add_rule (grammar, rule);

  rule = CONS (NT ("t"),
	       CONS (NT ("t"), CONS (T ("x"), CONS (NT ("f"), NULL))));
  smie_bnf_grammar_add_rule (grammar, rule);

  rule = CONS (NT ("t"),
	       CONS (NT ("f"), NULL));
  smie_bnf_grammar_add_rule (grammar, rule);

  rule = CONS (NT ("f"),
	       CONS (TV ("N"), NULL));
  smie_bnf_grammar_add_rule (grammar, rule);

  rule = CONS (NT ("f"),
	       CONS (T ("("), CONS (NT ("e"), CONS (T (")"), NULL))));
  smie_bnf_grammar_add_rule (grammar, rule);

#undef CONS
#undef NT
#undef T
#undef TV

  return grammar;
}

static smie_bnf_grammar_t *
populate_bnf_grammar_from_string (smie_symbol_pool_t *pool)
{
  smie_bnf_grammar_t *grammar;
  GError *error;

  error = NULL;
  grammar = smie_bnf_grammar_from_string (pool, "\
s: \"#\" e \"#\";\n\
e: e \"+\" t | t;\n\
t: t \"x\" f | f;\n\
f: N | \"(\" e \")\";",
				       &error);
  g_assert_no_error (error);
  return grammar;
}

static smie_prec2_grammar_t *
populate_prec2_grammar (smie_symbol_pool_t *pool)
{
  smie_prec2_grammar_t *grammar = smie_prec2_grammar_alloc ();

#define T(x) \
  smie_symbol_intern (pool, (x), SMIE_SYMBOL_TERMINAL)
#define ADD(left,op,right) \
  smie_prec2_grammar_add_rule (grammar, T(left), T(right), SMIE_PREC2_ ## op);

  ADD ("#", EQ, "#");
  ADD ("#", LT, "+");
  ADD ("#", LT, "x");
  ADD ("#", LT, "(");
  ADD ("+", GT, "#");
  ADD ("+", GT, "+");
  ADD ("+", LT, "x");
  ADD ("+", LT, "(");
  ADD ("+", GT, ")");
  ADD ("x", GT, "#");
  ADD ("x", GT, "+");
  ADD ("x", GT, "x");
  ADD ("x", LT, "(");
  ADD ("x", GT, ")");
  ADD ("(", LT, "+");
  ADD ("(", LT, "x");
  ADD ("(", LT, "(");
  ADD ("(", EQ, ")");
  ADD (")", GT, "#");
  ADD (")", GT, "+");
  ADD (")", GT, "x");
  ADD (")", GT, ")");

  smie_prec2_grammar_add_opener (grammar, T ("("));
  smie_prec2_grammar_add_closer (grammar, T (")"));

#undef ADD
#undef T

  return grammar;
}

static smie_precs_grammar_t *
populate_precs_grammar (smie_symbol_pool_t *pool)
{
  smie_precs_grammar_t *grammar = smie_precs_grammar_alloc ();

#define T(x) \
  smie_symbol_intern (pool, (x), SMIE_SYMBOL_TERMINAL)

  smie_precs_grammar_add_rule (grammar, T ("#"), 0, FALSE, 0, FALSE);
  smie_precs_grammar_add_rule (grammar, T ("("), 0, TRUE, 5, FALSE);
  smie_precs_grammar_add_rule (grammar, T ("+"), 2, FALSE, 1, FALSE);
  smie_precs_grammar_add_rule (grammar, T ("x"), 4, FALSE, 3, FALSE);
  smie_precs_grammar_add_rule (grammar, T (")"), 5, FALSE, 0, TRUE);

#undef T

  return grammar;
}

struct fixture
{
  smie_symbol_pool_t *pool;
  smie_bnf_grammar_t *bnf;
  smie_prec2_grammar_t *prec2;
  smie_precs_grammar_t *precs;
};

static void
setup_bnf (struct fixture *fixture, gconstpointer user_data)
{
  fixture->pool = smie_symbol_pool_alloc ();
}

static void
teardown_bnf (struct fixture *fixture, gconstpointer user_data)
{
  smie_symbol_pool_free (fixture->pool);
}

static void
test_construct_bnf (struct fixture *fixture, gconstpointer user_data)
{
  smie_bnf_grammar_t *expected, *actual;

  expected = populate_bnf_grammar (fixture->pool);
  g_assert (expected != NULL);

  actual = populate_bnf_grammar_from_string (fixture->pool);
  g_assert (actual != NULL);
  g_assert (smie_test_bnf_grammar_equal (expected, actual));
  smie_bnf_grammar_free (expected);
  smie_bnf_grammar_free (actual);
}

static void
setup_prec2 (struct fixture *fixture, gconstpointer user_data)
{
  fixture->pool = smie_symbol_pool_alloc ();
  fixture->bnf = populate_bnf_grammar_from_string (fixture->pool);
}

static void
teardown_prec2 (struct fixture *fixture, gconstpointer user_data)
{
  smie_symbol_pool_free (fixture->pool);
  smie_bnf_grammar_free (fixture->bnf);
}

static void
test_construct_prec2 (struct fixture *fixture, gconstpointer user_data)
{
  smie_prec2_grammar_t *expected, *actual;
  GError *error;
  expected = populate_prec2_grammar (fixture->pool);
  actual = smie_prec2_grammar_alloc ();
  error = NULL;
  g_assert (smie_bnf_to_prec2 (fixture->bnf, actual, &error));
  g_assert_no_error (error);
  g_assert (smie_test_prec2_grammar_equal (expected, actual));
  smie_prec2_grammar_free (expected);
  smie_prec2_grammar_free (actual);
}

static void
setup_precs (struct fixture *fixture, gconstpointer user_data)
{
  fixture->pool = smie_symbol_pool_alloc ();
  fixture->prec2 = populate_prec2_grammar (fixture->pool);
}

static void
teardown_precs (struct fixture *fixture, gconstpointer user_data)
{
  smie_prec2_grammar_free (fixture->prec2);
  smie_symbol_pool_free (fixture->pool);
}

static void
test_construct_precs (struct fixture *fixture, gconstpointer user_data)
{
  smie_precs_grammar_t *expected, *actual;
  GError *error;
  expected = populate_precs_grammar (fixture->pool);
  actual = smie_precs_grammar_alloc ();
  error = NULL;
  g_assert (smie_prec2_to_precs (fixture->prec2, actual, &error));
  g_assert_no_error (error);
  g_assert (smie_test_precs_grammar_equal (expected, actual));
}

static void
setup_movement (struct fixture *fixture, gconstpointer user_data)
{
  fixture->pool = smie_symbol_pool_alloc ();
  fixture->precs = populate_precs_grammar (fixture->pool);
}

static void
teardown_movement (struct fixture *fixture, gconstpointer user_data)
{
  smie_precs_grammar_free (fixture->precs);
  smie_symbol_pool_free (fixture->pool);
}

static void
test_movement_forward (struct fixture *fixture, gconstpointer user_data)
{
  smie_test_context_t context;

  context.input = "# ( 4 + ( 5 x 6 ) + 7 ) + 8 #";

  context.offset = 1;
  smie_forward_sexp (fixture->precs,
		     smie_test_advance_func,
		     smie_test_read_func,
		     &context);
  g_assert_cmpint (22, ==, context.offset);

  context.offset = 2;
  smie_forward_sexp (fixture->precs,
		     smie_test_advance_func,
		     smie_test_read_func,
		     &context);
  g_assert_cmpint (22, ==, context.offset);

  context.offset = 3;
  smie_forward_sexp (fixture->precs,
		     smie_test_advance_func,
		     smie_test_read_func,
		     &context);
  g_assert_cmpint (6, ==, context.offset);

  context.offset = 7;
  smie_forward_sexp (fixture->precs,
		     smie_test_advance_func,
		     smie_test_read_func,
		     &context);
  g_assert_cmpint (16, ==, context.offset);
}

static void
test_movement_backward (struct fixture *fixture, gconstpointer user_data)
{
  smie_test_context_t context;

  context.input = "# ( 4 + ( 5 x 6 ) + 7 ) + 8 #";

  context.offset = 23;
  smie_backward_sexp (fixture->precs,
		      smie_test_advance_func,
		      smie_test_read_func,
		      &context);
  g_assert_cmpint (2, ==, context.offset);

  context.offset = 22;
  smie_backward_sexp (fixture->precs,
		      smie_test_advance_func,
		      smie_test_read_func,
		      &context);
  g_assert_cmpint (2, ==, context.offset);

  context.offset = 17;
  smie_backward_sexp (fixture->precs,
		      smie_test_advance_func,
		      smie_test_read_func, &context);
  g_assert_cmpint (8, ==, context.offset);

  context.offset = 7;
  smie_backward_sexp (fixture->precs,
		      smie_test_advance_func,
		      smie_test_read_func,
		      &context);
  g_assert_cmpint (6, ==, context.offset);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add ("/grammar/construct/bnf", struct fixture, NULL,
	      setup_bnf,
	      test_construct_bnf,
	      teardown_bnf);
  g_test_add ("/grammar/construct/prec2", struct fixture, NULL,
	      setup_prec2,
	      test_construct_prec2,
	      teardown_prec2);
  g_test_add ("/grammar/construct/precs", struct fixture, NULL,
	      setup_precs,
	      test_construct_precs,
	      teardown_precs);
  g_test_add ("/grammar/movement/forward", struct fixture, NULL,
	      setup_movement,
	      test_movement_forward,
	      teardown_movement);
  g_test_add ("/grammar/movement/backward", struct fixture, NULL,
	      setup_movement,
	      test_movement_backward,
	      teardown_movement);
  return g_test_run ();
}
