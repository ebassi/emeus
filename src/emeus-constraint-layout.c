/* emeus-constraint-layout.c: The constraint layout manager
 *
 * Copyright 2016  Endless
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:emeus-constraint-layout
 * @Title: EmeusConstraintLayout
 * @Short_desc: A widget container using layout constraints
 *
 * #EmeusConstraintLayout is a #GtkContainter that uses constraints
 * associated to each of its children to decide their layout.
 */

#include "config.h"

#include "emeus-constraint-layout-private.h"

#include "emeus-constraint-private.h"
#include "emeus-types-private.h"
#include "emeus-expression-private.h"
#include "emeus-simplex-solver-private.h"
#include "emeus-utils-private.h"
#include "emeus-variable-private.h"

typedef struct {
  /* The child of the layout */
  GtkWidget *widget;

  /* A pointer to the solver created by the layout manager */
  SimplexSolver *solver;

  /* HashTable<string, Variable>; a hash table of variables, one
   * for each attribute; we use these to query and suggest the
   * values for the solver.
   */
  GHashTable *bound_attributes;

  /* HashSet<EmeusConstraint>; the set of constraints on the
   * widget, using the public API objects.
   */
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

  g_clear_pointer (&data->bound_attributes, g_hash_table_unref);
  g_clear_pointer (&data->constraints, g_object_unref);

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

static Variable *
get_child_attribute (EmeusConstraintLayout *layout,
                     GtkWidget             *widget,
                     const char            *attr_name)
{
  LayoutChildData *data;
  Variable *res;

  data = get_layout_child_data (layout, widget);
  if (data == NULL)
    return NULL;

  res = g_hash_table_lookup (data->bound_attributes, attr_name);
  if (res == NULL)
    {
      res = simplex_solver_create_variable (data->solver);
      g_hash_table_insert (data->bound_attributes, g_strdup (attr_name), res);
    }

  return res;
}

static double
get_child_attribute_value (EmeusConstraintLayout *layout,
                           GtkWidget             *widget,
                           const char            *attr_name)
{
  Variable *res = get_child_attribute (layout, widget, attr_name);

  if (res == NULL)
    return 0.0;

  return variable_get_value (res);
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
  EmeusConstraintLayout *self = EMEUS_CONSTRAINT_LAYOUT (container);
  LayoutChildData *child_data;

  child_data = g_slice_new (LayoutChildData);
  child_data->solver = &self->solver;
  child_data->widget = widget;
  child_data->constraints = g_hash_table_new_full (NULL, NULL, g_object_unref, NULL);
  child_data->bound_attributes = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                        g_free,
                                                        (GDestroyNotify) variable_unref);

  g_sequence_append (self->children, child_data);

  gtk_widget_set_parent (widget, GTK_WIDGET (self));
}

static void
emeus_constraint_layout_remove (GtkContainer *container,
                                GtkWidget    *widget)
{
  EmeusConstraintLayout *self = EMEUS_CONSTRAINT_LAYOUT (container);
  LayoutChildData *child_data;

  child_data = get_layout_child_data (self, widget);
  if (child_data == NULL)
    return;

  g_assert (child_data->widget == widget);

  gtk_widget_unparent (child_data->widget);
  child_data->widget = NULL;
}

static void
emeus_constraint_layout_forall (GtkContainer *container,
                                gboolean      internals,
                                GtkCallback   callback,
                                gpointer      data)
{
  EmeusConstraintLayout *self = EMEUS_CONSTRAINT_LAYOUT (container);
  GSequenceIter *iter;

  if (g_sequence_is_empty (self->children))
    return;

  iter = g_sequence_get_begin_iter (self->children);
  while (!g_sequence_iter_is_end (iter))
    {
      LayoutChildData *child_data = g_sequence_get (iter);

      callback (child_data->widget, data);

      iter = g_sequence_iter_next (iter);
    }
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

static void
add_child_constraint (EmeusConstraintLayout *layout,
                      GtkWidget             *widget,
                      EmeusConstraint       *constraint)
{
  LayoutChildData *data = get_layout_child_data (layout, widget);
  Variable *attr1, *attr2;
  Expression *expr;

  g_assert (data != NULL);

  if (!emeus_constraint_attach (constraint, layout))
    return;

  g_hash_table_add (data->constraints, g_object_ref_sink (constraint));

  /* attr1 is the LHS of the linear equation */
  attr1 = get_child_attribute (layout,
                               constraint->target_object,
                               get_attribute_name (constraint->target_attribute));

  /* attr2 is the RHS of the linear equation; if it's a constant value
   * we create a stay constraint for it. Stay constraints ensure that a
   * variable won't be modified by the solver.
   */
  if (constraint->source_attribute == EMEUS_CONSTRAINT_ATTRIBUTE_INVALID)
    {
      attr2 = simplex_solver_create_variable (data->solver);
      variable_set_value (attr2, emeus_constraint_get_constant (constraint));

      simplex_solver_add_stay_variable (data->solver, attr2, STRENGTH_REQUIRED);

      constraint->constraint =
        simplex_solver_add_constraint (data->solver,
                                       attr1,
                                       relation_to_operator (constraint->relation),
                                       expression_new_from_variable (attr2),
                                       strength_to_value (constraint->strength));

      return;
    }

  /* Alternatively, if it's not a constant value, we find the variable
   * associated with it
   */
  attr2 = get_child_attribute (layout,
                               constraint->source_object,
                               get_attribute_name (constraint->source_attribute));

  /* Turn attr2 into an expression in the form:
   *
   *   expr = attr2 * multiplier + constant
   */
  expr =
    expression_plus (expression_times (expression_new_from_variable (attr2),
                                       constraint->multiplier),
                     constraint->constant);

  constraint->constraint =
    simplex_solver_add_constraint (constraint->solver,
                                   attr1,
                                   relation_to_operator (constraint->relation),
                                   expr,
                                   strength_to_value (constraint->strength));
}

static void
remove_child_constraint (EmeusConstraintLayout *layout,
                         GtkWidget             *widget,
                         EmeusConstraint       *constraint)
{
  LayoutChildData *data = get_layout_child_data (layout, widget);

  g_assert (data != NULL);
  g_assert (emeus_constraint_is_attached (constraint));

  emeus_constraint_detach (constraint);
  g_hash_table_remove (data->constraints, constraint);
}

SimplexSolver *
emeus_constraint_layout_get_solver (EmeusConstraintLayout *layout)
{
  return &layout->solver;
}

gboolean
emeus_constraint_layout_has_child_data (EmeusConstraintLayout *layout,
                                        GtkWidget             *widget)
{
  return get_layout_child_data (layout, widget) != NULL;
}

/**
 * emeus_constraint_layout_pack:
 * @layout: a #EmeusConstraintLayout
 * @child: a #GtkWidget
 * @first_constraint: (nullable): a #EmeusConstraint
 * @...: a %NULL-terminated list of #EmeusConstraint instances
 *
 * Adds @child to the @layout, and applies a list of constraints to it.
 *
 * This convenience function is the equivalent of calling
 * gtk_container_add() and emeus_constraint_layout_child_add_constraint()
 * for each constraint instance.
 *
 * Since: 1.0
 */
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

/**
 * emeus_constraint_layout_child_add_constraint:
 * @layout: a #EmeusConstraintLayout
 * @child: a #GtkWidget
 * @constraint: a #EmeusConstraint
 *
 * Adds the given @constraint to the list of constraints applied to
 * the @child of the @layout.
 *
 * The #EmeusConstraintLayout will own the @constraint until the
 * @child is removed, or until the @constraint is removed.
 *
 * Since: 1.0
 */
void
emeus_constraint_layout_child_add_constraint (EmeusConstraintLayout *layout,
                                              GtkWidget             *child,
                                              EmeusConstraint       *constraint)
{
  g_return_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT (layout));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (EMEUS_IS_CONSTRAINT (constraint));

  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (layout));
  g_return_if_fail (!emeus_constraint_is_attached (constraint));

  add_child_constraint (layout, child, constraint);

  gtk_widget_queue_resize (child);
}

/**
 * emeus_constraint_layout_child_remove_constraint:
 * @layout: a #EmeusConstraintLayout
 * @child: a #GtkWidget
 * @constraint: a #EmeusConstraint
 *
 * Removes the given @constraint from the list of constraints applied
 * to the @child of the @layout.
 *
 * Since: 1.0
 */
void
emeus_constraint_layout_child_remove_constraint (EmeusConstraintLayout *layout,
                                                 GtkWidget             *child,
                                                 EmeusConstraint       *constraint)
{
  g_return_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT (layout));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (EMEUS_IS_CONSTRAINT (constraint));

  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (layout));
  g_return_if_fail (!emeus_constraint_is_attached (constraint));

  remove_child_constraint (layout, child, constraint);

  gtk_widget_queue_resize (child);
}

/**
 * emeus_constraint_layout_child_clear_constraints:
 * @layout: a #EmeusConstraintLayout
 * @child: a #GtkWidget
 *
 * Clears all the constraints associated with the child @widget of
 * the @layout.
 *
 * Since: 1.0
 */
void
emeus_constraint_layout_child_clear_constraints (EmeusConstraintLayout *layout,
                                                 GtkWidget             *child)
{
  LayoutChildData *child_data;

  g_return_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT (layout));
  g_return_if_fail (GTK_IS_WIDGET (child));

  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (layout));

  child_data = get_layout_child_data (layout, child);
  g_assert (child_data != NULL);

  g_clear_pointer (&child_data->constraints, g_hash_table_unref);
  child_data->constraints = g_hash_table_new_full (NULL, NULL, g_object_unref, NULL);

  gtk_widget_queue_resize (child);
}
