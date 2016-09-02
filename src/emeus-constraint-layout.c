#include "config.h"

#include "emeus-constraint-layout.h"

typedef struct {
  GtkWidget *widget;

  GList *constraints;
} LayoutChild;

struct _EmeusConstraintLayout
{
  GtkContainer parent_instance;

  GSequence *children;
};

G_DEFINE_TYPE (EmeusConstraintLayout, emeus_constraint_layout, GTK_TYPE_CONTAINER)

static void
emeus_constraint_layout_class_init (EmeusConstraintLayoutClass *klass)
{
}

static void
emeus_constraint_layout_init (EmeusConstraintLayout *self)
{
}

GtkWidget *
emeus_constraint_layout_new (void)
{
  return g_object_new (EMEUS_TYPE_CONSTRAINT_LAYOUT, NULL);
}

void
emeus_constraint_layout_pack (EmeusConstraintLayout   *layout,
                              GtkWidget               *child,
                              EmeusConstraint         *first_constaint,
                              ...)
{
}

void
emeus_constraint_layout_child_add_constraint (EmeusConstraintLayout   *layout,
                                              GtkWidget               *child,
                                              EmeusConstraint         *constraint)
{
}
