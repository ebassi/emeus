#include "config.h"

#include "emeus-macros-private.h"

#include "emeus-types.h"

EMEUS_DEFINE_ENUM_TYPE (EmeusConstraintAttribute, emeus_constraint_attribute,
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_ATTRIBUTE_INVALID, "invalid")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_ATTRIBUTE_LEFT, "left")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_ATTRIBUTE_RIGHT, "right")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_ATTRIBUTE_TOP, "top")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_ATTRIBUTE_BOTTOM, "bottom")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_ATTRIBUTE_START, "start")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_ATTRIBUTE_END, "end")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH, "width")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_ATTRIBUTE_HEIGHT, "height")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_X, "center-x")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_Y, "center-y")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_ATTRIBUTE_BASELINE, "baseline"))

EMEUS_DEFINE_ENUM_TYPE (EmeusConstraintRelation, emeus_constraint_relation,
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_RELATION_GE, "le")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_RELATION_EQ, "eq")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_RELATION_LE, "ge"))

EMEUS_DEFINE_ENUM_TYPE (EmeusConstraintStrength, emeus_constraint_strength,
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_STRENGTH_WEAK, "weak")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_STRENGTH_MEDIUM, "medium")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_STRENGTH_STRONG, "strong")
                        EMEUS_ENUM_VALUE (EMEUS_CONSTRAINT_STRENGTH_REQUIRED, "required"))
