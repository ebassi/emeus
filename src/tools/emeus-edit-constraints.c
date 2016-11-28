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

G_DECLARE_FINAL_TYPE (EditorApplicationWindow, editor_application_window, EDITOR, APPLICATION_WINDOW, GtkApplicationWindow)

struct _EditorApplicationWindow
{
  GtkApplicationWindow parent_instance;

  VflParser *vfl_parser;

  GtkWidget *lists_switcher;
  GtkWidget *lists_stack;
  GtkWidget *views_listbox;
  GtkWidget *metrics_listbox;
  GtkWidget *vfl_text_area;
  GtkWidget *layout_box;
};

G_DEFINE_TYPE (EditorApplicationWindow, editor_application_window, GTK_TYPE_APPLICATION_WINDOW)

static void
editor_application_window_finalize (GObject *gobject)
{
  EditorApplicationWindow *self = (EditorApplicationWindow *) gobject;

  vfl_parser_free (self->vfl_parser);

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
  gtk_widget_class_bind_template_child (widget_class, EditorApplicationWindow, layout_box);
}

static void
editor_application_window_init (EditorApplicationWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->vfl_parser = vfl_parser_new (-1, -1, NULL, NULL);
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
