#pragma once

#include "emeus-constraint-layout.h"
#include "emeus-types-private.h"

G_BEGIN_DECLS

SimplexSolver * emeus_constraint_layout_get_solver      (EmeusConstraintLayout *layout);

gboolean        emeus_constraint_layout_has_child_data  (EmeusConstraintLayout *layout,
                                                         GtkWidget             *widget);

G_END_DECLS
