#include "config.h"

#include "emeus-constraint-layout.h"

#include "emeus-types-private.h"
#include "emeus-expression-private.h"
#include "emeus-simplex-solver-private.h"
#include "emeus-variable-private.h"

typedef struct {
  GtkWidget *widget;

  SimplexSolver *solver;

  GHashTable *constraints;
} LayoutChildData;

struct _EmeusConstraintLayout
{
  GtkContainer parent_instance;

  GSequence *children;

  SimplexSolver solver;
};

G_DEFINE_TYPE (EmeusConstraintLayout, emeus_constraint_layout, GTK_TYPE_CONTAINER)

static void
layout_child_data_free (gpointer data_)
{
  LayoutChildData *data = data_;

  if (data_ == NULL)
    return;

  g_slice_free (LayoutChildData, data);
}

static void
emeus_constraint_layout_dispose (GObject *gobject)
{
  EmeusConstraintLayout *self = EMEUS_CONSTRAINT_LAYOUT (gobject);

  g_clear_pointer (&self->children, g_sequence_free);
  simplex_solver_clear (&self->solver);

  G_OBJECT_CLASS (emeus_constraint_layout_parent_class)->dispose (gobject);
}

static void
emeus_constraint_layout_get_preferred_size (GtkWidget      *widget,
                                            GtkOrientation  orientation,
                                            int            *minimum_p,
                                            int            *natural_p)
{
}

static void
emeus_constraint_layout_get_preferred_width (GtkWidget *widget,
                                             int       *minimum_p,
                                             int       *natural_p)
{
  emeus_constraint_layout_get_preferred_size (widget,
                                              GTK_ORIENTATION_HORIZONTAL,
                                              minimum_p, natural_p);
}

static void
emeus_constraint_layout_get_preferred_height (GtkWidget *widget,
                                              int       *minimum_p,
                                              int       *natural_p)
{
  emeus_constraint_layout_get_preferred_size (widget,
                                              GTK_ORIENTATION_VERTICAL,
                                              minimum_p, natural_p);
}

static void
emeus_constraint_layout_size_allocate (GtkWidget     *widget,
                                       GtkAllocation *allocation)
{
  gtk_widget_set_allocation (widget, allocation);
}

static void
emeus_constraint_layout_add (GtkContainer *container,
                             GtkWidget    *widget)
{
}

static void
emeus_constraint_layout_remove (GtkContainer *container,
                                GtkWidget    *widget)
{
}

static void
emeus_constraint_layout_forall (GtkContainer *container,
                                gboolean      internals,
                                GtkCallback   callback,
                                gpointer      data)
{
}

static void
emeus_constraint_layout_class_init (EmeusConstraintLayoutClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  gobject_class->dispose = emeus_constraint_layout_dispose;

  widget_class->get_preferred_width = emeus_constraint_layout_get_preferred_width;
  widget_class->get_preferred_height = emeus_constraint_layout_get_preferred_height;
  widget_class->size_allocate = emeus_constraint_layout_size_allocate;

  container_class->add = emeus_constraint_layout_add;
  container_class->remove = emeus_constraint_layout_remove;
  container_class->forall = emeus_constraint_layout_forall;
}

static void
emeus_constraint_layout_init (EmeusConstraintLayout *self)
{
  simplex_solver_init (&self->solver);

  self->children = g_sequence_new (layout_child_data_free);
}

GtkWidget *
emeus_constraint_layout_new (void)
{
  return g_object_new (EMEUS_TYPE_CONSTRAINT_LAYOUT, NULL);
}

static LayoutChildData *
get_layout_child_data (EmeusConstraintLayout *layout,
                       GtkWidget             *widget)
{
  GSequenceIter *iter;

  if (g_sequence_is_empty (layout->children))
    return NULL;

  iter = g_sequence_get_begin_iter (layout->children);
  while (!g_sequence_iter_is_end (iter))
    {
      LayoutChildData *data = g_sequence_get (iter);

      if (data->widget == widget)
        return data;

      iter = g_sequence_iter_next (iter);
    }

  return NULL;
}

static void
add_child_constraint (EmeusConstraintLayout *layout,
                      GtkWidget             *widget,
                      EmeusConstraint       *constraint)
{
  LayoutChildData *data = get_layout_child_data (layout, widget);

  g_assert (data != NULL);

  g_hash_table_add (data->constraints, g_object_ref_sink (constraint));
}

void
emeus_constraint_layout_pack (EmeusConstraintLayout *layout,
                              GtkWidget             *child,
                              EmeusConstraint       *first_constraint,
                              ...)
{
  EmeusConstraint *constraint;
  va_list args;

  g_return_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT (layout));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (EMEUS_IS_CONSTRAINT (first_constraint));

  g_return_if_fail (gtk_widget_get_parent (child) == NULL);

  va_start (args, first_constraint);

  gtk_container_add (GTK_CONTAINER (layout), child);

  constraint = first_constraint;
  while (constraint != NULL)
    {
      add_child_constraint (layout, child, constraint);

      constraint = va_arg (args, EmeusConstraint *);
    }

  va_end (args);
}

void
emeus_constraint_layout_child_add_constraint (EmeusConstraintLayout *layout,
                                              GtkWidget             *child,
                                              EmeusConstraint       *constraint)
{
  g_return_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT (layout));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (EMEUS_IS_CONSTRAINT (constraint));

  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (layout));

  add_child_constraint (layout, child, constraint);
}
