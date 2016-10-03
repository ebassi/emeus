/* emeus-constraint.c: The base constraint object
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
 * SECTION:emeus-constraint
 * @Title: EmeusConstraint
 * @Short_desc: The representation of a single constraint
 *
 * #EmeusConstraint is a type that describes a constraint between two widget
 * attributes which must be satisfied by a #GtkConstraintLayout.
 *
 * Each constraint is a linear equation in the form:
 *
 * |[<!-- language="C" -->
 *   target.attribute1 = source.attribute2 × multiplier + constant
 * ]|
 *
 * The #EmeusConstraintLayout will take all the constraints associated to
 * each of its children and solve the value of `attribute1` that satisfy the
 * system of equations.
 *
 * For instance, if a #EmeusConstraintLayout has two children, `button1` and
 * `button2`, and you wish for `button2` to follow `button1` with 8 pixels
 * of spacing between the two, using the direction of the text, you can
 * express this relationship as this constraint:
 *
 * |[<!-- language="C" -->
 *   button2.start = button1.end × 1.0 + 8.0
 * ]|
 *
 * If you also wish `button1` to have a minimum width of 120 pixels, and
 * `button2` to have the same size, you can use the following constraints:
 *
 * |[<!-- language="C" -->
 *   button1.width ≥ 120.0
 *   button2.width = button1.width × 1.0 + 0.0
 * ]|
 *
 * Each #EmeusConstraint instance references a target attribute; the target
 * object to which the attribute applies is a child of a #EmeusConstraintLayout,
 * and it's associated with the #EmeusConstraint instance once the constraint
 * is added to a child #GtkWidget of the layout.
 */

#include "config.h"

#include "emeus-constraint-private.h"

#include "emeus-expression-private.h"
#include "emeus-simplex-solver-private.h"
#include "emeus-variable-private.h"
#include "emeus-utils-private.h"

#include <math.h>
#include <float.h>
#include <gtk/gtk.h>

enum {
  PROP_TARGET_OBJECT = 1,
  PROP_TARGET_ATTRIBUTE,
  PROP_RELATION,
  PROP_SOURCE_OBJECT,
  PROP_SOURCE_ATTRIBUTE,
  PROP_MULTIPLIER,
  PROP_CONSTANT,
  PROP_STRENGTH,

  N_PROPERTIES
};

static GParamSpec *emeus_constraint_properties[N_PROPERTIES];

G_DEFINE_TYPE (EmeusConstraint, emeus_constraint, G_TYPE_INITIALLY_UNOWNED)

static void
emeus_constraint_finalize (GObject *gobject)
{
  EmeusConstraint *self = EMEUS_CONSTRAINT (gobject);

  g_free (self->description);

  G_OBJECT_CLASS (emeus_constraint_parent_class)->finalize (gobject);
}

static void
emeus_constraint_set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  EmeusConstraint *self = EMEUS_CONSTRAINT (gobject);

  switch (prop_id)
    {
    case PROP_TARGET_OBJECT:
      self->target_object = g_value_get_object (value);
      break;

    case PROP_TARGET_ATTRIBUTE:
      self->target_attribute = g_value_get_enum (value);
      break;

    case PROP_RELATION:
      self->relation = g_value_get_enum (value);
      break;

    case PROP_SOURCE_OBJECT:
      self->source_object = g_value_get_object (value);
      break;

    case PROP_SOURCE_ATTRIBUTE:
      self->source_attribute = g_value_get_enum (value);
      break;

    case PROP_MULTIPLIER:
      self->multiplier = g_value_get_double (value);
      break;

    case PROP_CONSTANT:
      self->constant = g_value_get_double (value);
      break;

    case PROP_STRENGTH:
      self->strength = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
emeus_constraint_get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  EmeusConstraint *self = EMEUS_CONSTRAINT (gobject);

  switch (prop_id)
    {
    case PROP_TARGET_OBJECT:
      g_value_set_object (value, self->target_object);
      break;

    case PROP_TARGET_ATTRIBUTE:
      g_value_set_enum (value, self->target_attribute);
      break;

    case PROP_RELATION:
      g_value_set_enum (value, self->relation);
      break;

    case PROP_SOURCE_OBJECT:
      g_value_set_object (value, self->source_object);
      break;

    case PROP_SOURCE_ATTRIBUTE:
      g_value_set_enum (value, self->source_attribute);
      break;

    case PROP_MULTIPLIER:
      g_value_set_double (value, self->multiplier);
      break;

    case PROP_CONSTANT:
      g_value_set_double (value, self->constant);
      break;

    case PROP_STRENGTH:
      g_value_set_enum (value, self->strength);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
emeus_constraint_class_init (EmeusConstraintClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = emeus_constraint_set_property;
  gobject_class->get_property = emeus_constraint_get_property;
  gobject_class->finalize = emeus_constraint_finalize;

  emeus_constraint_properties[PROP_TARGET_OBJECT] =
    g_param_spec_object ("target-object", "Target Object", NULL,
                         GTK_TYPE_WIDGET,
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS);

  emeus_constraint_properties[PROP_TARGET_ATTRIBUTE] =
    g_param_spec_enum ("target-attribute", "Target Attribute", NULL,
                       EMEUS_TYPE_CONSTRAINT_ATTRIBUTE,
                       EMEUS_CONSTRAINT_ATTRIBUTE_INVALID,
                       G_PARAM_CONSTRUCT_ONLY |
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS);

  emeus_constraint_properties[PROP_RELATION] =
    g_param_spec_enum ("relation", "Relation", NULL,
                       EMEUS_TYPE_CONSTRAINT_RELATION,
                       EMEUS_CONSTRAINT_RELATION_EQ,
                       G_PARAM_CONSTRUCT_ONLY |
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS);

  emeus_constraint_properties[PROP_SOURCE_OBJECT] =
    g_param_spec_object ("source-object", "Source Object", NULL,
                         GTK_TYPE_WIDGET,
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS);

  emeus_constraint_properties[PROP_SOURCE_ATTRIBUTE] =
    g_param_spec_enum ("source-attribute", "Source Attribute", NULL,
                       EMEUS_TYPE_CONSTRAINT_ATTRIBUTE,
                       EMEUS_CONSTRAINT_ATTRIBUTE_INVALID,
                       G_PARAM_CONSTRUCT_ONLY |
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS);

  emeus_constraint_properties[PROP_MULTIPLIER] =
    g_param_spec_double ("multiplier", "Multiplier", NULL,
                         -G_MAXDOUBLE, G_MAXDOUBLE,
                         1.0,
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS);

  emeus_constraint_properties[PROP_CONSTANT] =
    g_param_spec_double ("constant", "Constant", NULL,
                         -G_MAXDOUBLE, G_MAXDOUBLE,
                         0.0,
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS);

  emeus_constraint_properties[PROP_STRENGTH] =
    g_param_spec_enum ("strength", "Strength", NULL,
                       EMEUS_TYPE_CONSTRAINT_STRENGTH,
                       EMEUS_CONSTRAINT_STRENGTH_REQUIRED,
                       G_PARAM_CONSTRUCT_ONLY |
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, emeus_constraint_properties);
}

static void
emeus_constraint_init (EmeusConstraint *self)
{
  self->target_attribute = EMEUS_CONSTRAINT_ATTRIBUTE_INVALID;
  self->relation = EMEUS_CONSTRAINT_RELATION_EQ;
  self->source_attribute = EMEUS_CONSTRAINT_ATTRIBUTE_INVALID;
  self->multiplier = 1.0;
  self->constant = 0.0;
  self->strength = EMEUS_CONSTRAINT_STRENGTH_REQUIRED;
}

/**
 * emeus_constraint_new: (constructor)
 * @target_object: (type Gtk.Widget) (nullable): the target widget, or
 *   %NULL for the parent layout
 * @target_attribute: the attribute to set on the target widget
 * @relation: the relation between the target and source attributes
 * @source_object: (type Gtk.Widget) (nullable): the source widget, or
 *   %NULL for the parent layout
 * @source_attribute: the attribute to get from the source widget
 * @multiplier: the multiplication coefficient to apply to the source
 *   attribute
 * @constant: the constant to add to the source attribute
 *
 * Creates a new constraint using a value from the source widget's attribute
 * and applying it to the target widget's attribute.
 *
 * Returns: (transfer full): the newly created constraint
 *
 * Since: 1.0
 */
EmeusConstraint *
emeus_constraint_new (gpointer                 target_object,
                      EmeusConstraintAttribute target_attribute,
                      EmeusConstraintRelation  relation,
                      gpointer                 source_object,
                      EmeusConstraintAttribute source_attribute,
                      double                   multiplier,
                      double                   constant,
                      EmeusConstraintStrength  strength)
{
  g_return_val_if_fail (target_object == NULL || GTK_IS_WIDGET (target_object), NULL);
  g_return_val_if_fail (source_object == NULL || GTK_IS_WIDGET (source_object), NULL);

  return g_object_new (EMEUS_TYPE_CONSTRAINT,
                       "target-object", target_object,
                       "target-attribute", target_attribute,
                       "relation", relation,
                       "source-object", source_object,
                       "source-attribute", source_attribute,
                       "multiplier", multiplier,
                       "constant", constant,
                       "strength", strength,
                       NULL);
}

/**
 * emeus_constraint_new_constant: (constructor)
 * @target_object: (type Gtk.Widget) (nullable): the target widget,
 *   or %NULL for the parent layout
 * @target_attribute: the attribute to set on the target widget
 * @relation: the relation between the target and the constant
 * @constant: the constant value of the constraint
 *
 * Creates a new constant constraint.
 *
 * This function is the equivalent of creating a new #EmeusConstraint with:
 *
 *  - #EmeusConstraint:source_object set to %NULL
 *  - #EmeusConstraint:source_attribute set to %EMEUS_CONSTRAINT_ATTRIBUTE_INVALID
 *  - #EmeusConstraint:multiplier set to 1.0
 *
 * Returns: (transfer full): the newly created constraint
 *
 * Since: 1.0
 */
EmeusConstraint *
emeus_constraint_new_constant (gpointer                 target_object,
                               EmeusConstraintAttribute target_attribute,
                               EmeusConstraintRelation  relation,
                               double                   constant,
                               EmeusConstraintStrength  strength)
{
  g_return_val_if_fail (target_object == NULL || GTK_IS_WIDGET (target_object), NULL);

  return g_object_new (EMEUS_TYPE_CONSTRAINT,
                       "target-object", target_object,
                       "target-attribute", target_attribute,
                       "relation", relation,
                       "source-object", NULL,
                       "source-attribute", EMEUS_CONSTRAINT_ATTRIBUTE_INVALID,
                       "multiplier", 1.0,
                       "constant", constant,
                       "strength", strength,
                       NULL);
}

/**
 * emeus_constraint_get_target_object:
 * @constraint: a #EmeusConstraint
 *
 * Retrieves the target object of the constraint.
 *
 * This function may return %NULL if the @constraint is not attached
 * to a #EmeusConstraintLayout. Use emeus_constraint_is_attached() to
 * before calling this function.
 *
 * Returns: (transfer none) (type Gtk.Widget) (nullable): the target
 *   object
 *
 * Since: 1.0
 */
gpointer
emeus_constraint_get_target_object (EmeusConstraint *constraint)
{
  g_return_val_if_fail (EMEUS_IS_CONSTRAINT (constraint), NULL);

  return constraint->target_object;
}

/**
 * emeus_constraint_get_target_attribute:
 * @constraint: a #EmeusConstraint
 *
 * Retrieves the attribute of the target object bound by this @constraint.
 *
 * Returns: a constraint attribute
 *
 * Since: 1.0
 */
EmeusConstraintAttribute
emeus_constraint_get_target_attribute (EmeusConstraint *constraint)
{
  g_return_val_if_fail (EMEUS_IS_CONSTRAINT (constraint), EMEUS_CONSTRAINT_ATTRIBUTE_INVALID);

  return constraint->target_attribute;
}

/**
 * emeus_constraint_get_relation:
 * @constraint: a #EmeusConstraint
 *
 * Retrieves the relation between the target and source attributes.
 *
 * Returns: a constraint relation
 *
 * Since: 1.0
 */
EmeusConstraintRelation
emeus_constraint_get_relation (EmeusConstraint *constraint)
{
  g_return_val_if_fail (EMEUS_IS_CONSTRAINT (constraint), EMEUS_CONSTRAINT_RELATION_EQ);

  return constraint->relation;
}

/**
 * emeus_constraint_get_source_object:
 * @constraint: a #EmeusConstraint
 *
 * Retrieves the source object of the @constraint.
 *
 * Returns: (transfer none) (nullable) (type Gtk.Widget): the source object
 */
gpointer
emeus_constraint_get_source_object (EmeusConstraint *constraint)
{
  g_return_val_if_fail (EMEUS_IS_CONSTRAINT (constraint), NULL);

  return constraint->source_object;
}

/**
 * emeus_constraint_get_source_attribute:
 * @constraint: a #EmeusConstraint
 *
 * Retrieves the attribute of the source object bound by this @constraint.
 *
 * Returns: a constraint attribute
 *
 * Since: 1.0
 */
EmeusConstraintAttribute
emeus_constraint_get_source_attribute (EmeusConstraint *constraint)
{
  g_return_val_if_fail (EMEUS_IS_CONSTRAINT (constraint), EMEUS_CONSTRAINT_ATTRIBUTE_INVALID);

  return constraint->source_attribute;
}

/**
 * emeus_constraint_get_multiplier:
 * @constraint: a #EmeusConstraint
 *
 * Retrieves the multiplication factor of the @constraint.
 *
 * Returns: a factor
 *
 * Since: 1.0
 */
double
emeus_constraint_get_multiplier (EmeusConstraint *constraint)
{
  g_return_val_if_fail (EMEUS_IS_CONSTRAINT (constraint), 1.0);

  return constraint->multiplier;
}

/**
 * emeus_constraint_get_constant:
 * @constraint: a #EmeusConstraint
 *
 * Retrieves the additional constant of the @constraint.
 *
 * Returns: a constant
 *
 * Since: 1.0
 */
double
emeus_constraint_get_constant (EmeusConstraint *constraint)
{
  g_return_val_if_fail (EMEUS_IS_CONSTRAINT (constraint), 0.0);

  return constraint->constant;
}

/**
 * emeus_constraint_get_strength:
 * @constraint: a #EmeusConstraint
 *
 * Retrieves the strength of the @constraint.
 *
 * Returns: a constraint strength
 *
 * Since: 1.0
 */
EmeusConstraintStrength
emeus_constraint_get_strength (EmeusConstraint *constraint)
{
  g_return_val_if_fail (EMEUS_IS_CONSTRAINT (constraint), EMEUS_CONSTRAINT_STRENGTH_REQUIRED);

  return constraint->strength;
}

/**
 * emeus_constraint_is_required:
 * @constraint: a #EmeusConstraint
 *
 * Checks whether a @constraint is marked as required.
 *
 * Returns: %TRUE if the constraint is required
 *
 * Since: 1.0
 */
gboolean
emeus_constraint_is_required (EmeusConstraint *constraint)
{
  g_return_val_if_fail (EMEUS_IS_CONSTRAINT (constraint), FALSE);

  return constraint->strength == EMEUS_CONSTRAINT_STRENGTH_REQUIRED;
}

const char *
emeus_constraint_to_string (EmeusConstraint *constraint)
{
  GString *buf;

  if (constraint->description != NULL)
    return constraint->description;

  buf = g_string_new (NULL);

  if (constraint->target_object != NULL)
    {
      g_string_append (buf, G_OBJECT_TYPE_NAME (constraint->target_object));
      g_string_append (buf, ".");
    }

  g_string_append (buf, get_attribute_name (constraint->target_attribute));
  g_string_append (buf, " ");
  g_string_append (buf, get_relation_symbol (constraint->relation));
  g_string_append (buf, " ");

  if (constraint->target_attribute != EMEUS_CONSTRAINT_ATTRIBUTE_INVALID)
    {
      if (constraint->source_object != NULL)
        g_string_append (buf, G_OBJECT_TYPE_NAME (constraint->source_object));
      else
        g_string_append (buf, "parent");

      g_string_append (buf, ".");
      g_string_append (buf, G_OBJECT_TYPE_NAME (constraint->source_attribute));

      if (fabs (constraint->multiplier - 1.0) > DBL_EPSILON)
        g_string_append_printf (buf, " * %g", constraint->multiplier);

      if (fabs (constraint->constant - 0.0) > DBL_EPSILON)
        g_string_append (buf, " + ");
    }

  g_string_append_printf (buf, "%g", constraint->constant);

  constraint->description = g_string_free (buf, FALSE);

  return constraint->description;
}

gboolean
emeus_constraint_attach (EmeusConstraint       *constraint,
                         EmeusConstraintLayout *layout,
                         gpointer               target_object)
{
  constraint->target_object = target_object;
  constraint->solver = emeus_constraint_layout_get_solver (layout);

  return TRUE;
}

void
emeus_constraint_detach (EmeusConstraint *constraint)
{
  if (constraint->constraint != NULL)
    simplex_solver_remove_constraint (constraint->solver, constraint->constraint);

  constraint->constraint = NULL;
  constraint->target_object = NULL;
  constraint->solver = NULL;
}

/**
 * emeus_constraint_is_attached:
 * @constraint: a #EmeusConstraint
 *
 * Checks whether the @constraint is attached to a child of a
 * #EmeusConstraintLayout.
 *
 * Returns: %TRUE if the constraint is attached
 *
 * Since: 1.0
 */
gboolean
emeus_constraint_is_attached (EmeusConstraint *constraint)
{
  g_return_val_if_fail (EMEUS_IS_CONSTRAINT (constraint), FALSE);

  return constraint->solver != NULL;
}
