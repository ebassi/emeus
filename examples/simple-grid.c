#include <gtk/gtk.h>
#include "emeus.h"

#define EMEUS_TYPE_TEST_APPLICATION_WINDOW (emeus_test_application_window_get_type())

G_DECLARE_FINAL_TYPE (EmeusTestApplicationWindow,
                      emeus_test_application_window,
                      EMEUS, TEST_APPLICATION_WINDOW,
                      GtkApplicationWindow)

#define EMEUS_TYPE_TEST_APPLICATION (emeus_test_application_get_type())

G_DECLARE_FINAL_TYPE (EmeusTestApplication,
                      emeus_test_application,
                      EMEUS, TEST_APPLICATION,
                      GtkApplication)

struct _EmeusTestApplicationWindow
{
  GtkApplicationWindow parent_instance;

  GtkWidget *main_box;
  GtkWidget *layout;
  GtkWidget *quit_button;
};

struct _EmeusTestApplication
{
  GtkApplication parent_instance;

  GtkWidget *window;
};

G_DEFINE_TYPE (EmeusTestApplicationWindow,
               emeus_test_application_window,
               GTK_TYPE_APPLICATION_WINDOW)

static void
emeus_test_application_window_class_init (EmeusTestApplicationWindowClass *klass)
{
}

static void
emeus_test_application_window_init (EmeusTestApplicationWindow *self)
{
  GtkWidget *box, *layout, *button;

  gtk_window_set_default_size (GTK_WINDOW (self), 400, 500);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (self), box);
  gtk_widget_show (box);
  self->main_box = box;

  layout = emeus_constraint_layout_new ();
  gtk_widget_set_hexpand (layout, TRUE);
  gtk_widget_set_vexpand (layout, TRUE);
  gtk_container_add (GTK_CONTAINER (box), layout);
  gtk_widget_show (layout);
  self->layout = layout;

  button = gtk_button_new_with_label ("Quit");
  gtk_widget_set_hexpand (layout, TRUE);
  gtk_container_add (GTK_CONTAINER (box), button);
  gtk_widget_show (button);
  self->quit_button = button;

  g_signal_connect_swapped (self->quit_button,
                            "clicked", G_CALLBACK (gtk_widget_destroy),
                            self);
}

static GtkWidget *
emeus_test_application_window_new (EmeusTestApplication *app)
{
  return g_object_new (EMEUS_TYPE_TEST_APPLICATION_WINDOW,
                       "application", app,
                       NULL);
}

G_DEFINE_TYPE (EmeusTestApplication,
               emeus_test_application,
               GTK_TYPE_APPLICATION)

static void
emeus_test_application_activate (GApplication *application)
{
  EmeusTestApplication *self = EMEUS_TEST_APPLICATION (application);

  if (self->window == NULL)
    self->window = emeus_test_application_window_new (self);

  gtk_widget_show (self->window);

  gtk_window_present (GTK_WINDOW (self->window));
}

static void
emeus_test_application_class_init (EmeusTestApplicationClass *klass)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

  application_class->activate = emeus_test_application_activate;
}

static void
emeus_test_application_init (EmeusTestApplication *self)
{
}

static GApplication *
emeus_test_application_new (void)
{
  return g_object_new (EMEUS_TYPE_TEST_APPLICATION,
                       "application-id", "io.github.ebassi.EmeusTestApplication",
                       NULL);
}

int
main (int argc, char *argv[])
{
  return g_application_run (emeus_test_application_new (), argc, argv);
}
