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
#include <gtksourceview/gtksource.h>
#include <smie-gtksourceview.h>
#include <stdlib.h>
#include <string.h>

struct _EditorApplication
{
  GtkApplication parent;
};

struct _EditorApplicationClass
{
  GtkApplicationClass parent;
};

struct _EditorApplicationWindow
{
  GtkApplicationWindow parent;
  GtkSourceView *view;
  GtkSourceFile *file;
  GtkSourceBuffer *buffer;
  smie_indenter_t *indenter;
};

struct _EditorApplicationWindowClass
{
  GtkApplicationWindowClass parent;
};

#define EDITOR_TYPE_APPLICATION editor_application_get_type ()

G_DECLARE_FINAL_TYPE (EditorApplication,
		      editor_application,
		      EDITOR,
		      APPLICATION,
		      GtkApplication);

#define EDITOR_TYPE_APPLICATION_WINDOW editor_application_window_get_type ()

G_DECLARE_FINAL_TYPE (EditorApplicationWindow,
		      editor_application_window,
		      EDITOR,
		      APPLICATION_WINDOW,
		      GtkApplicationWindow);

G_DEFINE_TYPE (EditorApplication, editor_application, GTK_TYPE_APPLICATION);

G_DEFINE_TYPE (EditorApplicationWindow, editor_application_window,
               GTK_TYPE_APPLICATION_WINDOW);

static const gchar *indent_filename;

static void
remove_all_marks (GtkSourceBuffer *buffer)
{
  GtkTextIter start;
  GtkTextIter end;

  gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer), &start, &end);
  gtk_source_buffer_remove_source_marks (buffer, &start, &end, NULL);
}

static void
set_indenter (EditorApplicationWindow *window, const gchar *filename)
{
  GMappedFile *mapped_file;
  smie_symbol_pool_t *pool;
  smie_bnf_grammar_t *bnf;
  smie_prec2_grammar_t *prec2;
  smie_grammar_t *grammar;
  const gchar *contents;
  GError *error;

  error = NULL;
  mapped_file = g_mapped_file_new (filename, FALSE, &error);
  if (!mapped_file)
    {
      g_warning ("Error while loading the file: %s", error->message);
      g_error_free (error);
      return;
    }

  pool = smie_symbol_pool_alloc ();
  error = NULL;
  bnf = smie_bnf_grammar_alloc (pool);
  contents = (const gchar *) g_mapped_file_get_contents (mapped_file);
  if (!smie_bnf_grammar_load (bnf, contents, &error))
    {
      g_warning ("Error while loading the grammar: %s", error->message);
      g_error_free (error);
      smie_symbol_pool_unref (pool);
      smie_bnf_grammar_free (bnf);
      g_mapped_file_unref (mapped_file);
      return;
    }
  g_mapped_file_unref (mapped_file);

  error = NULL;
  prec2 = smie_prec2_grammar_alloc (pool);
  if (!smie_bnf_to_prec2 (bnf, prec2, NULL, &error))
    {
      g_warning ("Error while converting BNF to prec2: %s", error->message);
      g_error_free (error);
      smie_symbol_pool_unref (pool);
      return;
    }
  smie_bnf_grammar_free (bnf);

  error = NULL;
  grammar = smie_grammar_alloc (pool);
  if (!smie_prec2_to_grammar (prec2, grammar, &error))
    {
      g_warning ("Error while converting prec2 to grammar: %s", error->message);
      g_error_free (error);
      smie_symbol_pool_unref (pool);
      return;
    }
  smie_prec2_grammar_free (prec2);

  window->indenter
    = smie_indenter_new (grammar,
			 2,
			 &smie_gtk_source_buffer_cursor_functions);
}

static void
load_ready (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GtkSourceFileLoader *loader = GTK_SOURCE_FILE_LOADER (source_object);
  EditorApplicationWindow *window = EDITOR_APPLICATION_WINDOW (user_data);
  GtkTextIter iter;
  GFile *location;
  GtkSourceLanguageManager *manager;
  GtkSourceLanguage *language;
  gchar *filename;
  GError *error = NULL;

  if (!gtk_source_file_loader_load_finish (loader, res, &error))
    {
      g_warning ("Error while loading the file: %s", error->message);
      g_error_free (error);
      g_clear_object (&window->file);
      goto out;
    }

  gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (window->buffer), &iter);
  gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (window->buffer), &iter);

  location = gtk_source_file_loader_get_location (loader);
  filename = g_file_get_path (location);
  manager = gtk_source_language_manager_get_default ();
  language = gtk_source_language_manager_guess_language (manager,
							 filename,
							 NULL);
  g_free (filename);
  if (language)
    gtk_source_buffer_set_language (window->buffer, language);

  if (indent_filename)
    set_indenter (window, indent_filename);

 out:
  g_object_unref (loader);
  gtk_window_present (GTK_WINDOW (window));
}

static void
editor_application_open (GApplication *application,
			 GFile **files,
			 gint n_files,
			 const gchar *hint)
{
  gint i;
  for (i = 0; i < n_files; i++)
    {
      EditorApplicationWindow *window;
      GFile *location = files[i];
      GtkSourceFileLoader *loader;

      window = g_object_new (EDITOR_TYPE_APPLICATION_WINDOW,
			     "application", application,
			     NULL);
      g_clear_object (&window->file);
      window->file = gtk_source_file_new ();

      gtk_source_file_set_location (window->file, location);
      loader = gtk_source_file_loader_new (window->buffer, window->file);
      remove_all_marks (window->buffer);

      gtk_source_file_loader_load_async (loader,
					 G_PRIORITY_DEFAULT,
					 NULL,
					 NULL, NULL, NULL,
					 (GAsyncReadyCallback) load_ready,
					 window);
    }
}

static void
editor_application_activate (GApplication *application)
{
  EditorApplicationWindow *window;

  window = g_object_new (EDITOR_TYPE_APPLICATION_WINDOW,
			 "application", application,
			 NULL);

  if (indent_filename)
    set_indenter (window, indent_filename);

  gtk_window_present (GTK_WINDOW (window));
}

static gboolean
editor_application_window_key_press_event (GtkWidget *widget,
					   GdkEventKey *event)
{
  EditorApplicationWindow *window = EDITOR_APPLICATION_WINDOW (widget);
  if (event->keyval == GDK_KEY_KP_Tab || event->keyval == GDK_KEY_Tab)
    {
      GtkTextMark *mark;
      smie_gtk_source_buffer_context_t context;
      gint indent, current_indent;
      GtkTextIter iter, start_iter, end_iter;

      memset (&context, 0, sizeof (smie_gtk_source_buffer_context_t));
      context.buffer = window->buffer;
      mark = gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (window->buffer));
      gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (window->buffer),
					&iter,
					mark);
      gtk_text_iter_assign (&context.iter, &iter);
      indent = smie_indenter_calculate (window->indenter, &context);
      g_printf ("%d\n", indent);
      g_assert (indent >= 0);

      /* Point START_ITER to the beginning of the line.  */
      gtk_text_iter_assign (&start_iter, &iter);
      while (!gtk_text_iter_is_start (&start_iter)
	     && !gtk_text_iter_starts_line (&start_iter)
	     && gtk_text_iter_backward_char (&start_iter))
	;

      /* Point END_ITER to the end of the indent and count the offset.  */
      gtk_text_iter_assign (&end_iter, &start_iter);
      current_indent = 0;
      while (!gtk_text_iter_is_end (&end_iter)
	     && !gtk_text_iter_ends_line (&end_iter)
	     && g_unichar_isspace (gtk_text_iter_get_char (&end_iter))
	     && gtk_text_iter_forward_char (&end_iter))
	current_indent++;

      /* Replace the current indent if it doesn't match the computed one.  */
      if (indent < current_indent)
	{
	  while (indent < current_indent)
	    {
	      gtk_text_iter_backward_char (&end_iter);
	      indent--;
	    }
	  gtk_text_buffer_delete (GTK_TEXT_BUFFER (window->buffer),
				  &start_iter, &end_iter);
	}
      else if (indent > current_indent)
	{
	  gchar *text = g_new0 (gchar, indent - current_indent);
	  memset (text, ' ', indent * sizeof (gchar));
	  gtk_text_buffer_insert (GTK_TEXT_BUFFER (window->buffer),
				  &start_iter,
				  text,
				  indent - current_indent);
	  g_free (text);
	}
      return TRUE;
    }
  return GTK_WIDGET_CLASS (editor_application_window_parent_class)->key_press_event (widget, event);
}

static void
editor_application_class_init (EditorApplicationClass *klass)
{
  G_APPLICATION_CLASS (klass)->activate = editor_application_activate;
  G_APPLICATION_CLASS (klass)->open = editor_application_open;
}

static void
editor_application_init (EditorApplication *application)
{
}

static void
editor_application_window_class_init (EditorApplicationWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  gtk_widget_class_set_template_from_resource (widget_class,
					       "/org/du_a/Editor/editor.ui");
  gtk_widget_class_bind_template_child (widget_class,
					EditorApplicationWindow,
					view);
  widget_class->key_press_event = editor_application_window_key_press_event;
}

static void
editor_application_window_init (EditorApplicationWindow *window)
{
  GtkTextView *view;
  gtk_widget_init_template (GTK_WIDGET (window));

  view = GTK_TEXT_VIEW (window->view);
  window->buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (view));
}

static GOptionEntry entries[] =
  {
    { "indent", 'i', 0, G_OPTION_ARG_STRING, &indent_filename,
      "Use FILE as indentation rule", "FILE" },
    { NULL }
  };

int
main (int argc, char **argv)
{
  GApplication *application;
  GOptionContext *context;
  GError *error = NULL;
  int status;

  context = g_option_context_new ("- editor");
  g_option_context_add_main_entries (context, entries, "editor");
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("option parsing failed: %s\n", error->message);
      return EXIT_FAILURE;
    }

  application = g_object_new (EDITOR_TYPE_APPLICATION,
			      "application-id", "org.du_a.Editor",
			      "flags", G_APPLICATION_HANDLES_OPEN,
			      NULL);
  status = g_application_run (application, argc, argv);
  g_object_unref (application);

  return status;
}
