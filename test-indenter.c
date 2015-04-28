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

#include "smie-indenter.h"
#include "smie-test.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct fixture
{
  smie_indenter_t *indenter;
  void *grammar_addr;
  size_t grammar_size;
  void *input_addr;
  size_t input_size;
};

static void
setup (struct fixture *fixture, gconstpointer user_data)
{
  smie_symbol_pool_t *pool = smie_symbol_pool_alloc ();
  smie_bnf_grammar_t *bnf;
  smie_prec2_grammar_t *prec2;
  smie_grammar_t *grammar;
  int fd;
  struct stat statbuf;
  GError *error;

  fd = open (GRAMMAR_FILE, O_RDONLY);
  g_assert (fd >= 0);
  g_assert (fstat (fd, &statbuf) >= 0);
  fixture->grammar_size = statbuf.st_size;
  fixture->grammar_addr = mmap (NULL, fixture->grammar_size,
				PROT_READ, MAP_SHARED, fd, 0);
  close (fd);
  g_assert (fixture->grammar_addr);

  error = NULL;
  bnf = smie_bnf_grammar_alloc (pool);
  smie_bnf_grammar_load (bnf,
			 (const gchar *) fixture->grammar_addr,
			 &error);
  g_assert (bnf);
  g_assert_no_error (error);

  error = NULL;
  prec2 = smie_prec2_grammar_alloc (pool);
  g_assert (smie_bnf_to_prec2 (bnf, prec2, NULL, &error));
  g_assert_no_error (error);
  smie_bnf_grammar_free (bnf);

  error = NULL;
  grammar = smie_grammar_alloc (pool);
  smie_prec2_to_grammar (prec2, grammar, &error);
  g_assert_no_error (error);
  smie_prec2_grammar_free (prec2);
  smie_symbol_pool_unref (pool);

  fixture->indenter = smie_indenter_new (grammar,
					 2,
					 &smie_test_cursor_functions);

  fd = open (INPUT_FILE, O_RDONLY);
  g_assert (fd >= 0);
  g_assert (fstat (fd, &statbuf) >= 0);
  fixture->input_size = statbuf.st_size;
  fixture->input_addr = mmap (NULL, fixture->input_size,
			      PROT_READ, MAP_SHARED, fd, 0);
  close (fd);
  g_assert (fixture->input_addr);
}

static void
teardown (struct fixture *fixture, gconstpointer user_data)
{
  smie_indenter_unref (fixture->indenter);
  munmap (fixture->grammar_addr, fixture->grammar_size);
  munmap (fixture->input_addr, fixture->input_size);
}

static void
test_basic (struct fixture *fixture, gconstpointer user_data)
{
  struct smie_test_context_t context;
  gint column;
  context.input = fixture->input_addr;

  context.offset = 0;
  column = smie_indenter_calculate (fixture->indenter, &context);
  g_assert_cmpint (0, ==, column);

  context.offset = 34;
  column = smie_indenter_calculate (fixture->indenter, &context);
  g_assert_cmpint (2, ==, column);

  context.offset = 45;
  column = smie_indenter_calculate (fixture->indenter, &context);
  g_assert_cmpint (4, ==, column);

  context.offset = 51;
  column = smie_indenter_calculate (fixture->indenter, &context);
  g_assert_cmpint (2, ==, column);

  context.offset = 53;
  column = smie_indenter_calculate (fixture->indenter, &context);
  g_assert_cmpint (0, ==, column);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add ("/indenter/basic", struct fixture, NULL,
	      setup,
	      test_basic,
	      teardown);
  return g_test_run ();
}
