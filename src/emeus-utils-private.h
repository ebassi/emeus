#pragma once

#include "emeus-types.h"
#include "emeus-types-private.h"

G_BEGIN_DECLS

const char *get_attribute_name (EmeusConstraintAttribute attr);
const char *get_relation_symbol (EmeusConstraintRelation rel);

OperatorType relation_to_operator (EmeusConstraintRelation rel);
StrengthType strength_to_value (EmeusConstraintStrength strength);

G_END_DECLS
