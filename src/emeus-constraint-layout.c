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

#include <math.h>

G_DEFINE_TYPE (EmeusConstraintLayout, emeus_constraint_layout, GTK_TYPE_CONTAINER)

G_DEFINE_TYPE (EmeusConstraintLayoutChild, emeus_constraint_layout_child, GTK_TYPE_BIN)

static void
emeus_constraint_layout_dispose (GObject *gobject)
{
  EmeusConstraintLayout *self = EMEUS_CONSTRAINT_LAYOUT (gobject);

  g_clear_pointer (&self->children, g_sequence_free);
  g_clear_pointer (&self->bound_attributes, g_hash_table_unref);

  simplex_solver_clear (&self->solver);

  G_OBJECT_CLASS (emeus_constraint_layout_parent_class)->dispose (gobject);
}

static Variable *
get_layout_attribute (EmeusConstraintLayout   *layout,
                      EmeusConstraintAttribute attr)
{
  const char *attr_name = get_attribute_name (attr);
  Variable *res = g_hash_table_lookup (layout->bound_attributes, attr_name);

  if (res == NULL)
    {
      res = simplex_solver_create_variable (&layout->solver);
      g_hash_table_insert (layout->bound_attributes, (gpointer) attr_name, res);
    }

  return res;
}

static Variable *
get_child_attribute (EmeusConstraintLayoutChild *child,
                     EmeusConstraintAttribute    attr)
{
  GtkTextDirection text_dir;
  const char *attr_name;
  Variable *res;

  /* Resolve the start/end attributes depending on the child's text direction */
  if (attr == EMEUS_CONSTRAINT_ATTRIBUTE_START)
    {
      text_dir = gtk_widget_get_direction (GTK_WIDGET (child));
      if (text_dir == GTK_TEXT_DIR_RTL)
        attr = EMEUS_CONSTRAINT_ATTRIBUTE_RIGHT;
      else
        attr = EMEUS_CONSTRAINT_ATTRIBUTE_LEFT;
    }
  else if (attr == EMEUS_CONSTRAINT_ATTRIBUTE_END)
    {
      text_dir = gtk_widget_get_direction (GTK_WIDGET (child));
      if (text_dir == GTK_TEXT_DIR_RTL)
        attr = EMEUS_CONSTRAINT_ATTRIBUTE_LEFT;
      else
        attr = EMEUS_CONSTRAINT_ATTRIBUTE_RIGHT;
    }

  attr_name = get_attribute_name (attr);
  res = g_hash_table_lookup (child->bound_attributes, attr_name);
  if (res != NULL)
    return res;

  res = simplex_solver_create_variable (child->solver);
  g_hash_table_insert (child->bound_attributes, (gpointer) attr_name, res);

  /* Some attributes are really constraints computed from other
   * attributes, to avoid creating additional constraints from
   * the user's perspective
   */
  switch (attr)
    {
    case EMEUS_CONSTRAINT_ATTRIBUTE_RIGHT:
      {
        Variable *left = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_LEFT);
        Variable *width = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH);
        Expression *expr = expression_plus_variable (expression_new_from_variable (left), width);

        simplex_solver_add_constraint (child->solver,
                                       res,
                                       relation_to_operator (EMEUS_CONSTRAINT_RELATION_EQ),
                                       expr,
                                       strength_to_value (EMEUS_CONSTRAINT_STRENGTH_REQUIRED));
      }
      break;

    case EMEUS_CONSTRAINT_ATTRIBUTE_BOTTOM:
      {
        Variable *top = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_TOP);
        Variable *height = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_HEIGHT);
        Expression *expr = expression_plus_variable (expression_new_from_variable (top), height);

        simplex_solver_add_constraint (child->solver,
                                       res,
                                       relation_to_operator (EMEUS_CONSTRAINT_RELATION_EQ),
                                       expr,
                                       strength_to_value (EMEUS_CONSTRAINT_STRENGTH_REQUIRED));
      }
      break;

    case EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_X:
      {
        Variable *left = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_LEFT);
        Variable *width = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH);
        Expression *expr =
          expression_times (expression_plus_variable (expression_new_from_variable (left), width),
                            0.5);

        simplex_solver_add_constraint (child->solver,
                                       res,
                                       relation_to_operator (EMEUS_CONSTRAINT_RELATION_EQ),
                                       expr,
                                       strength_to_value (EMEUS_CONSTRAINT_STRENGTH_REQUIRED));
      }
      break;

    case EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_Y:
      {
        Variable *top = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_TOP);
        Variable *height = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_HEIGHT);
        Expression *expr =
          expression_times (expression_plus_variable (expression_new_from_variable (top), height),
                            0.5);

        simplex_solver_add_constraint (child->solver,
                                       res,
                                       relation_to_operator (EMEUS_CONSTRAINT_RELATION_EQ),
                                       expr,
                                       strength_to_value (EMEUS_CONSTRAINT_STRENGTH_REQUIRED));
      }
      break;

    default:
      break;
    }

  return res;
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

  emeus_constraint_layout_pack (self, widget, NULL);
}

static void
emeus_constraint_layout_remove (GtkContainer *container,
                                GtkWidget    *widget)
{
  EmeusConstraintLayout *self = EMEUS_CONSTRAINT_LAYOUT (container);
  EmeusConstraintLayoutChild *child;
  gboolean was_visible;

  child = EMEUS_CONSTRAINT_LAYOUT_CHILD (widget);
  if (g_sequence_iter_get_sequence (child->iter) != self->children)
    {
      g_critical ("Tried to remove non child %p", widget);
      return;
    }

  was_visible = gtk_widget_get_visible (widget);

  gtk_widget_unparent (widget);
  g_sequence_remove (child->iter);

  if (was_visible && gtk_widget_get_visible (GTK_WIDGET (container)))
    gtk_widget_queue_resize (GTK_WIDGET (container));
}

static void
emeus_constraint_layout_forall (GtkContainer *container,
                                gboolean      internals,
                                GtkCallback   callback,
                                gpointer      data)
{
  EmeusConstraintLayout *self = EMEUS_CONSTRAINT_LAYOUT (container);
  GSequenceIter *iter;
  GtkWidget *child;

  if (g_sequence_is_empty (self->children))
    return;

  iter = g_sequence_get_begin_iter (self->children);
  while (!g_sequence_iter_is_end (iter))
    {
      child = g_sequence_get (iter);
      iter = g_sequence_iter_next (iter);

      callback (child, data);
    }
}

static GType
emeus_constraint_layout_child_type (GtkContainer *container)
{
  return EMEUS_TYPE_CONSTRAINT_LAYOUT_CHILD;
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
  container_class->child_type = emeus_constraint_layout_child_type;
  gtk_container_class_handle_border_width (container_class);
}

static void
emeus_constraint_layout_init (EmeusConstraintLayout *self)
{
  Variable *var;

  simplex_solver_init (&self->solver);

  self->children = g_sequence_new (NULL);

  self->bound_attributes = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                  g_free,
                                                  (GDestroyNotify) variable_unref);

  /* Add two required stay constraints for the top left corner */
  var = simplex_solver_create_variable (&self->solver);
  variable_set_value (var, 0.0);
  g_hash_table_insert (self->bound_attributes,
                       g_strdup (get_attribute_name (EMEUS_CONSTRAINT_ATTRIBUTE_TOP)),
                       var);
  simplex_solver_add_stay_variable (&self->solver, var, STRENGTH_REQUIRED);

  var = simplex_solver_create_variable (&self->solver);
  variable_set_value (var, 0.0);
  g_hash_table_insert (self->bound_attributes,
                       g_strdup (get_attribute_name (EMEUS_CONSTRAINT_ATTRIBUTE_LEFT)),
                       var);
  simplex_solver_add_stay_variable (&self->solver, var, STRENGTH_REQUIRED);
}

GtkWidget *
emeus_constraint_layout_new (void)
{
  return g_object_new (EMEUS_TYPE_CONSTRAINT_LAYOUT, NULL);
}

static void
add_child_constraint (EmeusConstraintLayout      *layout,
                      EmeusConstraintLayoutChild *child,
                      EmeusConstraint            *constraint)
{
  Variable *attr1, *attr2;
  Expression *expr;

  if (!emeus_constraint_attach (constraint, layout))
    return;

  g_hash_table_add (child->constraints, g_object_ref_sink (constraint));

  /* attr1 is the LHS of the linear equation */
  attr1 = get_child_attribute (constraint->target_object,
                               constraint->target_attribute);

  /* attr2 is the RHS of the linear equation; if it's a constant value
   * we create a stay constraint for it. Stay constraints ensure that a
   * variable won't be modified by the solver.
   */
  if (constraint->source_attribute == EMEUS_CONSTRAINT_ATTRIBUTE_INVALID)
    {
      attr2 = simplex_solver_create_variable (constraint->solver);
      variable_set_value (attr2, emeus_constraint_get_constant (constraint));

      simplex_solver_add_stay_variable (child->solver, attr2, STRENGTH_REQUIRED);

      constraint->constraint =
        simplex_solver_add_constraint (constraint->solver,
                                       attr1,
                                       relation_to_operator (constraint->relation),
                                       expression_new_from_variable (attr2),
                                       strength_to_value (constraint->strength));

      return;
    }

  /* Alternatively, if it's not a constant value, we find the variable
   * associated with it
   */
  if (constraint->source_object != NULL)
    {
      attr2 = get_child_attribute (constraint->source_object,
                                   constraint->source_attribute);
    }
  else
    {
      attr2 = get_layout_attribute (layout, constraint->source_attribute);
    }

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

static gboolean
remove_child_constraint (EmeusConstraintLayout      *layout,
                         EmeusConstraintLayoutChild *child,
                         EmeusConstraint            *constraint)
{
  g_assert (emeus_constraint_is_attached (constraint));

  if (&layout->solver != constraint->solver)
    {
      g_critical ("Attempting to remove unknown constraint %p", constraint);
      return FALSE;
    }

  emeus_constraint_detach (constraint);

  g_hash_table_remove (child->constraints, constraint);

  return TRUE;
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
  return gtk_widget_get_parent (widget) == GTK_WIDGET (layout);
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
  EmeusConstraintLayoutChild *layout_child;
  EmeusConstraint *constraint;
  va_list args;

  g_return_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT (layout));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (EMEUS_IS_CONSTRAINT (first_constraint));

  g_return_if_fail (gtk_widget_get_parent (child) == NULL);

  if (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child))
    layout_child = EMEUS_CONSTRAINT_LAYOUT_CHILD (child);
  else
    {
      layout_child = (EmeusConstraintLayoutChild *) emeus_constraint_layout_child_new ();

      gtk_widget_show (GTK_WIDGET (layout_child));
      gtk_container_add (GTK_CONTAINER (layout_child), child);
    }

  layout_child->iter = g_sequence_append (layout->children, layout_child);

  if (first_constraint == NULL)
    return;

  va_start (args, first_constraint);

  constraint = first_constraint;
  while (constraint != NULL)
    {
      add_child_constraint (layout, layout_child, constraint);

      constraint = va_arg (args, EmeusConstraint *);
    }

  va_end (args);
}

static void
emeus_constraint_layout_child_dispose (GObject *gobject)
{
  EmeusConstraintLayoutChild *self = EMEUS_CONSTRAINT_LAYOUT_CHILD (gobject);

  g_clear_pointer (&self->constraints, g_hash_table_unref);
  g_clear_pointer (&self->bound_attributes, g_hash_table_unref);

  G_OBJECT_CLASS (emeus_constraint_layout_child_parent_class)->dispose (gobject);
}

static void
emeus_constraint_layout_child_get_preferred_size (EmeusConstraintLayoutChild *self,
                                                  GtkOrientation              orientation,
                                                  int                        *minimum_p,
                                                  int                        *natural_p)
{
  GtkWidget *child = gtk_bin_get_child (GTK_BIN (self));
  int child_min = 0;
  int child_nat = 0;
  int size = 0;
  Variable *attr = NULL;

  switch (orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      attr = get_child_attribute (self, EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH);
      if (gtk_widget_get_visible (child))
        gtk_widget_get_preferred_width (child, &child_min, &child_nat);
      break;

    case GTK_ORIENTATION_VERTICAL:
      attr = get_child_attribute (self, EMEUS_CONSTRAINT_ATTRIBUTE_HEIGHT);
      if (gtk_widget_get_visible (child))
        gtk_widget_get_preferred_height (child, &child_min, &child_nat);
      break;
    }

  g_assert (attr != NULL);

  size = ceil (variable_get_value (attr));

  if (minimum_p != NULL)
    *minimum_p = MAX (child_min, size);

  if (natural_p != NULL)
    *natural_p = MIN (child_nat, size);
}

static void
emeus_constraint_layout_child_get_preferred_width (GtkWidget *widget,
                                                   int       *minimum_p,
                                                   int       *natural_p)
{
  emeus_constraint_layout_child_get_preferred_size (EMEUS_CONSTRAINT_LAYOUT_CHILD (widget),
                                                    GTK_ORIENTATION_HORIZONTAL,
                                                    minimum_p,
                                                    natural_p);
}

static void
emeus_constraint_layout_child_get_preferred_height (GtkWidget *widget,
                                                    int       *minimum_p,
                                                    int       *natural_p)
{
  emeus_constraint_layout_child_get_preferred_size (EMEUS_CONSTRAINT_LAYOUT_CHILD (widget),
                                                    GTK_ORIENTATION_VERTICAL,
                                                    minimum_p,
                                                    natural_p);
}

static void
emeus_constraint_layout_child_size_allocate (GtkWidget     *widget,
                                             GtkAllocation *allocation)
{
  EmeusConstraintLayoutChild *self = EMEUS_CONSTRAINT_LAYOUT_CHILD (widget);
  GtkWidget *child = gtk_bin_get_child (GTK_BIN (widget));
  GtkAllocation child_alloc;
  int baseline;

  gtk_widget_set_allocation (widget, allocation);

  if (!gtk_widget_get_visible (child))
    return;

  child_alloc.x = 0;
  child_alloc.y = 0;
  child_alloc.width = allocation->width;
  child_alloc.height = allocation->height;

  baseline = variable_get_value (get_child_attribute (self, EMEUS_CONSTRAINT_ATTRIBUTE_BASELINE));

  gtk_widget_size_allocate_with_baseline (child, &child_alloc, baseline > 0 ? baseline : -1);
}

static void
emeus_constraint_layout_child_class_init (EmeusConstraintLayoutChildClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  gobject_class->dispose = emeus_constraint_layout_child_dispose;

  widget_class->get_preferred_width = emeus_constraint_layout_child_get_preferred_width;
  widget_class->get_preferred_height = emeus_constraint_layout_child_get_preferred_height;
  widget_class->size_allocate = emeus_constraint_layout_child_size_allocate;

  gtk_container_class_handle_border_width (container_class);
}

static void
emeus_constraint_layout_child_init (EmeusConstraintLayoutChild *self)
{
  gtk_widget_set_redraw_on_allocate (GTK_WIDGET (self), TRUE);

  self->constraints = g_hash_table_new_full (NULL, NULL,
                                             g_object_unref,
                                             NULL);

  self->bound_attributes = g_hash_table_new_full (NULL, NULL,
                                                  NULL,
                                                  (GDestroyNotify) variable_unref);
}

/**
 * emeus_constraint_layout_child_new: (constructor)
 *
 * Creates a new #EmeusConstraintLayoutChild widget.
 *
 * Returns: (transfer full): the newly created #EmeusConstraintLayoutChild widget
 *
 * Since: 1.0
 */
GtkWidget *
emeus_constraint_layout_child_new (void)
{
  return g_object_new (EMEUS_TYPE_CONSTRAINT_LAYOUT_CHILD, NULL);
}

/**
 * emeus_constraint_layout_child_add_constraint:
 * @child: a #EmeusConstraintLayoutChild
 * @constraint: a #EmeusConstraint
 *
 * Adds the given @constraint to the list of constraints applied to
 * the @child of a #EmeusConstraintLayout
 *
 * The #EmeusConstraintLayoutChild will own the @constraint until the
 * @child is removed, or until the @constraint is removed.
 *
 * Since: 1.0
 */
void
emeus_constraint_layout_child_add_constraint (EmeusConstraintLayoutChild *child,
                                              EmeusConstraint            *constraint)
{
  EmeusConstraintLayout *layout;
  GtkWidget *widget;

  g_return_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child));
  g_return_if_fail (gtk_widget_get_parent (GTK_WIDGET (child)) != NULL);
  g_return_if_fail (EMEUS_IS_CONSTRAINT (constraint));

  g_return_if_fail (!emeus_constraint_is_attached (constraint));

  widget = GTK_WIDGET (child);
  layout = EMEUS_CONSTRAINT_LAYOUT (gtk_widget_get_parent (widget));

  add_child_constraint (layout, child, constraint);

  if (gtk_widget_get_visible (widget))
    gtk_widget_queue_resize (widget);
}

/**
 * emeus_constraint_layout_child_remove_constraint:
 * @child: a #EmeusConstraintLayoutChild
 * @constraint: a #EmeusConstraint
 *
 * Removes the given @constraint from the list of constraints applied
 * to the @child of a #EmeusConstraintLayout.
 *
 * Since: 1.0
 */
void
emeus_constraint_layout_child_remove_constraint (EmeusConstraintLayoutChild *child,
                                                 EmeusConstraint            *constraint)
{
  EmeusConstraintLayout *layout;
  GtkWidget *widget;

  g_return_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child));
  g_return_if_fail (EMEUS_IS_CONSTRAINT (constraint));
  g_return_if_fail (gtk_widget_get_parent (GTK_WIDGET (child)) != NULL);
  g_return_if_fail (!emeus_constraint_is_attached (constraint));

  widget = GTK_WIDGET (child);
  layout = EMEUS_CONSTRAINT_LAYOUT (gtk_widget_get_parent (widget));

  if (!remove_child_constraint (layout, child, constraint))
    return;

  if (gtk_widget_get_visible (widget))
    gtk_widget_queue_resize (widget);
}

/**
 * emeus_constraint_layout_child_clear_constraints:
 * @child: a #EmeusConstraintLayoutChild
 *
 * Clears all the constraints associated with a child of #EmeusConstraintLayout.
 *
 * Since: 1.0
 */
void
emeus_constraint_layout_child_clear_constraints (EmeusConstraintLayoutChild *child)
{
  g_return_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child));

  g_hash_table_remove_all (child->constraints);
  g_hash_table_remove_all (child->bound_attributes);

  gtk_widget_queue_resize (GTK_WIDGET (child));
}

int
emeus_constraint_layout_child_get_top (EmeusConstraintLayoutChild *child)
{
  Variable *res;

  g_return_val_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child), 0);

  res = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_TOP);

  return floor (variable_get_value (res));
}

int
emeus_constraint_layout_child_get_right (EmeusConstraintLayoutChild *child)
{
  Variable *res;

  g_return_val_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child), 0);

  res = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_RIGHT);

  return ceil (variable_get_value (res));
}

int
emeus_constraint_layout_child_get_bottom (EmeusConstraintLayoutChild *child)
{
  Variable *res;

  g_return_val_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child), 0);

  res = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_BOTTOM);

  return ceil (variable_get_value (res));
}

int
emeus_constraint_layout_child_get_left (EmeusConstraintLayoutChild *child)
{
  Variable *res;

  g_return_val_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child), 0);

  res = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_LEFT);

  return floor (variable_get_value (res));
}

int
emeus_constraint_layout_child_get_width (EmeusConstraintLayoutChild *child)
{
  Variable *res;

  g_return_val_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child), 0);

  res = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH);

  return ceil (variable_get_value (res));
}

int
emeus_constraint_layout_child_get_height (EmeusConstraintLayoutChild *child)
{
  Variable *res;

  g_return_val_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child), 0);

  res = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_HEIGHT);

  return ceil (variable_get_value (res));
}

int
emeus_constraint_layout_child_get_center_x (EmeusConstraintLayoutChild *child)
{
  Variable *res;

  g_return_val_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child), 0);

  res = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_X);

  return ceil (variable_get_value (res));
}

int
emeus_constraint_layout_child_get_center_y (EmeusConstraintLayoutChild *child)
{
  Variable *res;

  g_return_val_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child), 0);

  res = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_Y);

  return ceil (variable_get_value (res));
}

void
emeus_constraint_layout_child_set_intrinsic_width (EmeusConstraintLayoutChild *child,
                                                   int                         width)
{
  Variable *attr;

  g_return_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child));

  if (child->intrinsic_width == width)
    return;

  attr = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH);

  if (child->intrinsic_width < 0)
    simplex_solver_add_edit_variable (child->solver, attr, STRENGTH_REQUIRED);

  child->intrinsic_width = width;

  if (width > 0)
    simplex_solver_suggest_value (child->solver, attr, width);

  simplex_solver_resolve (child->solver);

  if (gtk_widget_get_visible (GTK_WIDGET (child)))
    gtk_widget_queue_resize (GTK_WIDGET (child));
}

void
emeus_constraint_layout_child_set_intrinsic_height (EmeusConstraintLayoutChild *child,
                                                    int                         height)
{
  Variable *attr;

  g_return_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child));

  if (child->intrinsic_height == height)
    return;

  attr = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_HEIGHT);

  if (child->intrinsic_height < 0)
    simplex_solver_add_edit_variable (child->solver, attr, STRENGTH_REQUIRED - 1);

  child->intrinsic_height = height;

  if (height > 0)
    simplex_solver_suggest_value (child->solver, attr, height);

  simplex_solver_resolve (child->solver);

  if (gtk_widget_get_visible (GTK_WIDGET (child)))
    gtk_widget_queue_resize (GTK_WIDGET (child));
}
