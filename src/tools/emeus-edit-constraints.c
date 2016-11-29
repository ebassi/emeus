/* emeus-edit-constraints.c
 *
 * Copyright (C) 2016 Endless
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <libintl.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <emeus.h>

#include "emeus-vfl-parser-private.h"

static int last_view_id;

G_DECLARE_FINAL_TYPE (ViewListRow, view_list_row, VIEW, LIST_ROW, GtkListBoxRow)

struct _ViewListRow
{
  GtkListBoxRow parent_instance;

  GtkWidget *name_entry;
  GtkWidget *color_chooser;
  GtkWidget *view_widget;
};

G_DEFINE_TYPE (ViewListRow, view_list_row, GTK_TYPE_LIST_BOX_ROW)

static gboolean
view_widget__draw (GtkWidget   *area,
                   cairo_t     *cr,
                   ViewListRow *row)
{
  GdkRGBA color;

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (row->color_chooser), &color);

  gdk_cairo_set_source_rgba (cr, &color);
  cairo_rectangle (cr, 0, 0,
                   gtk_widget_get_allocated_width (area),
                   gtk_widget_get_allocated_height (area));
  cairo_fill (cr);

  return TRUE;
}

static void
view_list_row_dispose (GObject *gobject)
{
  ViewListRow *self = VIEW_LIST_ROW (gobject);

  g_clear_object (&self->view_widget);

  G_OBJECT_CLASS (view_list_row_parent_class)->dispose (gobject);
}

static void
view_list_row_class_init (ViewListRowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = view_list_row_dispose;

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/endlessm/EmeusEditor/view-list-row.ui");

  gtk_widget_class_bind_template_child (widget_class, ViewListRow, name_entry);
  gtk_widget_class_bind_template_child (widget_class, ViewListRow, color_chooser);
}

static void
view_list_row_init (ViewListRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  char *tmp_name = g_strdup_printf ("view%d", last_view_id++);
  gtk_entry_set_text (GTK_ENTRY (self->name_entry), tmp_name);
  g_free (tmp_name);

  self->view_widget = gtk_drawing_area_new ();
  g_object_ref_sink (self->view_widget);

  g_signal_connect (self->view_widget, "draw", G_CALLBACK (view_widget__draw), self);

  GdkRGBA color = {
    .red = g_random_double_range (0.2, 0.8),
    .green = g_random_double_range (0.2, 0.8),
    .blue = g_random_double_range (0.2, 0.8),
    .alpha = 1.0
  };

  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (self->color_chooser), &color);
}

#if 0
G_DECLARE_FINAL_TYPE (MetricListRow, metric_list_row, METRIC, LIST_ROW, GtkListBoxRow)

struct _MetricListRow
{
  GtkListBoxRow parent_instance;

  GtkWidget *name_entry;
  GtkWidget *spin_button;
};

G_DEFINE_TYPE (MetricListRow, metric_list_row, GTK_TYPE_LIST_BOX_ROW)

static void
metric_list_row_class_init (MetricListRowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  gtk_widget_class_set_template_from_resource (widget_class, "/com/endlessm/EmeusEditor/metric-list-row.ui");
  gtk_widget_class_bind_template_child (widget_class, MetricListRow, name_entry);
  gtk_widget_class_bind_template_child (widget_class, MetricListRow, spin_button);
}

static void
metric_list_row_init (MetricListRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
#endif

G_DECLARE_FINAL_TYPE (EditorApplicationWindow, editor_application_window, EDITOR, APPLICATION_WINDOW, GtkApplicationWindow)

struct _EditorApplicationWindow
{
  GtkApplicationWindow parent_instance;

  VflParser *vfl_parser;

  GtkWidget *lists_switcher;
  GtkWidget *lists_stack;
  GtkWidget *views_listbox;
  GtkWidget *metrics_listbox;
  GtkWidget *text_switcher;
  GtkWidget *text_stack;
  GtkWidget *vfl_text_area;
  GtkWidget *log_text_area;
  GtkWidget *layout_box;

  GHashTable *views;
  GHashTable *metrics;
};

G_DEFINE_TYPE (EditorApplicationWindow, editor_application_window, GTK_TYPE_APPLICATION_WINDOW)

static void
add_view_button__clicked (EditorApplicationWindow *self)
{
  const char *current_list = gtk_stack_get_visible_child_name (GTK_STACK (self->lists_stack));

  if (g_strcmp0 (current_list, "views") == 0)
    {
      ViewListRow *row = g_object_new (view_list_row_get_type (), NULL);

      gtk_list_box_insert (GTK_LIST_BOX (self->views_listbox), GTK_WIDGET (row), -1);

      const char *view_name = gtk_entry_get_text (GTK_ENTRY (row->name_entry));
      g_hash_table_insert (self->views, g_strdup (view_name), row->view_widget);

      emeus_constraint_layout_pack (EMEUS_CONSTRAINT_LAYOUT (self->layout_box),
                                    row->view_widget,
                                    view_name, NULL);

      gtk_widget_show (row->view_widget);
      g_object_ref (row->view_widget);

      g_debug ("*** Added view '%s' (widget: %p)", view_name, row->view_widget);
    }

  if (g_strcmp0 (current_list, "metrics") == 0)
    {

    }
}

static void
remove_view_button__clicked (EditorApplicationWindow *self)
{

}

static void
clear_view_button__clicked (EditorApplicationWindow *self)
{

}

static void
log_text_area_add_message (EditorApplicationWindow *self,
                           const char              *message,
                           gboolean                 is_error)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->log_text_area));

  GtkTextIter end_iter;
  gtk_text_buffer_get_end_iter (buffer, &end_iter);

  if (is_error)
    gtk_text_buffer_insert_with_tags_by_name (buffer, &end_iter, message, -1, "error", NULL);
  else
    gtk_text_buffer_insert (buffer, &end_iter, message, -1);

  if (is_error)
    {
      GtkWidget *log_view = gtk_stack_get_child_by_name (GTK_STACK (self->text_stack), "log");
      gtk_container_child_set (GTK_CONTAINER (self->text_stack), log_view,
                               "needs-attention", TRUE,
                               NULL);
    }
}

static void
text_stack__notify__visible_child_name (EditorApplicationWindow *self)
{
  const char *page_name = gtk_stack_get_visible_child_name (GTK_STACK (self->text_stack));

  if (g_strcmp0 (page_name, "log") == 0)
    {
      GtkWidget *log_view = gtk_stack_get_child_by_name (GTK_STACK (self->text_stack), "log");
      gtk_container_child_set (GTK_CONTAINER (self->text_stack), log_view,
                               "needs-attention", FALSE,
                               NULL);
    }
}

static void
vfl_text_area__changed (EditorApplicationWindow *self,
                        GtkTextBuffer           *buffer)
{
  GtkTextIter start_iter, end_iter;

  gtk_text_buffer_get_start_iter (buffer, &start_iter);
  gtk_text_buffer_get_end_iter (buffer, &end_iter);

  char *contents = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);
  if (contents == NULL || *contents == '\0')
    return;

  char **lines = g_strsplit (contents, "\n", -1);

  g_free (contents);

  gboolean had_error = FALSE;

  vfl_parser_set_views (self->vfl_parser, self->views);

  for (int i = 0; lines[i] != 0; i += 1)
    {
      const char *line = lines[i];
      GError *error = NULL;

      vfl_parser_parse_line (self->vfl_parser, line, -1, &error);
      if (error != NULL)
        {
          int offset = vfl_parser_get_error_offset (self->vfl_parser);
          int range = vfl_parser_get_error_range (self->vfl_parser);

          char *squiggly = NULL;

          if (range > 0)
            {
              squiggly = g_new0 (char, range + 1);
              for (int s = 0; s < range - 1; s++)
                squiggly[s] = '~';
              squiggly[range] = '\0';
            }

          char *error_msg =
            g_strdup_printf ("ERROR: line %d: %s\n"
                             "%s\n"
                             "%*s^%s\n",
                             i + 1, error->message,
                             line,
                             offset, " ", squiggly != NULL ? squiggly : "");

          g_free (squiggly);
          g_error_free (error);

          log_text_area_add_message (self, error_msg, TRUE);
          g_free (error_msg);

          had_error = TRUE;

          break;
        }
    }

  if (had_error)
    {
      g_strfreev (lines);
      return;
    }

  log_text_area_add_message (self, _("Visual format parsed successfully\n"), FALSE);

  emeus_constraint_layout_clear_constraints (EMEUS_CONSTRAINT_LAYOUT (self->layout_box));

  GList *constraints = emeus_create_constraints_from_description ((const char * const *) lines,
                                                                  g_strv_length (lines),
                                                                  -1, -1,
                                                                  self->views,
                                                                  NULL);

  g_debug ("*** Generated %d constraints from %d lines", g_list_length (constraints), g_strv_length (lines));

  for (GList *l = constraints; l != NULL; l = l->next)
    emeus_constraint_layout_add_constraint (EMEUS_CONSTRAINT_LAYOUT (self->layout_box), l->data);

  g_list_free (constraints);

  g_strfreev (lines);

  gtk_widget_queue_resize (GTK_WIDGET (self->layout_box));
}

static void
editor_application_window_finalize (GObject *gobject)
{
  EditorApplicationWindow *self = (EditorApplicationWindow *) gobject;

  vfl_parser_free (self->vfl_parser);

  g_clear_pointer (&self->views, g_hash_table_unref);

  G_OBJECT_CLASS (editor_application_window_parent_class)->finalize (gobject);
}

static void
editor_application_window_class_init (EditorApplicationWindowClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = editor_application_window_finalize;

  g_type_ensure (EMEUS_TYPE_CONSTRAINT_LAYOUT);

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  gtk_widget_class_set_template_from_resource (widget_class, "/com/endlessm/EmeusEditor/editor-window.ui");
  gtk_widget_class_bind_template_child (widget_class, EditorApplicationWindow, lists_switcher);
  gtk_widget_class_bind_template_child (widget_class, EditorApplicationWindow, lists_stack);
  gtk_widget_class_bind_template_child (widget_class, EditorApplicationWindow, views_listbox);
  gtk_widget_class_bind_template_child (widget_class, EditorApplicationWindow, metrics_listbox);
  gtk_widget_class_bind_template_child (widget_class, EditorApplicationWindow, vfl_text_area);
  gtk_widget_class_bind_template_child (widget_class, EditorApplicationWindow, log_text_area);
  gtk_widget_class_bind_template_child (widget_class, EditorApplicationWindow, text_switcher);
  gtk_widget_class_bind_template_child (widget_class, EditorApplicationWindow, text_stack);
  gtk_widget_class_bind_template_child (widget_class, EditorApplicationWindow, layout_box);

  gtk_widget_class_bind_template_callback (widget_class, add_view_button__clicked);
  gtk_widget_class_bind_template_callback (widget_class, remove_view_button__clicked);
  gtk_widget_class_bind_template_callback (widget_class, clear_view_button__clicked);
  gtk_widget_class_bind_template_callback (widget_class, text_stack__notify__visible_child_name);
}

static void
editor_application_window_init (EditorApplicationWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->views = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  self->vfl_parser = vfl_parser_new (-1, -1, self->views, NULL);

  g_signal_connect_swapped (gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->vfl_text_area)),
                            "changed", G_CALLBACK (vfl_text_area__changed),
                            self);

  GtkTextBuffer *log_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->log_text_area));
  gtk_text_buffer_create_tag (log_buffer, "error",
                              "foreground-rgba", &(GdkRGBA) { 1.0, 0.0, 0.0, 1.0 },
                              NULL);
}

G_DECLARE_FINAL_TYPE (EditorApplication, editor_application, EDITOR, APPLICATION, GtkApplication)

struct _EditorApplication
{
  GtkApplication parent_instance;
};

G_DEFINE_TYPE (EditorApplication, editor_application, GTK_TYPE_APPLICATION)

static void
quit_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  g_application_quit (G_APPLICATION (user_data));
}

static GActionEntry app_entries[] =
{
  { "quit", quit_activated, NULL, NULL, NULL }
};

static void
editor_application_startup (GApplication *application)
{
  const char *quit_accels[] = { "<Ctrl>Q", NULL };

  G_APPLICATION_CLASS (editor_application_parent_class)->startup (application);

  g_action_map_add_action_entries (G_ACTION_MAP (application),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   application);
  gtk_application_set_accels_for_action (GTK_APPLICATION (application),
                                         "app.quit",
                                         quit_accels);

  GtkBuilder *builder = gtk_builder_new_from_resource ("/com/endlessm/EmeusEditor/editor-menu.ui");
  GMenuModel *app_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "appmenu"));
  gtk_application_set_app_menu (GTK_APPLICATION (application), app_menu);
  g_object_unref (builder);
}

static void
editor_application_activate (GApplication *application)
{
  GtkWidget *widget = g_object_new (editor_application_window_get_type (),
                                    "application", application,
                                    NULL);

  gtk_widget_show (widget);
}

static void
editor_application_class_init (EditorApplicationClass *klass)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

  application_class->startup = editor_application_startup;
  application_class->activate = editor_application_activate;
}

static void
editor_application_init (EditorApplication *self)
{

}

static GApplication *
editor_application_new (void)
{
  return g_object_new (editor_application_get_type (),
                       "application-id", "com.endlessm.EmeusEditor",
                       NULL);
}

int
main (int argc, char *argv[])
{
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  return g_application_run (editor_application_new (), argc, argv);
}
