#pragma once

#include "emeus-constraint.h"
#include "emeus-constraint-layout-private.h"
#include "emeus-types-private.h"

G_BEGIN_DECLS

struct _EmeusConstraint
{
  GInitiallyUnowned parent_instance;

  gpointer target_object;
  EmeusConstraintAttribute target_attribute;

  EmeusConstraintRelation relation;

  gpointer source_object;
  EmeusConstraintAttribute source_attribute;

  double multiplier;
  double constant;

  EmeusConstraintStrength strength;

  SimplexSolver *solver;
  Constraint *constraint;
};

void            emeus_constraint_attach                 (EmeusConstraint       *constraint,
                                                         EmeusConstraintLayout *layout);
void            emeus_constraint_detach                 (EmeusConstraint       *constraint);

Constraint *    emeus_constraint_get_real_constraint    (EmeusConstraint       *constraint);

G_END_DECLS
