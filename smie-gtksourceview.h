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
#ifndef __SMIE_GTKSOURCEVIEW_H__
#define __SMIE_GTKSOURCEVIEW_H__ 1

#include <smie.h>
#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

typedef struct smie_gtk_source_buffer_context_t smie_gtk_source_buffer_context_t;

struct smie_gtk_source_buffer_context_t
{
  GtkSourceBuffer *buffer;
  GtkTextIter iter;
  GList *stack;
};

smie_cursor_functions_t smie_gtk_source_buffer_cursor_functions;

G_END_DECLS

#endif	/* __SMIE_GTKSOURCEVIEW_H__ */
