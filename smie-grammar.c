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

#include <glib/gprintf.h>
#include <string.h>
#include "smie-grammar.h"
#include "smie-gram-gen.h"
#include "smie-private.h"

static guint
smie_symbol_hash (gconstpointer key)
{
  const struct smie_symbol_t *symbol = key;
  return g_str_hash (symbol->name) ^ g_int_hash (&symbol->type);
}

static gboolean
smie_symbol_equal (gconstpointer a, gconstpointer b)
{
  const struct smie_symbol_t *as = a;
  const struct smie_symbol_t *bs = b;
  return as->type == bs->type && strcmp (as->name, bs->name) == 0;
}

const struct smie_symbol_t *
smie_symbol_intern (struct smie_symbol_pool_t *pool,
		    const gchar *name,
		    enum smie_symbol_type_t type)
{
  struct smie_symbol_t symbol, *result;
  symbol.name = (gchar *) name;
  symbol.type = type;
  if (!g_hash_table_lookup_extended (pool->allocated,
				     &symbol,
				     (gpointer *) &result,
				     NULL))
    {
      result = g_new0 (struct smie_symbol_t, 1);
      result->name = g_strdup (name);
      result->type = type;
      g_hash_table_add (pool->allocated, result);
    }
  return result;
}

static void
smie_symbol_free (struct smie_symbol_t *symbol)
{
  g_free (symbol->name);
  g_free (symbol);
}

struct smie_symbol_pool_t *
smie_symbol_pool_alloc (void)
{
  struct smie_symbol_pool_t *result = g_new0 (struct smie_symbol_pool_t, 1);
  result->ref_count = 1;
  result->allocated = g_hash_table_new_full (smie_symbol_hash,
					     smie_symbol_equal,
					     (GDestroyNotify) smie_symbol_free,
					     NULL);
  return result;
}

void
smie_symbol_pool_free (struct smie_symbol_pool_t *pool)
{
  g_hash_table_unref (pool->allocated);
  g_free (pool);
}

struct smie_symbol_pool_t *
smie_symbol_pool_ref (struct smie_symbol_pool_t *pool)
{
  g_return_val_if_fail (pool, NULL);
  g_return_val_if_fail (pool->ref_count > 0, NULL);
  g_atomic_int_inc (&pool->ref_count);
  return pool;
}

void
smie_symbol_pool_unref (struct smie_symbol_pool_t *pool)
{
  g_return_if_fail (pool);
  g_return_if_fail (pool->ref_count > 0);
  if (g_atomic_int_dec_and_test (&pool->ref_count))
    smie_symbol_pool_free (pool);
}

static struct smie_rule_t *
smie_rule_alloc (GList *symbols)
{
  struct smie_rule_t *result = g_new0 (struct smie_rule_t, 1);
  result->symbols = symbols;
  return result;
}

static void
smie_rule_free (struct smie_rule_t *rule)
{
  g_list_free (rule->symbols);
  g_free (rule);
}

static struct smie_rule_list_t *
smie_rule_list_alloc (void)
{
  return g_new0 (struct smie_rule_list_t, 1);
}

static void
smie_rule_list_free (struct smie_rule_list_t *rules)
{
  g_list_free_full (rules->rules, (GDestroyNotify) smie_rule_free);
  g_free (rules);
}

struct smie_bnf_grammar_t *
smie_bnf_grammar_alloc (struct smie_symbol_pool_t *pool)
{
  struct smie_bnf_grammar_t *result = g_new0 (struct smie_bnf_grammar_t, 1);
  result->pool = smie_symbol_pool_ref (pool);
  result->rules = g_hash_table_new_full (smie_symbol_hash,
					 smie_symbol_equal,
					 NULL,
					 (GDestroyNotify) smie_rule_list_free);
  return result;
}

void
smie_bnf_grammar_free (struct smie_bnf_grammar_t *bnf)
{
  smie_symbol_pool_unref (bnf->pool);
  g_hash_table_unref (bnf->rules);
  g_free (bnf);
}

gboolean
smie_bnf_grammar_load (struct smie_bnf_grammar_t *grammar,
		       const gchar *input,
		       GError **error)
{
  const gchar *cp = input;
  if (yyparse (grammar, &cp, error) != 0)
    {
      smie_bnf_grammar_free (grammar);
      return FALSE;
    }
  return TRUE;
}

gboolean
smie_bnf_grammar_add_rule (struct smie_bnf_grammar_t *bnf, GList *symbols)
{
  struct smie_rule_list_t *rules;
  GList *l;
  enum smie_symbol_type_t last_type = SMIE_SYMBOL_TERMINAL;

  g_return_val_if_fail (bnf, FALSE);
  g_return_val_if_fail (symbols && symbols->next, FALSE);

  /* Check if there are no consecutive non-terminals.  */
  for (l = symbols->next; l; l = l->next)
    {
      struct smie_symbol_t *symbol = l->data;
      g_return_val_if_fail (!(symbol->type == SMIE_SYMBOL_NON_TERMINAL
			      && last_type == SMIE_SYMBOL_NON_TERMINAL),
			    FALSE);
      last_type = symbol->type;
    }

  rules = g_hash_table_lookup (bnf->rules, symbols->data);
  if (!rules)
    {
      rules = smie_rule_list_alloc ();
      g_hash_table_insert (bnf->rules, symbols->data, rules);
    }

  rules->rules = g_list_append (rules->rules, smie_rule_alloc (symbols));
  return TRUE;
}

static guint
smie_prec2_hash (gconstpointer key)
{
  const struct smie_prec2_t *prec2 = key;
  return smie_symbol_hash (prec2->left) ^ smie_symbol_hash (prec2->right);
}

static gboolean
smie_prec2_equal (gconstpointer a, gconstpointer b)
{
  const struct smie_prec2_t *ap = a;
  const struct smie_prec2_t *bp = b;
  return smie_symbol_equal (ap->left, bp->left)
    && smie_symbol_equal (ap->right, bp->right);
}

static struct smie_prec2_t *
smie_prec2_alloc (const struct smie_symbol_t *left,
		  const struct smie_symbol_t *right)
{
  struct smie_prec2_t *result = g_new0 (struct smie_prec2_t, 1);
  result->left = left;
  result->right = right;
  return result;
}

struct smie_prec2_grammar_t *
smie_prec2_grammar_alloc (struct smie_symbol_pool_t *pool)
{
  struct smie_prec2_grammar_t *result = g_new0 (struct smie_prec2_grammar_t, 1);
  result->pool = smie_symbol_pool_ref (pool);
  result->prec2 = g_hash_table_new_full (smie_prec2_hash,
					 smie_prec2_equal,
					 g_free,
					 NULL);
  result->first = g_hash_table_new (smie_symbol_hash, smie_symbol_equal);
  result->last = g_hash_table_new (smie_symbol_hash, smie_symbol_equal);
  result->closer = g_hash_table_new (smie_symbol_hash, smie_symbol_equal);
  result->opener_closer = g_hash_table_new_full (smie_prec2_hash,
					      smie_prec2_equal,
					      g_free,
					      NULL);
  return result;
}

void
smie_prec2_grammar_free (struct smie_prec2_grammar_t *grammar)
{
  smie_symbol_pool_unref (grammar->pool);
  g_hash_table_unref (grammar->prec2);
  g_hash_table_unref (grammar->first);
  g_hash_table_unref (grammar->last);
  g_hash_table_unref (grammar->closer);
  g_hash_table_unref (grammar->opener_closer);
  g_free (grammar);
}

gboolean
smie_prec2_grammar_add_rule (struct smie_prec2_grammar_t *grammar,
			     const struct smie_symbol_t *a,
			     const struct smie_symbol_t *b,
			     enum smie_prec2_type_t type)
{
  struct smie_prec2_t *prec2 = smie_prec2_alloc (a, b);
  gpointer value;

  if (g_hash_table_lookup_extended (grammar->prec2, prec2, NULL, &value)
      && GPOINTER_TO_INT (value) != type)
    return FALSE;

  g_hash_table_insert (grammar->prec2, prec2, GINT_TO_POINTER (type));
  return TRUE;
}

static gboolean
smie_prec2_grammar_add_pair_symbol (struct smie_prec2_grammar_t *grammar,
				    const struct smie_symbol_t *opener_symbol,
				    const struct smie_symbol_t *closer_symbol,
				    gboolean is_last)
{
  struct smie_prec2_t p2;
  p2.left = opener_symbol;
  p2.right = closer_symbol;

  g_return_val_if_fail (!g_hash_table_contains (grammar->first, closer_symbol),
			FALSE);

  if (g_hash_table_contains (grammar->opener_closer, &p2))
    return FALSE;

  g_hash_table_add (grammar->first, (gpointer) opener_symbol);
  g_hash_table_add (grammar->closer, (gpointer) closer_symbol);
  if (is_last)
    g_hash_table_add (grammar->last, (gpointer) closer_symbol);

  return g_hash_table_add (grammar->opener_closer,
			   g_memdup (&p2, sizeof (struct smie_prec2_t)));
}

gboolean
smie_prec2_grammar_add_pair (struct smie_prec2_grammar_t *grammar,
			     const gchar *opener_token,
			     const gchar *closer_token,
			     gboolean is_last)
{
  const struct smie_symbol_t *opener_symbol, *closer_symbol;

  opener_symbol
    = smie_symbol_intern (grammar->pool, opener_token, SMIE_SYMBOL_TERMINAL);

  closer_symbol
    = smie_symbol_intern (grammar->pool, closer_token, SMIE_SYMBOL_TERMINAL);

  return smie_prec2_grammar_add_pair_symbol (grammar,
					     opener_symbol,
					     closer_symbol,
					     is_last);
}

struct smie_grammar_t *
smie_grammar_alloc (struct smie_symbol_pool_t *pool)
{
  struct smie_grammar_t *result = g_new0 (struct smie_grammar_t, 1);
  result->pool = smie_symbol_pool_ref (pool);
  result->precs = g_hash_table_new_full (smie_symbol_hash,
					 smie_symbol_equal,
					 NULL,
					 g_free);
  return result;
}

void
smie_grammar_free (struct smie_grammar_t *grammar)
{
  smie_symbol_pool_unref (grammar->pool);
  g_hash_table_unref (grammar->precs);
  if (grammar->opener_closer)
    g_hash_table_unref (grammar->opener_closer);
  g_free (grammar);
}

static GHashTable *
smie_bnf_grammar_build_op_set (struct smie_bnf_grammar_t *bnf,
			       gboolean is_last)
{
  GHashTableIter iter;
  gpointer key, value;
  GHashTable *op = g_hash_table_new_full (smie_symbol_hash,
					  smie_symbol_equal,
					  NULL,
					  (GDestroyNotify) g_hash_table_unref);
  gint change_count;

  /* Compute the initial set.  */
  g_hash_table_iter_init (&iter, bnf->rules);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      struct smie_symbol_t *a = key;
      struct smie_rule_list_t *rules = value;
      GList *l;
      GHashTable *op_a = g_hash_table_new (smie_symbol_hash, smie_symbol_equal);

      for (l = rules->rules; l; l = l->next)
	{
	  struct smie_rule_t *rule = l->data;

	  if (is_last)
	    {
	      GList *r = g_list_last (rule->symbols->next);
	      for (; r != rule->symbols; r = r->prev)
		{
		  struct smie_symbol_t *b = r->data;
		  if (b->type == SMIE_SYMBOL_TERMINAL
		      || b->type == SMIE_SYMBOL_TERMINAL_VARIABLE)
		    {
		      g_hash_table_add (op_a, b);
		      break;
		    }
		}
	    }
	  else
	    {
	      GList *r = rule->symbols->next;
	      for (; r; r = r->next)
		{
		  struct smie_symbol_t *b = r->data;
		  if (b->type == SMIE_SYMBOL_TERMINAL
		      || b->type == SMIE_SYMBOL_TERMINAL_VARIABLE)
		    {
		      g_hash_table_add (op_a, b);
		      break;
		    }
		}
	    }
	}
      g_hash_table_insert (op, a, op_a);
    }

  /* Loop until all the elements of OP are fixed.  */
  do
    {
      change_count = 0;
      g_hash_table_iter_init (&iter, bnf->rules);
      while (g_hash_table_iter_next (&iter, &key, &value))
	{
	  struct smie_symbol_t *a = key;
	  struct smie_rule_list_t *rules = value;
	  GList *l;

	  for (l = rules->rules; l; l = l->next)
	    {
	      struct smie_rule_t *rule = l->data;
	      GList *r = is_last ? rule->symbols->next
		: g_list_last (rule->symbols->next);
	      if (r)
		{
		  struct smie_symbol_t *b = r->data;
		  if (b->type == SMIE_SYMBOL_NON_TERMINAL)
		    {
		      GHashTable *op_a = g_hash_table_lookup (op, a);
		      GHashTable *op_b = g_hash_table_lookup (op, b);
		      GHashTableIter iter_b;
		      gpointer key_b;
		      g_hash_table_iter_init (&iter_b, op_b);
		      while (g_hash_table_iter_next (&iter_b, &key_b, NULL))
			if (g_hash_table_add (op_a, key_b))
			  change_count++;
		    }
		}
	    }
	}
    }
  while (change_count > 0);
  return op;
}

#ifdef DEBUG
static void
smie_debug_dump_op_set (GHashTable *op, const char *name)
{
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, op);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      struct smie_symbol_t *symbol = key;
      GHashTable *set = value;
      GHashTableIter iter2;
      gpointer key2;
      g_printf ("%s(%s): ", name, symbol->name);
      g_hash_table_iter_init (&iter2, set);
      while (g_hash_table_iter_next (&iter2, &key2, NULL))
	{
	  struct smie_symbol_t *symbol2 = key2;
	  g_printf ("%s ", symbol2->name);
	}
      g_printf ("\n");
    }
}
#endif

gboolean
smie_bnf_to_prec2 (struct smie_bnf_grammar_t *bnf,
		   struct smie_prec2_grammar_t *prec2,
		   GError **error)
{
  GHashTable *first_op = smie_bnf_grammar_build_op_set (bnf, FALSE);
  GHashTable *last_op = smie_bnf_grammar_build_op_set (bnf, TRUE);
  GHashTableIter iter;
  gpointer value;

  g_hash_table_iter_init (&iter, bnf->rules);
  while (g_hash_table_iter_next (&iter, NULL, &value))
    {
      struct smie_rule_list_t *rules = value;
      GList *l;
      for (l = rules->rules; l; l = l->next)
	{
	  struct smie_rule_t *rule = l->data;
	  GList *l1;

	  /* Mark first/last */
	  if (rule->symbols->next)
	    {
	      struct smie_symbol_t *first_symbol = rule->symbols->next->data;
	      GList *last = g_list_last (rule->symbols->next);
	      struct smie_symbol_t *last_symbol = last->data;

	      if (first_symbol != last_symbol)
		{
		  if (first_symbol->type == SMIE_SYMBOL_TERMINAL
		      || first_symbol->type == SMIE_SYMBOL_TERMINAL_VARIABLE)
		    {
		      GList *l2;
		      g_hash_table_add (prec2->first, first_symbol);
		      l2 = rule->symbols->next->next;
		      for (; l2; l2 = l2->next)
			{
			  struct smie_symbol_t *closer = l2->data;
			  gboolean is_last = l2->next == NULL;
			  if (closer->type == SMIE_SYMBOL_TERMINAL
			      || closer->type == SMIE_SYMBOL_TERMINAL_VARIABLE)
			    smie_prec2_grammar_add_pair_symbol (prec2,
								first_symbol,
								closer,
								is_last);
			}
		    }
		}
	    }
	  for (l1 = rule->symbols->next; l1; l1 = l1->next)
	    {
	      struct smie_symbol_t *a = l1->data;
	      if (a->type == SMIE_SYMBOL_TERMINAL
		  || a->type == SMIE_SYMBOL_TERMINAL_VARIABLE)
		{
		  GList *l2 = l1->next;
		  if (l2)
		    {
		      struct smie_symbol_t *b = l2->data;
		      if (b->type == SMIE_SYMBOL_TERMINAL
			  || b->type == SMIE_SYMBOL_TERMINAL_VARIABLE)
			{
			  struct smie_prec2_t *p
			    = smie_prec2_alloc (a, b);
			  gpointer v = GINT_TO_POINTER (SMIE_PREC2_EQ);
			  g_hash_table_insert (prec2->prec2, p, v);
			}
		      else if (b->type == SMIE_SYMBOL_NON_TERMINAL
			       || b->type == SMIE_SYMBOL_TERMINAL_VARIABLE)
			{
			  GHashTable *op_b;
			  GHashTableIter iter_b;
			  gpointer key_b;
			  GList *l3 = l2->next;
			  if (l3)
			    {
			      struct smie_symbol_t *c = l3->data;
			      if (c->type == SMIE_SYMBOL_TERMINAL
				  || c->type == SMIE_SYMBOL_TERMINAL_VARIABLE)
				{
				  struct smie_prec2_t *p
				    = smie_prec2_alloc (a, c);
				  gpointer v = GINT_TO_POINTER (SMIE_PREC2_EQ);
				  g_hash_table_insert (prec2->prec2, p, v);
				}
			    }
			  op_b = g_hash_table_lookup (first_op, b);
			  if (!op_b)
			    continue;
			  g_hash_table_iter_init (&iter_b, op_b);
			  while (g_hash_table_iter_next (&iter_b, &key_b, NULL))
			    {
			      struct smie_symbol_t *d = key_b;
			      struct smie_prec2_t *p
				= smie_prec2_alloc (a, d);
			      gpointer v = GINT_TO_POINTER (SMIE_PREC2_LT);
			      g_hash_table_insert (prec2->prec2, p, v);
			    }
			}
		    }
		}
	      else
		{
		  GList *l2 = l1->next;
		  if (l2)
		    {
		      struct smie_symbol_t *b = l2->data;
		      if (b->type == SMIE_SYMBOL_TERMINAL
			  || b->type == SMIE_SYMBOL_TERMINAL_VARIABLE)
			{
			  GHashTable *op_a;
			  GHashTableIter iter_a;
			  gpointer key_a;
			  op_a = g_hash_table_lookup (last_op, a);
			  if (!op_a)
			    continue;
			  g_hash_table_iter_init (&iter_a, op_a);
			  while (g_hash_table_iter_next (&iter_a, &key_a, NULL))
			    {
			      struct smie_symbol_t *e = key_a;
			      struct smie_prec2_t *p
				= smie_prec2_alloc (e, b);
			      gpointer v = GINT_TO_POINTER (SMIE_PREC2_GT);
			      g_hash_table_insert (prec2->prec2, p, v);
			    }
			}
		    }
		}
	    }
	}
    }

  g_hash_table_unref (first_op);
  g_hash_table_unref (last_op);
  return TRUE;
}

#ifdef DEBUG
void
smie_debug_dump_prec2_grammar (struct smie_prec2_grammar_t *grammar)
{
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, grammar->prec2);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      struct smie_prec2_t *prec2 = key;
      enum smie_prec2_type_t type = GPOINTER_TO_INT (value);
      g_printf ("%s %c %s\n",
		prec2->left->name,
		type == SMIE_PREC2_EQ ? '='
		: type == SMIE_PREC2_LT ? '<' : '>',
		prec2->right->name);
    }
}
#endif

static guint smie_func2_hash (gconstpointer key);

static guint
smie_func_hash (gconstpointer key)
{
  const struct smie_func_t *func = key;
  guint hash = g_int_hash (&func->type);
  switch (func->type)
    {
    case SMIE_FUNC_F:
    case SMIE_FUNC_G:
      hash ^= smie_symbol_hash (func->u.symbol);
      break;
    case SMIE_FUNC_COMPOSED:
      hash ^= smie_func2_hash (&func->u.composed);
      break;
    }
  return hash;
}

#ifdef DEBUG
static gchar *
smie_debug_func_name (const struct smie_func_t *func)
{
  switch (func->type)
    {
    case SMIE_FUNC_F:
      return g_strdup_printf ("f_%s", func->u.symbol->name);
    case SMIE_FUNC_G:
      return g_strdup_printf ("g_%s", func->u.symbol->name);
    case SMIE_FUNC_COMPOSED:
      {
	gchar *f_name = smie_debug_func_name (func->u.composed.f);
	gchar *g_name = smie_debug_func_name (func->u.composed.g);
	gchar *composed_name = g_strdup_printf ("%s,%s", f_name, g_name);
	g_free (f_name);
	g_free (g_name);
	return composed_name;
      }
    }
}

static void
smie_debug_dump_unassigned (GHashTable *unassigned)
{
  GHashTableIter iter;
  gpointer key;
  g_hash_table_iter_init (&iter, unassigned);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      struct smie_func2_t *func2 = key;
      gchar *f_name = smie_debug_func_name (func2->f);
      gchar *g_name = smie_debug_func_name (func2->g);
      g_printf ("%s < %s\n", f_name, g_name);
      g_free (f_name);
      g_free (g_name);
    }
}

static void
smie_debug_dump_assigned (GHashTable *assigned)
{
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, assigned);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      struct smie_func_t *func = key;
      gint prec = GPOINTER_TO_INT (value);
      gchar *f_name = smie_debug_func_name (func);
      g_printf ("%s %d\n", f_name, prec);
      g_free (f_name);
    }
}

void
smie_debug_dump_grammar (struct smie_grammar_t *grammar)
{
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, grammar->precs);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      struct smie_symbol_t *symbol = key;
      struct smie_prec_t *prec = value;
      g_printf ("f(%s) = %d\n", symbol->name, prec->left_prec);
      g_printf ("g(%s) = %d\n", symbol->name, prec->right_prec);
    }
}
#endif

static gboolean
smie_func_equal (gconstpointer a, gconstpointer b)
{
  const struct smie_func_t *fa = a;
  const struct smie_func_t *fb = b;
  if (fa->type != fb->type)
    return FALSE;
  switch (fa->type)
    {
    case SMIE_FUNC_F:
    case SMIE_FUNC_G:
      return smie_symbol_equal (fa->u.symbol, fb->u.symbol);
    case SMIE_FUNC_COMPOSED:
      return smie_func_equal (fa->u.composed.f, fb->u.composed.f)
	&& smie_func_equal (fa->u.composed.g, fb->u.composed.g);
    }
  return FALSE;
}

static struct smie_func2_t *
smie_func2_alloc (const struct smie_func_t *f, const struct smie_func_t *g)
{
  struct smie_func2_t *result = g_new0 (struct smie_func2_t, 1);
  result->f = f;
  result->g = g;
  return result;
}

static guint
smie_func2_hash (gconstpointer key)
{
  const struct smie_func2_t *func2 = key;
  return smie_func_hash (func2->f) ^ smie_func_hash (func2->g);
}

static gboolean
smie_func2_equal (gconstpointer a, gconstpointer b)
{
  const struct smie_func2_t *fa = a;
  const struct smie_func2_t *fb = b;
  return smie_func_equal (fa->f, fb->f) && smie_func_equal (fa->g, fb->g);
}

gboolean
smie_prec2_to_precs (struct smie_prec2_grammar_t *prec2,
		     struct smie_grammar_t *precs,
		     GError **error)
{
  GHashTable *allocated = g_hash_table_new_full (smie_func_hash,
						 smie_func_equal,
						 g_free,
						 NULL);
  GHashTable *unassigned = g_hash_table_new_full (smie_func2_hash,
						  smie_func2_equal,
						  g_free,
						  NULL);
  GHashTable *composed = g_hash_table_new (smie_func_hash, smie_func_equal);
  GHashTable *assigned = g_hash_table_new (smie_func_hash, smie_func_equal);
  gint iteration_count;
  GHashTableIter iter;
  gpointer key, value;
  gboolean result = TRUE;

  /* Allocate all possible functions.  */
  g_hash_table_iter_init (&iter, precs->pool->allocated);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      struct smie_symbol_t *symbol = key;
      if (symbol->type == SMIE_SYMBOL_TERMINAL
	  || symbol->type == SMIE_SYMBOL_TERMINAL_VARIABLE)
	{
	  struct smie_func_t *func;
	  func = g_new0 (struct smie_func_t, 1);
	  func->u.symbol = key;
	  func->type = SMIE_FUNC_F;
	  g_hash_table_add (allocated, func);
	  func = g_new0 (struct smie_func_t, 1);
	  func->u.symbol = key;
	  func->type = SMIE_FUNC_G;
	  g_hash_table_add (allocated, func);
	}
    }

  g_hash_table_iter_init (&iter, prec2->prec2);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      struct smie_prec2_t *p2 = key;
      enum smie_prec2_type_t p2_type = GPOINTER_TO_INT (value);
      struct smie_func_t func, *f, *g, *fg;

      func.u.symbol = p2->left;
      func.type = SMIE_FUNC_F;
      g_hash_table_lookup_extended (allocated,
				    &func,
				    (gpointer *) &f,
				    NULL);

      func.u.symbol = p2->right;
      func.type = SMIE_FUNC_G;
      g_hash_table_lookup_extended (allocated,
				    &func,
				    (gpointer *) &g,
				    NULL);

      switch (p2_type)
	{
	case SMIE_PREC2_LT:
	  g_hash_table_add (unassigned, smie_func2_alloc (f, g));
	  break;
	case SMIE_PREC2_GT:
	  g_hash_table_add (unassigned, smie_func2_alloc (g, f));
	  break;
	case SMIE_PREC2_EQ:
	  func.u.composed.f = f;
	  func.u.composed.g = g;
	  func.type = SMIE_FUNC_COMPOSED;
	  if (!g_hash_table_lookup_extended (allocated,
					     &func,
					     (gpointer *) &fg,
					     NULL))
	    {
	      fg = g_memdup (&func, sizeof (struct smie_func_t));
	      g_hash_table_add (allocated, fg);
	    }
	  g_hash_table_insert (composed, f, fg);
	  g_hash_table_insert (composed, g, fg);
	  break;
	}
    }

  /* Replace all occurrences of composed functions.  */
  g_hash_table_iter_init (&iter, unassigned);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      struct smie_func2_t *f2 = key;
      struct smie_func_t *fg;
      fg = g_hash_table_lookup (composed, f2->f);
      if (fg)
	f2->f = fg;

      fg = g_hash_table_lookup (composed, f2->g);
      if (fg)
	f2->g = fg;
    }

  /* Loop until unassigned is empty.  */
  iteration_count = 0;
  while (g_hash_table_size (unassigned) > 0)
    {
      GList *to_remove = NULL, *l;

      g_hash_table_iter_init (&iter, unassigned);
      while (g_hash_table_iter_next (&iter, &key, NULL))
	{
	  struct smie_func2_t *funcs = key;
	  GHashTableIter iter2;
	  gpointer key2;
	  gboolean found = FALSE;

	  g_hash_table_iter_init (&iter2, unassigned);
	  while (g_hash_table_iter_next (&iter2, &key2, NULL))
	    {
	      struct smie_func2_t *funcs2 = key2;
	      if (smie_func_equal (funcs->f, funcs2->g))
		{
		  found = TRUE;
		  break;
		}
	    }
	  if (!found)
	    to_remove = g_list_append (to_remove, (gpointer) funcs->f);
	}

      if (!to_remove)
	{
	  g_set_error (error, SMIE_ERROR, SMIE_ERROR_GRAMMAR,
		       "cycle found in prec2 grammar");
	  g_hash_table_unref (composed);
	  g_hash_table_unref (unassigned);
	  result = FALSE;
	  goto out;
	}

      for (l = to_remove; l; l = l->next)
	{
	  struct smie_func_t *func = l->data;
	  if (!g_hash_table_contains (assigned, func))
	    {
	      g_hash_table_insert (assigned,
				   func,
				   GINT_TO_POINTER (iteration_count));
	      iteration_count++;
	    }
	  g_hash_table_iter_init (&iter, unassigned);
	  while (g_hash_table_iter_next (&iter, &key, NULL))
	    {
	      struct smie_func2_t *funcs = key;
	      if (smie_func_equal (funcs->f, func))
		g_hash_table_iter_remove (&iter);
	    }
	}
      g_list_free (to_remove);
      iteration_count += 10;
    }
  g_hash_table_unref (unassigned);

  /* Decompose functions in assigned.  */
  g_hash_table_remove_all (composed);
  g_hash_table_iter_init (&iter, assigned);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      struct smie_func_t *func = key;
      if (func->type == SMIE_FUNC_COMPOSED)
	{
	  g_hash_table_insert (composed, (gpointer) func->u.composed.f, value);
	  g_hash_table_insert (composed, (gpointer) func->u.composed.g, value);
	  g_hash_table_iter_remove (&iter);
	}
    }
  g_hash_table_iter_init (&iter, composed);
  while (g_hash_table_iter_next (&iter, &key, &value))
    g_hash_table_insert (assigned, key, value);
  g_hash_table_unref (composed);

  /* Add still unassigned functions.  */
  g_hash_table_iter_init (&iter, allocated);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      struct smie_func_t *func = key;
      if (func->type != SMIE_FUNC_COMPOSED
	  && !g_hash_table_contains (assigned, func))
	g_hash_table_insert (assigned,
			     func,
			     GINT_TO_POINTER (iteration_count++));
    }

  g_hash_table_iter_init (&iter, assigned);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      struct smie_func_t *func = key;
      struct smie_prec_t *prec
	= g_hash_table_lookup (precs->precs, (gpointer) func->u.symbol);
      if (!prec)
	{
	  prec = g_new0 (struct smie_prec_t, 1);
	  prec->is_first
	    = g_hash_table_contains (prec2->first,
				     (gpointer) func->u.symbol);
	  prec->is_last
	    = g_hash_table_contains (prec2->last,
				     (gpointer) func->u.symbol);
	  prec->is_closer
	    = g_hash_table_contains (prec2->closer,
				     (gpointer) func->u.symbol);
	  g_hash_table_insert (precs->precs, (gpointer) func->u.symbol, prec);
	}
      switch (func->type)
	{
	case SMIE_FUNC_F:
	  prec->left_prec = GPOINTER_TO_INT (value);
	  break;
	case SMIE_FUNC_G:
	  prec->right_prec = GPOINTER_TO_INT (value);
	  break;
	default:
	  g_assert_not_reached ();
	}
    }
  precs->opener_closer = g_hash_table_ref (prec2->opener_closer);
 out:
  g_hash_table_unref (assigned);
  g_hash_table_unref (allocated);
  return result;
}

gboolean
smie_grammar_add_rule (struct smie_grammar_t *grammar,
		       const struct smie_symbol_t *symbol,
		       gint left_prec,
		       gboolean left_is_first,
		       gint right_prec,
		       gboolean right_is_last,
		       gboolean right_is_closer)
{
  struct smie_prec_t *prec = g_new0 (struct smie_prec_t, 1);
  prec->left_prec = left_prec;
  prec->is_first = left_is_first;
  prec->right_prec = right_prec;
  prec->is_last = right_is_last;
  prec->is_closer = right_is_closer;
  return g_hash_table_insert (grammar->precs,
			      (gpointer) symbol,
			      prec);
}

gboolean
smie_grammar_is_first (struct smie_grammar_t *grammar,
		       const gchar *token)
{
  const struct smie_symbol_t *symbol
    = smie_symbol_intern (grammar->pool, token, SMIE_SYMBOL_TERMINAL);
  struct smie_prec_t *prec
    = g_hash_table_lookup (grammar->precs, (gpointer) symbol);
  return prec && prec->is_first;
}

gboolean
smie_grammar_is_last (struct smie_grammar_t *grammar,
		      const gchar *token)
{
  const struct smie_symbol_t *symbol
    = smie_symbol_intern (grammar->pool, token, SMIE_SYMBOL_TERMINAL);
  struct smie_prec_t *prec
    = g_hash_table_lookup (grammar->precs, (gpointer) symbol);
  return prec && prec->is_last;
}

gboolean
smie_grammar_is_closer (struct smie_grammar_t *grammar,
			const gchar *token)
{
  const struct smie_symbol_t *symbol
    = smie_symbol_intern (grammar->pool, token, SMIE_SYMBOL_TERMINAL);
  struct smie_prec_t *prec
    = g_hash_table_lookup (grammar->precs, (gpointer) symbol);
  return prec && prec->is_closer;
}

gboolean
smie_grammar_is_pair (smie_grammar_t *grammar,
		      const gchar *opener_token,
		      const gchar *closer_token)
{
  struct smie_prec2_t p2;
  if (!grammar->opener_closer)
    return FALSE;
  else
    {
      const struct smie_symbol_t *opener_symbol
	= smie_symbol_intern (grammar->pool, opener_token,
			      SMIE_SYMBOL_TERMINAL);
      const struct smie_symbol_t *closer_symbol
	= smie_symbol_intern (grammar->pool, closer_token,
			      SMIE_SYMBOL_TERMINAL);
      p2.left = opener_symbol;
      p2.right = closer_symbol;
      return g_hash_table_contains (grammar->opener_closer, &p2);
    }
}

typedef gboolean (*smie_select_function_t) (struct smie_prec_t *, gint *);

static gboolean
smie_select_left (struct smie_prec_t *prec, gint *precp)
{
  *precp = prec->left_prec;
  return prec->is_first;
}

static gboolean
smie_select_right (struct smie_prec_t *prec, gint *precp)
{
  *precp = prec->right_prec;
  return prec->is_last;
}

enum smie_direction_t
  {
    SMIE_DIRECTION_FORWARD,
    SMIE_DIRECTION_BACKWARD
  };

static gboolean
smie_advance_sexp (struct smie_grammar_t *grammar,
		   enum smie_direction_t direction,
		   gint count,
		   smie_advance_function_t advance_func,
		   smie_read_token_function_t read_func,
		   gpointer context,
		   smie_select_function_t op_forward,
		   smie_select_function_t op_backward)
{
  GList *stack = NULL;

  /* Place the cursor on the token.  */
  if (!read_func (NULL, context)
      && !advance_func (SMIE_ADVANCE_TOKENS, count, context))
    return FALSE;

  do
    {
      struct smie_symbol_t symbol;
      struct smie_prec_t *prec;
      gint prec_value;
      gchar *token;

      if (!read_func (&token, context))
	goto out;

      symbol.name = token;
      symbol.type = SMIE_SYMBOL_TERMINAL;
      prec = g_hash_table_lookup (grammar->precs, &symbol);
      g_free (token);
      if (!prec)
	continue;
      else if (op_backward (prec, &prec_value))
	stack = g_list_prepend (stack, prec);
      else
	{
	  while (stack)
	    {
	      struct smie_prec_t *prec2 = stack->data;
	      gint prec_value2;
	      op_forward (prec, &prec_value);
	      op_backward (prec2, &prec_value2);
	      if (prec_value >= prec_value2)
		break;
	      stack = g_list_delete_link (stack, stack);
	    }
	  if (!stack)
	    return TRUE;
	  else
	    {
	      struct smie_prec_t *prec2 = stack->data;
	      gint prec_value2;
	      op_forward (prec, &prec_value);
	      op_backward (prec2, &prec_value2);
	      if (prec_value == prec_value2)
		stack = g_list_delete_link (stack, stack);
	      if (stack)
		{
		  if (!op_forward (prec, &prec_value))
		    stack = g_list_prepend (stack, prec);
		}
	      else if (op_forward (prec, &prec_value))
		goto out;
	    }
	}
    }
  while (advance_func (SMIE_ADVANCE_TOKENS, count, context));

 out:
  g_list_free (stack);
  return FALSE;
}

gboolean
smie_forward_sexp (struct smie_grammar_t *grammar,
		   smie_advance_function_t advance_func,
		   smie_read_token_function_t read_func,
		   gpointer context)
{
  return smie_advance_sexp (grammar,
			    SMIE_DIRECTION_FORWARD,
			    1,
			    advance_func,
			    read_func,
			    context,
			    smie_select_right,
			    smie_select_left);
}

gboolean
smie_backward_sexp (struct smie_grammar_t *grammar,
		    smie_advance_function_t advance_func,
		    smie_read_token_function_t read_func,
		    gpointer context)
{
  return smie_advance_sexp (grammar,
			    SMIE_DIRECTION_BACKWARD,
			    -1,
			    advance_func,
			    read_func,
			    context,
			    smie_select_left,
			    smie_select_right);
}

GQuark
smie_error_quark (void)
{
  return g_quark_from_static_string ("smie-error-quark");
}
