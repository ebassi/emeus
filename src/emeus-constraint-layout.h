#pragma once

#include <emeus-types.h>
#include <emeus-constraint.h>

G_BEGIN_DECLS

#define EMEUS_TYPE_CONSTRAINT_LAYOUT (emeus_constraint_layout_get_type())

EMEUS_AVAILABLE_IN_1_0
G_DECLARE_FINAL_TYPE (EmeusConstraintLayout, emeus_constraint_layout, EMEUS, CONSTRAINT_LAYOUT, GtkContainer)

EMEUS_AVAILABLE_IN_1_0
GtkWidget *     emeus_constraint_layout_new     (void);

EMEUS_AVAILABLE_IN_1_0
void            emeus_constraint_layout_pack                    (EmeusConstraintLayout   *layout,
                                                                 GtkWidget               *child,
                                                                 EmeusConstraint         *first_constraint,
                                                                 ...) G_GNUC_NULL_TERMINATED;
EMEUS_AVAILABLE_IN_1_0
void            emeus_constraint_layout_child_add_constraint    (EmeusConstraintLayout   *layout,
                                                                 GtkWidget               *child,
                                                                 EmeusConstraint         *constraint);

G_END_DECLS
