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

enum {
  CHILD_PROP_NAME = 1,

  CHILD_N_PROPS
};

static GParamSpec *emeus_constraint_layout_child_properties[CHILD_N_PROPS];

G_DEFINE_TYPE (EmeusConstraintLayout, emeus_constraint_layout, GTK_TYPE_CONTAINER)

G_DEFINE_TYPE (EmeusConstraintLayoutChild, emeus_constraint_layout_child, GTK_TYPE_BIN)

static void
emeus_constraint_layout_finalize (GObject *gobject)
{
  EmeusConstraintLayout *self = EMEUS_CONSTRAINT_LAYOUT (gobject);

  g_clear_pointer (&self->children, g_sequence_free);
  g_clear_pointer (&self->bound_attributes, g_hash_table_unref);

  simplex_solver_remove_constraint (&self->solver, self->top_constraint);
  simplex_solver_remove_constraint (&self->solver, self->left_constraint);

  simplex_solver_clear (&self->solver);

  G_OBJECT_CLASS (emeus_constraint_layout_parent_class)->finalize (gobject);
}

static Variable *
get_layout_attribute (EmeusConstraintLayout   *layout,
                      EmeusConstraintAttribute attr)
{
  const char *attr_name = get_attribute_name (attr);
  Variable *res = g_hash_table_lookup (layout->bound_attributes, attr_name);

  if (res == NULL)
    {
      res = simplex_solver_create_variable (&layout->solver, attr_name, 0.0);
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

  res = simplex_solver_create_variable (child->solver, attr_name, 0.0);
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

        child->right_constraint =
          simplex_solver_add_constraint (child->solver,
                                         res,
                                         relation_to_operator (EMEUS_CONSTRAINT_RELATION_EQ),
                                         expr,
                                         strength_to_value (EMEUS_CONSTRAINT_STRENGTH_REQUIRED));

        expression_unref (expr);
      }
      break;

    case EMEUS_CONSTRAINT_ATTRIBUTE_BOTTOM:
      {
        Variable *top = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_TOP);
        Variable *height = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_HEIGHT);
        Expression *expr = expression_plus_variable (expression_new_from_variable (top), height);

        child->bottom_constraint =
          simplex_solver_add_constraint (child->solver,
                                         res,
                                         relation_to_operator (EMEUS_CONSTRAINT_RELATION_EQ),
                                         expr,
                                         strength_to_value (EMEUS_CONSTRAINT_STRENGTH_REQUIRED));

        expression_unref (expr);
      }
      break;

    case EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_X:
      {
        Variable *left = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_LEFT);
        Variable *width = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH);
        Expression *expr =
          expression_times (expression_plus_variable (expression_new_from_variable (left), width),
                            0.5);

        child->center_x_constraint =
          simplex_solver_add_constraint (child->solver,
                                         res,
                                         relation_to_operator (EMEUS_CONSTRAINT_RELATION_EQ),
                                         expr,
                                         strength_to_value (EMEUS_CONSTRAINT_STRENGTH_REQUIRED));

        expression_unref (expr);
      }
      break;

    case EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_Y:
      {
        Variable *top = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_TOP);
        Variable *height = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_HEIGHT);
        Expression *expr =
          expression_times (expression_plus_variable (expression_new_from_variable (top), height),
                            0.5);

        child->center_y_constraint =
          simplex_solver_add_constraint (child->solver,
                                         res,
                                         relation_to_operator (EMEUS_CONSTRAINT_RELATION_EQ),
                                         expr,
                                         strength_to_value (EMEUS_CONSTRAINT_STRENGTH_REQUIRED));

        expression_unref (expr);
      }
      break;

    default:
      break;
    }

  return res;
}

static void
emeus_constraint_layout_get_preferred_size (EmeusConstraintLayout *self,
                                            GtkOrientation         orientation,
                                            int                   *minimum_p,
                                            int                   *natural_p)
{
  Variable *size = NULL;

  if (g_sequence_is_empty (self->children))
    {
      if (minimum_p != NULL)
        *minimum_p = 0;

      if (natural_p != NULL)
        *natural_p = 0;

      return;
    }

  switch (orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      size = get_layout_attribute (self, EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH);
      break;

    case GTK_ORIENTATION_VERTICAL:
      size = get_layout_attribute (self, EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH);
      break;
    }

  g_assert (size != NULL);

  if (minimum_p != NULL)
    *minimum_p = variable_get_value (size);
  if (natural_p != NULL)
    *natural_p = variable_get_value (size);
}

static void
emeus_constraint_layout_get_preferred_width (GtkWidget *widget,
                                             int       *minimum_p,
                                             int       *natural_p)
{
  emeus_constraint_layout_get_preferred_size (EMEUS_CONSTRAINT_LAYOUT (widget),
                                              GTK_ORIENTATION_HORIZONTAL,
                                              minimum_p, natural_p);
}

static void
emeus_constraint_layout_get_preferred_height (GtkWidget *widget,
                                              int       *minimum_p,
                                              int       *natural_p)
{
  emeus_constraint_layout_get_preferred_size (EMEUS_CONSTRAINT_LAYOUT (widget),
                                              GTK_ORIENTATION_VERTICAL,
                                              minimum_p, natural_p);
}

static void
emeus_constraint_layout_size_allocate (GtkWidget     *widget,
                                       GtkAllocation *allocation)
{
  EmeusConstraintLayout *self = EMEUS_CONSTRAINT_LAYOUT (widget);
  EmeusConstraintLayoutChild *child;
  Variable *layout_width, *layout_height;
  GSequenceIter *iter;

  gtk_widget_set_allocation (widget, allocation);

  if (g_sequence_is_empty (self->children))
    return;

  layout_width = get_layout_attribute (self, EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH);
  layout_height = get_layout_attribute (self, EMEUS_CONSTRAINT_ATTRIBUTE_HEIGHT);

  if (!simplex_solver_has_edit_variable (&self->solver, layout_width))
    simplex_solver_add_edit_variable (&self->solver, layout_width, STRENGTH_REQUIRED);
  if (!simplex_solver_has_edit_variable (&self->solver, layout_height))
    simplex_solver_add_edit_variable (&self->solver, layout_height, STRENGTH_REQUIRED);

  simplex_solver_suggest_value (&self->solver, layout_width, allocation->width);
  simplex_solver_suggest_value (&self->solver, layout_height, allocation->height);
  simplex_solver_resolve (&self->solver);

  iter = g_sequence_get_begin_iter (self->children);
  while (!g_sequence_iter_is_end (iter))
    {
      Variable *top, *left, *width, *height, *center_x;
      GtkAllocation child_alloc;
      GtkRequisition minimum;

      child = g_sequence_get (iter);
      iter = g_sequence_iter_next (iter);

      top = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_TOP);
      left = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_LEFT);
      width = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH);
      height = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_HEIGHT);
      center_x = get_child_attribute (child, EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_X);

#ifdef EMEUS_ENABLE_DEBUG
      g_print ("child [%p] = { .top:%g, .left:%g, .width:%g, .height:%g, .centerX:%g }\n",
               child,
               variable_get_value (top),
               variable_get_value (left),
               variable_get_value (width),
               variable_get_value (height),
               variable_get_value (center_x));
#endif

      gtk_widget_get_preferred_size (GTK_WIDGET (child), &minimum, NULL);

      child_alloc.x = floor (variable_get_value (top));
      child_alloc.y = floor (variable_get_value (left));
      child_alloc.width = variable_get_value (width) > minimum.width
                        ? ceil (variable_get_value (width))
                        : minimum.width;
      child_alloc.height = variable_get_value (height) > minimum.height
                         ? ceil (variable_get_value (height))
                         : minimum.height;

      gtk_widget_size_allocate (GTK_WIDGET (child), &child_alloc);
    }
}

static void
emeus_constraint_layout_add (GtkContainer *container,
                             GtkWidget    *widget)
{
  EmeusConstraintLayout *self = EMEUS_CONSTRAINT_LAYOUT (container);

  emeus_constraint_layout_pack (self, widget, NULL, NULL);
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

  gobject_class->finalize = emeus_constraint_layout_finalize;

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

  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);

  simplex_solver_init (&self->solver);

  self->children = g_sequence_new (NULL);

  self->bound_attributes = g_hash_table_new_full (NULL, NULL,
                                                  NULL,
                                                  (GDestroyNotify) variable_unref);

  /* Add two required stay constraints for the top left corner */
  var = simplex_solver_create_variable (&self->solver, "parent.top", 0.0);
  g_hash_table_insert (self->bound_attributes,
                       (gpointer) get_attribute_name (EMEUS_CONSTRAINT_ATTRIBUTE_TOP),
                       var);
  self->top_constraint =
    simplex_solver_add_stay_variable (&self->solver, var, STRENGTH_REQUIRED);

  var = simplex_solver_create_variable (&self->solver, "parent.left", 0.0);
  g_hash_table_insert (self->bound_attributes,
                       (gpointer) get_attribute_name (EMEUS_CONSTRAINT_ATTRIBUTE_LEFT),
                       var);
  self->left_constraint =
    simplex_solver_add_stay_variable (&self->solver, var, STRENGTH_REQUIRED);
}

/**
 * emeus_constraint_layout_new:
 *
 * Creates a new constraint-based layout manager.
 *
 * Returns: (transfer full): the newly created layout widget
 *
 * Since: 1.0
 */
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

  if (emeus_constraint_is_attached (constraint))
    {
      const char *constraint_description = emeus_constraint_to_string (constraint);

      g_critical ("Constraint '%s' cannot be attached to more than "
                  "one child.",
                  constraint_description);

      return;
    }

  if (!emeus_constraint_attach (constraint, layout, child))
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
      attr2 = simplex_solver_create_variable (constraint->solver, "const",
                                              emeus_constraint_get_constant (constraint));

      simplex_solver_add_stay_variable (child->solver, attr2, STRENGTH_REQUIRED);

      expr = expression_new_from_variable (attr2);

      constraint->constraint =
        simplex_solver_add_constraint (constraint->solver,
                                       attr1,
                                       relation_to_operator (constraint->relation),
                                       expr,
                                       strength_to_value (constraint->strength));

      expression_unref (expr);
      variable_unref (attr2);

      return;
    }

  /* Alternatively, if it's not a constant value, we find the variable
   * associated with it
   */
  if (constraint->source_object != NULL)
    {
      EmeusConstraintLayoutChild *source_child;

      if (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (constraint->source_object))
        source_child = constraint->source_object;
      else
        source_child = (EmeusConstraintLayoutChild *) gtk_widget_get_parent (constraint->source_object);

      attr2 = get_child_attribute (source_child, constraint->source_attribute);
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

  expression_unref (expr);
}

static gboolean
remove_child_constraint (EmeusConstraintLayout      *layout,
                         EmeusConstraintLayoutChild *child,
                         EmeusConstraint            *constraint)
{
  if (constraint->target_object != child)
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
  GSequenceIter *iter;

  if (gtk_widget_get_parent (widget) == GTK_WIDGET (layout))
    return TRUE;

  iter = g_sequence_get_begin_iter (layout->children);
  while (!g_sequence_iter_is_end (iter))
    {
      GtkWidget *child = g_sequence_get (iter);

      iter = g_sequence_iter_next (iter);

      if (gtk_widget_get_parent (widget) == child)
        return TRUE;
    }

  return FALSE;
}

/**
 * emeus_constraint_layout_pack:
 * @layout: a #EmeusConstraintLayout
 * @child: a #GtkWidget
 * @name: (nullable): an optional name for the @child
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
                              const char            *name,
                              EmeusConstraint       *first_constraint,
                              ...)
{
  EmeusConstraintLayoutChild *layout_child;
  EmeusConstraint *constraint;
  va_list args;

  g_return_if_fail (EMEUS_IS_CONSTRAINT_LAYOUT (layout));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (EMEUS_IS_CONSTRAINT (first_constraint) || first_constraint == NULL);

  g_return_if_fail (gtk_widget_get_parent (child) == NULL);

  if (EMEUS_IS_CONSTRAINT_LAYOUT_CHILD (child))
    layout_child = EMEUS_CONSTRAINT_LAYOUT_CHILD (child);
  else
    {
      layout_child = (EmeusConstraintLayoutChild *) emeus_constraint_layout_child_new (name);

      gtk_widget_show (GTK_WIDGET (layout_child));
      gtk_container_add (GTK_CONTAINER (layout_child), child);
    }

  layout_child->iter = g_sequence_append (layout->children, layout_child);
  layout_child->solver = &layout->solver;

  gtk_widget_set_parent (GTK_WIDGET (layout_child), GTK_WIDGET (layout));

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
emeus_constraint_layout_child_finalize (GObject *gobject)
{
  EmeusConstraintLayoutChild *self = EMEUS_CONSTRAINT_LAYOUT_CHILD (gobject);

  g_free (self->name);

  G_OBJECT_CLASS (emeus_constraint_layout_child_parent_class)->finalize (gobject);
}

static void
emeus_constraint_layout_child_dispose (GObject *gobject)
{
  EmeusConstraintLayoutChild *self = EMEUS_CONSTRAINT_LAYOUT_CHILD (gobject);

  if (self->right_constraint != NULL)
    {
      simplex_solver_remove_constraint (self->solver, self->right_constraint);
      self->right_constraint = NULL;
    }

  if (self->bottom_constraint != NULL)
    {
      simplex_solver_remove_constraint (self->solver, self->bottom_constraint);
      self->bottom_constraint = NULL;
    }

  if (self->center_x_constraint != NULL)
    {
      simplex_solver_remove_constraint (self->solver, self->center_x_constraint);
      self->center_x_constraint = NULL;
    }

  if (self->center_y_constraint != NULL)
    {
      simplex_solver_remove_constraint (self->solver, self->center_y_constraint);
      self->center_y_constraint = NULL;
    }

  g_clear_pointer (&self->constraints, g_hash_table_unref);
  g_clear_pointer (&self->bound_attributes, g_hash_table_unref);

  G_OBJECT_CLASS (emeus_constraint_layout_child_parent_class)->dispose (gobject);
}

static void
emeus_constraint_layout_child_set_property (GObject      *gobject,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  EmeusConstraintLayoutChild *self = EMEUS_CONSTRAINT_LAYOUT_CHILD (gobject);

  switch (prop_id)
    {
    case CHILD_PROP_NAME:
      self->name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
emeus_constraint_layout_child_get_property (GObject    *gobject,
                                            guint       prop_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  EmeusConstraintLayoutChild *self = EMEUS_CONSTRAINT_LAYOUT_CHILD (gobject);

  switch (prop_id)
    {
    case CHILD_PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
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
    *natural_p = MAX (child_nat, size);
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
emeus_constraint_layout_child_class_init (EmeusConstraintLayoutChildClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  gobject_class->set_property = emeus_constraint_layout_child_set_property;
  gobject_class->get_property = emeus_constraint_layout_child_get_property;
  gobject_class->dispose = emeus_constraint_layout_child_dispose;
  gobject_class->finalize = emeus_constraint_layout_child_finalize;

  widget_class->get_preferred_width = emeus_constraint_layout_child_get_preferred_width;
  widget_class->get_preferred_height = emeus_constraint_layout_child_get_preferred_height;

  gtk_container_class_handle_border_width (container_class);

  emeus_constraint_layout_child_properties[CHILD_PROP_NAME] =
    g_param_spec_string ("name", "Name", "The name of the child",
                         NULL,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, CHILD_N_PROPS,
                                     emeus_constraint_layout_child_properties);
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
 * @name: (nullable): a name for the child
 *
 * Creates a new #EmeusConstraintLayoutChild widget.
 *
 * Returns: (transfer full): the newly created #EmeusConstraintLayoutChild widget
 *
 * Since: 1.0
 */
GtkWidget *
emeus_constraint_layout_child_new (const char *name)
{
  return g_object_new (EMEUS_TYPE_CONSTRAINT_LAYOUT_CHILD,
                       "name", name,
                       NULL);
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
