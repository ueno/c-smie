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
#ifndef __SMIE_PRIVATE_H__
#define __SMIE_PRIVATE_H__ 1

#include "smie-grammar.h"

G_BEGIN_DECLS

struct _smie_symbol_pool_t
{
  volatile gint ref_count;
  GHashTable *allocated;
};

struct _smie_symbol_t
{
  gchar *name;
  smie_symbol_type_t type;
};

struct smie_rule_t
{
  GList *symbols;
};

struct smie_rule_list_t
{
  GList *rules;
};

struct _smie_bnf_grammar_t
{
  smie_symbol_pool_t *pool;
  GHashTable *rules;
};

struct smie_prec2_t
{
  const smie_symbol_t *left;
  const smie_symbol_t *right;
};

struct _smie_prec2_grammar_t
{
  smie_symbol_pool_t *pool;
  GHashTable *prec2;
  GHashTable *classes;
  GHashTable *pairs;
  GHashTable *ends;
};

struct smie_prec_t
{
  smie_prec_type_t type;
  GList *op;
};

struct _smie_precs_grammar_t
{
  smie_symbol_pool_t *pool;
  GList *precs;
};

enum smie_func_type_t
  {
    SMIE_FUNC_F,
    SMIE_FUNC_G
  };

struct smie_func2_t
{
  const struct smie_func_t *f;
  const struct smie_func_t *g;
};

struct smie_func_t
{
  const smie_symbol_t *symbol;
  enum smie_func_type_t type;
};

struct smie_level_t
{
  gint left_prec;
  gint right_prec;
  smie_symbol_class_t symbol_class;
};

struct _smie_grammar_t
{
  smie_symbol_pool_t *pool;
  GHashTable *levels;
  GHashTable *pairs;
  GHashTable *ends;
};

struct smie_grammar_parser_context_t
{
  smie_bnf_grammar_t *bnf;
  smie_precs_grammar_t *precs;
  GList *resolvers;
  const gchar *input;
};

G_END_DECLS

#endif	/* __SMIE_PRIVATE_H__ */
