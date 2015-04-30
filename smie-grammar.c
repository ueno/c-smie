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
smie_bnf_grammar_load (struct smie_bnf_grammar_t *bnf,
		       const gchar *input,
		       GError **error)
{
  const gchar *cp = input;
  if (yyparse (bnf, &cp, error) != 0)
    {
      smie_bnf_grammar_free (bnf);
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

struct smie_prec2_grammar_t *
smie_prec2_grammar_alloc (struct smie_symbol_pool_t *pool)
{
  struct smie_prec2_grammar_t *result = g_new0 (struct smie_prec2_grammar_t, 1);
  result->pool = smie_symbol_pool_ref (pool);
  result->prec2 = g_hash_table_new_full (smie_prec2_hash,
					 smie_prec2_equal,
					 g_free,
					 NULL);
  result->classes = g_hash_table_new (smie_symbol_hash, smie_symbol_equal);
  result->pairs = g_hash_table_new_full (smie_prec2_hash,
					 smie_prec2_equal,
					 g_free,
					 NULL);
  result->ends = g_hash_table_new (smie_symbol_hash, smie_symbol_equal);
  return result;
}

void
smie_prec2_grammar_free (struct smie_prec2_grammar_t *prec2)
{
  smie_symbol_pool_unref (prec2->pool);
  g_hash_table_unref (prec2->prec2);
  g_hash_table_unref (prec2->classes);
  g_hash_table_unref (prec2->pairs);
  g_hash_table_unref (prec2->ends);
  g_free (prec2);
}

gboolean
smie_prec2_grammar_add_rule (struct smie_prec2_grammar_t *prec2,
			     const struct smie_symbol_t *left,
			     const struct smie_symbol_t *right,
			     enum smie_prec2_type_t type,
			     struct smie_prec2_grammar_t *override)
{
  struct smie_prec2_t p2;
  gpointer value;

  p2.left = left;
  p2.right = right;
  if (g_hash_table_lookup_extended (prec2->prec2, &p2, NULL, &value)
      && GPOINTER_TO_INT (value) != type)
    {
      if (override
	  && g_hash_table_lookup_extended (override->prec2, &p2, NULL, &value))
	{
	  g_hash_table_insert (prec2->prec2,
			       g_memdup (&p2, sizeof (struct smie_prec2_t)),
			       value);
	  return TRUE;
	}
      return FALSE;
    }

  g_hash_table_insert (prec2->prec2,
		       g_memdup (&p2, sizeof (struct smie_prec2_t)),
		       GINT_TO_POINTER (type));
  return TRUE;
}

gboolean
smie_prec2_grammar_add_pair (struct smie_prec2_grammar_t *prec2,
			     const struct smie_symbol_t *opener_symbol,
			     const struct smie_symbol_t *closer_symbol)
{
  struct smie_prec2_t p2;

  p2.left = opener_symbol;
  p2.right = closer_symbol;
  if (g_hash_table_contains (prec2->pairs, &p2))
    return FALSE;

  g_hash_table_add (prec2->ends, (gpointer) closer_symbol);
  return g_hash_table_add (prec2->pairs,
			   g_memdup (&p2, sizeof (struct smie_prec2_t)));
}

gboolean
smie_prec2_grammar_set_symbol_class (struct smie_prec2_grammar_t *prec2,
				     const struct smie_symbol_t *symbol,
				     enum smie_symbol_class_t symbol_class)
{
  return g_hash_table_insert (prec2->classes,
			      (gpointer) symbol,
			      GINT_TO_POINTER (symbol_class));
}

struct smie_precs_grammar_t *
smie_precs_grammar_alloc (struct smie_symbol_pool_t *pool)
{
  struct smie_precs_grammar_t *result = g_new0 (struct smie_precs_grammar_t, 1);
  result->pool = smie_symbol_pool_ref (pool);
  return result;
}

static void
smie_prec_free (struct smie_prec_t *prec)
{
  g_list_free (prec->op);
  g_free (prec);
}

void
smie_precs_grammar_free (struct smie_precs_grammar_t *precs)
{
  smie_symbol_pool_unref (precs->pool);
  g_list_free_full (precs->precs, (GDestroyNotify) smie_prec_free);
  g_free (precs);
}

void
smie_precs_grammar_add_prec (struct smie_precs_grammar_t *precs,
			     smie_prec_type_t type,
			     const smie_symbol_t **symbols)
{
  struct smie_prec_t *prec = g_new0 (struct smie_prec_t, 1);
  prec->type = type;
  for (; *symbols; symbols++)
    prec->op = g_list_append (prec->op, (gpointer) *symbols);
  precs->precs = g_list_append (precs->precs, (gpointer) prec);
}

static void
smie_precs_to_prec2 (struct smie_precs_grammar_t *precs,
		     struct smie_prec2_grammar_t *prec2)
{
  GList *l = precs->precs;
  for (; l; l = l->next)
    {
      struct smie_prec_t *prec = l->data;
      smie_prec_type_t type = prec->type;
      struct smie_symbol_t *op = prec->op->data;
      GList *l2 = prec->op;
      for (; l2; l2 = l2->next)
	{
	  struct smie_symbol_t *other_op = l2->data;
	  GList *l3;
	  smie_prec_type_t op1, op2;
	  switch (type)
	    {
	    case SMIE_PREC_LEFT:
	      smie_prec2_grammar_add_rule (prec2, op, other_op, SMIE_PREC2_GT,
					   NULL);
	      break;
	    case SMIE_PREC_RIGHT:
	      smie_prec2_grammar_add_rule (prec2, op, other_op, SMIE_PREC2_LT,
					   NULL);
	      break;
	    case SMIE_PREC_ASSOC:
	      smie_prec2_grammar_add_rule (prec2,
					   op, other_op, SMIE_PREC2_EQ,
					   NULL);
	      break;
	    default:
	      break;
	    }

	  op1 = SMIE_PREC2_LT;
	  op2 = SMIE_PREC2_GT;
	  for (l3 = precs->precs; l3; l3 = l3->next)
	    {
	      struct smie_prec_t *other_prec = l3->data;
	      if (prec == other_prec)
		{
		  op1 = SMIE_PREC2_GT;
		  op2 = SMIE_PREC2_LT;
		}
	      else
		{
		  GList *l4 = other_prec->op;
		  other_op = l4->data;
		  for (; l4; l4 = l4->next)
		    {
		      smie_prec2_grammar_add_rule (prec2, op, other_op, op2,
						   NULL);
		      smie_prec2_grammar_add_rule (prec2, other_op, op, op1,
						   NULL);
		    }
		}
	    }
	}
    }
}

struct smie_grammar_t *
smie_grammar_alloc (struct smie_symbol_pool_t *pool)
{
  struct smie_grammar_t *result = g_new0 (struct smie_grammar_t, 1);
  result->pool = smie_symbol_pool_ref (pool);
  result->levels = g_hash_table_new_full (smie_symbol_hash,
					  smie_symbol_equal,
					  NULL,
					  g_free);
  return result;
}

void
smie_grammar_free (struct smie_grammar_t *grammar)
{
  smie_symbol_pool_unref (grammar->pool);
  g_hash_table_unref (grammar->levels);
  if (grammar->pairs)
    g_hash_table_unref (grammar->pairs);
  if (grammar->ends)
    g_hash_table_unref (grammar->ends);
  g_free (grammar);
}

struct smie_symbol_pool_t *
smie_grammar_get_symbol_pool (smie_grammar_t *grammar)
{
  return grammar->pool;
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
		   GList *resolvers,
		   GError **error)
{
  GHashTable *first_op = smie_bnf_grammar_build_op_set (bnf, FALSE);
  GHashTable *last_op = smie_bnf_grammar_build_op_set (bnf, TRUE);
  GHashTableIter iter;
  struct smie_prec2_grammar_t *override = NULL;
  gpointer value;

  if (resolvers)
    {
      GList *l;
      override = smie_prec2_grammar_alloc (smie_symbol_pool_ref (prec2->pool));
      for (l = resolvers; l; l = l->next)
	{
	  struct smie_precs_grammar_t *precs = l->data;
	  smie_precs_to_prec2 (precs, override);
	}
    }

  g_hash_table_iter_init (&iter, bnf->rules);
  while (g_hash_table_iter_next (&iter, NULL, &value))
    {
      struct smie_rule_list_t *rules = value;
      GList *l;
      for (l = rules->rules; l; l = l->next)
	{
	  struct smie_rule_t *rule = l->data;
	  GList *l1;

	  /* Mark closer and opener.  */
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
		      smie_prec2_grammar_set_symbol_class (prec2,
							   first_symbol,
							   SMIE_SYMBOL_CLASS_OPENER);
		      l2 = rule->symbols->next->next;
		      for (; l2; l2 = l2->next)
			{
			  struct smie_symbol_t *closer = l2->data;
			  if (closer->type == SMIE_SYMBOL_TERMINAL
			      || closer->type == SMIE_SYMBOL_TERMINAL_VARIABLE)
			    {
			      smie_prec2_grammar_add_pair (prec2,
							   first_symbol,
							   closer);
			      if (!l2->next)
				smie_prec2_grammar_set_symbol_class (prec2,
								     closer,
								     SMIE_SYMBOL_CLASS_CLOSER);
			    }
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
			smie_prec2_grammar_add_rule (prec2,
						     a,
						     b,
						     SMIE_PREC2_EQ,
						     override);
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
				smie_prec2_grammar_add_rule (prec2,
							     a,
							     c,
							     SMIE_PREC2_EQ,
							     override);
			    }
			  op_b = g_hash_table_lookup (first_op, b);
			  if (!op_b)
			    continue;
			  g_hash_table_iter_init (&iter_b, op_b);
			  while (g_hash_table_iter_next (&iter_b, &key_b, NULL))
			    {
			      struct smie_symbol_t *d = key_b;
			      smie_prec2_grammar_add_rule (prec2,
							   a,
							   d,
							   SMIE_PREC2_LT,
							   override);
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
			      smie_prec2_grammar_add_rule (prec2,
							   e,
							   b,
							   SMIE_PREC2_GT,
							   override);
			    }
			}
		    }
		}
	    }
	}
    }

  if (override)
    smie_prec2_grammar_free (override);
  g_hash_table_unref (first_op);
  g_hash_table_unref (last_op);
  return TRUE;
}

#ifdef DEBUG
void
smie_debug_dump_prec2_grammar (struct smie_prec2_grammar_t *prec2)
{
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, prec2->prec2);
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
      hash ^= smie_symbol_hash (func->symbol);
      break;
    }
  return hash;
}

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
      return smie_symbol_equal (fa->symbol, fb->symbol);
    }
  return FALSE;
}

#ifdef DEBUG
static gchar *
smie_debug_func_name (const struct smie_func_t *func)
{
  switch (func->type)
    {
    case SMIE_FUNC_F:
      return g_strdup_printf ("f_%s", func->symbol->name);
    case SMIE_FUNC_G:
      return g_strdup_printf ("g_%s", func->symbol->name);
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

  g_hash_table_iter_init (&iter, grammar->levels);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      struct smie_symbol_t *symbol = key;
      struct smie_level_t *level = value;
      g_printf ("f(%s) = %d\n", symbol->name, level->left_prec);
      g_printf ("g(%s) = %d\n", symbol->name, level->right_prec);
      g_printf ("class(%s) = %s\n",
		symbol->name,
		level->symbol_class == SMIE_SYMBOL_CLASS_OPENER ? "opener"
		: level->symbol_class == SMIE_SYMBOL_CLASS_CLOSER ? "closer"
		: "neither");
    }
}
#endif

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
smie_prec2_to_grammar (struct smie_prec2_grammar_t *prec2,
		       struct smie_grammar_t *grammar,
		       GError **error)
{
  GHashTable *allocated = g_hash_table_new_full (smie_func_hash,
						 smie_func_equal,
						 g_free,
						 g_free);
  GHashTable *inequalities = g_hash_table_new_full (smie_func2_hash,
						    smie_func2_equal,
						    g_free,
						    NULL);
  GHashTable *equalities = g_hash_table_new_full (smie_func2_hash,
						  smie_func2_equal,
						  g_free,
						  NULL);
  GHashTable *transitive = g_hash_table_new (smie_func_hash,
					     smie_func_equal);
  GHashTable *assigned = g_hash_table_new (g_direct_hash, g_direct_equal);
  gint iteration_count;
  GHashTableIter iter;
  gpointer key, value;
  gboolean result = TRUE;

  /* Allocate all possible functions.  */
  g_hash_table_iter_init (&iter, grammar->pool->allocated);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      struct smie_symbol_t *symbol = key;
      if (symbol->type == SMIE_SYMBOL_TERMINAL
	  || symbol->type == SMIE_SYMBOL_TERMINAL_VARIABLE)
	{
	  struct smie_func_t *func;
	  func = g_new0 (struct smie_func_t, 1);
	  func->symbol = key;
	  func->type = SMIE_FUNC_F;
	  g_hash_table_insert (allocated, func, g_new0 (struct smie_level_t, 1));
	  func = g_new0 (struct smie_func_t, 1);
	  func->symbol = key;
	  func->type = SMIE_FUNC_G;
	  g_hash_table_insert (allocated, func, g_new0 (struct smie_level_t, 1));
	}
    }

  g_hash_table_iter_init (&iter, prec2->prec2);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      struct smie_prec2_t *p2 = key;
      enum smie_prec2_type_t p2_type = GPOINTER_TO_INT (value);
      struct smie_func_t func, *f, *g;

      func.symbol = p2->left;
      func.type = SMIE_FUNC_F;
      g_hash_table_lookup_extended (allocated,
				    &func,
				    (gpointer *) &f,
				    NULL);

      func.symbol = p2->right;
      func.type = SMIE_FUNC_G;
      g_hash_table_lookup_extended (allocated,
				    &func,
				    (gpointer *) &g,
				    NULL);

      switch (p2_type)
	{
	case SMIE_PREC2_LT:
	  g_hash_table_add (inequalities, smie_func2_alloc (f, g));
	  break;
	case SMIE_PREC2_GT:
	  g_hash_table_add (inequalities, smie_func2_alloc (g, f));
	  break;
	case SMIE_PREC2_EQ:
	  g_hash_table_add (equalities, smie_func2_alloc (f, g));
	  break;
	}
    }

  /* Replace all occurrences of composed functions.  */
  g_hash_table_iter_init (&iter, equalities);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      struct smie_func2_t *f2 = key;
      GHashTableIter iter2;
      gpointer key2;

      if (f2->f == f2->g)
	continue;

      g_hash_table_iter_init (&iter2, equalities);
      while (g_hash_table_iter_next (&iter2, &key2, NULL))
	{
	  struct smie_func2_t *other_f2 = key2;

	  if (other_f2 == f2)
	    continue;

	  if (f2->f == other_f2->g)
	    {
	      g_hash_table_insert (transitive,
				   (gpointer) other_f2->g,
				   (gpointer) f2->g);
	      other_f2->g = f2->g;
	    }
	  if (f2->f == other_f2->f)
	    {
	      g_hash_table_insert (transitive,
				   (gpointer) other_f2->f,
				   (gpointer) f2->g);
	      other_f2->f = f2->g;
	    }
	}

      g_hash_table_iter_init (&iter2, inequalities);
      while (g_hash_table_iter_next (&iter2, &key2, NULL))
	{
	  struct smie_func2_t *other_f2 = key2;
	  if (f2->f == other_f2->g)
	    other_f2->g = f2->g;
	  if (f2->f == other_f2->f)
	    other_f2->f = f2->g;
	}
    }

  /* Loop until inequalities is empty.  */
  iteration_count = 0;
  while (g_hash_table_size (inequalities) > 0)
    {
      GList *to_remove = NULL, *l;

      g_hash_table_iter_init (&iter, inequalities);
      while (g_hash_table_iter_next (&iter, &key, NULL))
	{
	  struct smie_func2_t *funcs = key;
	  GHashTableIter iter2;
	  gpointer key2;
	  gboolean found = FALSE;

	  g_hash_table_iter_init (&iter2, inequalities);
	  while (g_hash_table_iter_next (&iter2, &key2, NULL))
	    {
	      struct smie_func2_t *funcs2 = key2;
	      if (funcs->f == funcs2->g)
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
	  g_hash_table_unref (equalities);
	  g_hash_table_unref (inequalities);
	  result = FALSE;
	  goto out;
	}

      for (l = to_remove; l; l = l->next)
	{
	  struct smie_func_t *func = l->data;
	  if (!g_hash_table_contains (assigned, func))
	    {
	      g_hash_table_insert (assigned, func,
				   GINT_TO_POINTER (iteration_count));
	      iteration_count++;
	    }
	  g_hash_table_iter_init (&iter, inequalities);
	  while (g_hash_table_iter_next (&iter, &key, NULL))
	    {
	      struct smie_func2_t *funcs = key;
	      if (funcs->f == func)
		g_hash_table_iter_remove (&iter);
	    }
	}
      g_list_free (to_remove);
      iteration_count += 10;
    }
  g_hash_table_unref (inequalities);

  /* Propagate equality constraints back to their sources.  */
  g_hash_table_iter_init (&iter, equalities);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      struct smie_func2_t *funcs = key;
      if (g_hash_table_lookup_extended (assigned, funcs->f,
					NULL, &value))
	g_hash_table_insert (assigned, (gpointer) funcs->g, value);
      else if (g_hash_table_lookup_extended (assigned, funcs->g,
					     NULL, &value))
	g_hash_table_insert (assigned, (gpointer) funcs->f, value);
    }
  g_hash_table_iter_init (&iter, transitive);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      gpointer value1;
      if (!g_hash_table_contains (assigned, key)
	  && g_hash_table_lookup_extended (assigned, value, NULL, &value1))
	{
	  g_hash_table_insert (assigned, key, value1);
	}
    }
  g_hash_table_unref (transitive);

  /* Fill in the remaining functions.  */
  g_hash_table_iter_init (&iter, allocated);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      if (!g_hash_table_contains (assigned, key))
	g_hash_table_insert (assigned,
			     key,
			     GINT_TO_POINTER (iteration_count++));
    }

  g_hash_table_iter_init (&iter, assigned);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      struct smie_func_t *func = key;
      struct smie_level_t *level
	= g_hash_table_lookup (grammar->levels, (gpointer) func->symbol);
      if (!level)
	{
	  gpointer value;
	  level = g_new0 (struct smie_level_t, 1);
	  if (g_hash_table_lookup_extended (prec2->classes,
					    (gpointer) func->symbol,
					    NULL,
					    &value))
	    level->symbol_class = GPOINTER_TO_INT (value);
	  g_hash_table_insert (grammar->levels, (gpointer) func->symbol, level);
	}
      switch (func->type)
	{
	case SMIE_FUNC_F:
	  level->left_prec = GPOINTER_TO_INT (value);
	  break;
	case SMIE_FUNC_G:
	  level->right_prec = GPOINTER_TO_INT (value);
	  break;
	}
    }
  grammar->pairs = g_hash_table_ref (prec2->pairs);
  grammar->ends = g_hash_table_ref (prec2->ends);
 out:
  g_hash_table_unref (assigned);
  g_hash_table_unref (allocated);
  return result;
}

gboolean
smie_grammar_add_level (struct smie_grammar_t *grammar,
			const struct smie_symbol_t *symbol,
			gint left_prec,
			gint right_prec)
{
  struct smie_level_t *level = g_new0 (struct smie_level_t, 1);
  level->left_prec = left_prec;
  level->right_prec = right_prec;
  return g_hash_table_insert (grammar->levels, (gpointer) symbol, level);
}

enum smie_symbol_class_t
smie_grammar_get_symbol_class (struct smie_grammar_t *grammar,
			       const struct smie_symbol_t *symbol)
{
  struct smie_level_t *level
    = g_hash_table_lookup (grammar->levels, (gpointer) symbol);
  return level ? level->symbol_class : SMIE_SYMBOL_CLASS_NEITHER;
}

void
smie_grammar_set_symbol_class (struct smie_grammar_t *grammar,
			       const struct smie_symbol_t *symbol,
			       enum smie_symbol_class_t symbol_class)
{
  struct smie_level_t *level
    = g_hash_table_lookup (grammar->levels, (gpointer) symbol);
  g_return_if_fail (level);
  level->symbol_class = symbol_class;
}

gboolean
smie_grammar_has_pair (struct smie_grammar_t *grammar,
		       const struct smie_symbol_t *opener_symbol,
		       const struct smie_symbol_t *closer_symbol)
{
  struct smie_prec2_t p2;
  if (!grammar->pairs)
    return FALSE;
  else
    {
      p2.left = opener_symbol;
      p2.right = closer_symbol;
      return g_hash_table_contains (grammar->pairs, &p2);
    }
}

gboolean
smie_grammar_is_pair_end (struct smie_grammar_t *grammar,
			  const struct smie_symbol_t *closer_symbol)
{
  return grammar->ends
    && g_hash_table_contains (grammar->ends, (gpointer) closer_symbol);
}

gboolean
smie_grammar_is_keyword (struct smie_grammar_t *grammar,
			 const struct smie_symbol_t *symbol)
{
  return g_hash_table_contains (grammar->levels, (gpointer) symbol);
}

gint
smie_grammar_get_left_prec (struct smie_grammar_t *grammar,
			    const smie_symbol_t *symbol)
{
  struct smie_level_t *level
    = g_hash_table_lookup (grammar->levels, (gpointer) symbol);
  g_return_val_if_fail (level, NULL);
  return level->left_prec;
}

gint
smie_grammar_get_right_prec (struct smie_grammar_t *grammar,
			     const struct smie_symbol_t *symbol)
{
  struct smie_level_t *level
    = g_hash_table_lookup (grammar->levels, (gpointer) symbol);
  g_return_val_if_fail (level, NULL);
  return level->right_prec;
}

typedef gboolean (*smie_select_function_t) (struct smie_level_t *, gint *);

static gboolean
smie_select_left (struct smie_level_t *level, gint *precp)
{
  *precp = level->left_prec;
  return level->symbol_class == SMIE_SYMBOL_CLASS_OPENER;
}

static gboolean
smie_select_right (struct smie_level_t *level, gint *precp)
{
  *precp = level->right_prec;
  return level->symbol_class == SMIE_SYMBOL_CLASS_CLOSER;
}

static gboolean
smie_is_associative (struct smie_level_t *level)
{
  gint prec_value, prec_value2;
  smie_select_left (level, &prec_value);
  smie_select_right (level, &prec_value2);
  return prec_value == prec_value2;
}

static gboolean
smie_next_sexp (struct smie_grammar_t *grammar,
		smie_next_token_function_t next_token_func,
		smie_read_token_function_t read_token_func,
		gpointer context,
		smie_select_function_t op_forward,
		smie_select_function_t op_backward)
{
  GList *stack = NULL;

  /* Place the cursor on the token.  */
  if (!read_token_func (context, NULL) && !next_token_func (context, TRUE))
    return FALSE;

  do
    {
      struct smie_symbol_t symbol;
      struct smie_level_t *level;
      gint prec_value;
      gchar *token;

      if (!read_token_func (context, &token))
	goto out;

      symbol.name = token;
      symbol.type = SMIE_SYMBOL_TERMINAL;
      level = g_hash_table_lookup (grammar->levels, &symbol);
      g_free (token);
      if (!level)
	continue;
      else if (op_backward (level, &prec_value))
	stack = g_list_prepend (stack, level);
      else
	{
	  while (stack)
	    {
	      struct smie_level_t *level2 = stack->data;
	      gint prec_value2;
	      op_forward (level, &prec_value);
	      op_backward (level2, &prec_value2);
	      if (prec_value >= prec_value2)
		break;
	      stack = g_list_delete_link (stack, stack);
	    }
	  if (!stack)
	    return TRUE;
	  else
	    {
	      struct smie_level_t *level2 = stack->data;
	      gint prec_value2;
	      op_forward (level, &prec_value);
	      op_backward (level2, &prec_value2);
	      if (prec_value == prec_value2)
		stack = g_list_delete_link (stack, stack);
	      if (stack)
		{
		  if (!op_forward (level, &prec_value))
		    stack = g_list_prepend (stack, level);
		}
	      else if (op_forward (level, &prec_value))
		goto out;
	      else if (!smie_is_associative (level))
		stack = g_list_prepend (NULL, level);
	      else if (smie_is_associative (level2))
		return TRUE;
	      else
		stack = g_list_prepend (NULL, level2);
	    }
	}
    }
  while (next_token_func (context, TRUE));

 out:
  g_list_free (stack);
  return FALSE;
}

gboolean
smie_forward_sexp (struct smie_grammar_t *grammar,
		   smie_next_token_function_t next_token_func,
		   smie_read_token_function_t read_token_func,
		   gpointer context)
{
  return smie_next_sexp (grammar,
			 next_token_func,
			 read_token_func,
			 context,
			 smie_select_right,
			 smie_select_left);
}

gboolean
smie_backward_sexp (struct smie_grammar_t *grammar,
		    smie_next_token_function_t next_token_func,
		    smie_read_token_function_t read_token_func,
		    gpointer context)
{
  return smie_next_sexp (grammar,
			 next_token_func,
			 read_token_func,
			 context,
			 smie_select_left,
			 smie_select_right);
}

GQuark
smie_error_quark (void)
{
  return g_quark_from_static_string ("smie-error-quark");
}
