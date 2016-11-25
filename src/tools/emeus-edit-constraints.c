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

G_DECLARE_FINAL_TYPE (EditorApplicationWindow, editor_application_window, EDITOR, APPLICATION_WINDOW, GtkApplicationWindow)

struct _EditorApplicationWindow
{
  GtkApplicationWindow parent_instance;
};

G_DEFINE_TYPE (EditorApplicationWindow, editor_application_window, GTK_TYPE_APPLICATION_WINDOW)

static void
editor_application_window_class_init (EditorApplicationWindowClass *klass)
{

}

static void
editor_application_window_init (EditorApplicationWindow *self)
{
  gtk_window_set_title (GTK_WINDOW (self), "Constraint Editor");
}

G_DECLARE_FINAL_TYPE (EditorApplication, editor_application, EDITOR, APPLICATION, GtkApplication)

struct _EditorApplication
{
  GtkApplication parent_instance;
};

G_DEFINE_TYPE (EditorApplication, editor_application, GTK_TYPE_APPLICATION)

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
