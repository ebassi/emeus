#include "config.h"

#include "emeus-utils-private.h"

static const char *attribute_names[] = {
  [EMEUS_CONSTRAINT_ATTRIBUTE_INVALID]  = "invalid",
  [EMEUS_CONSTRAINT_ATTRIBUTE_LEFT]     = "left",
  [EMEUS_CONSTRAINT_ATTRIBUTE_RIGHT]    = "right",
  [EMEUS_CONSTRAINT_ATTRIBUTE_TOP]      = "top",
  [EMEUS_CONSTRAINT_ATTRIBUTE_BOTTOM]   = "bottom",
  [EMEUS_CONSTRAINT_ATTRIBUTE_START]    = "start",
  [EMEUS_CONSTRAINT_ATTRIBUTE_END]      = "end",
  [EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH]    = "width",
  [EMEUS_CONSTRAINT_ATTRIBUTE_HEIGHT]   = "height",
  [EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_X] = "center-x",
  [EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_Y] = "center-y",
  [EMEUS_CONSTRAINT_ATTRIBUTE_BASELINE] = "center-y",
};

static const char *relation_symbols[] = {
  [EMEUS_CONSTRAINT_RELATION_EQ] = "==",
  [EMEUS_CONSTRAINT_RELATION_LE] = "<=",
  [EMEUS_CONSTRAINT_RELATION_GE] = ">=",
};

const char *
get_attribute_name (EmeusConstraintAttribute attr)
{
  return attribute_names[attr];
}

const char *
get_relation_symbol (EmeusConstraintRelation rel)
{
  return relation_symbols[rel];
}

OperatorType
relation_to_operator (EmeusConstraintRelation rel)
{
  switch (rel)
    {
    case EMEUS_CONSTRAINT_RELATION_EQ:
      return OPERATOR_TYPE_EQ;
    case EMEUS_CONSTRAINT_RELATION_LE:
      return OPERATOR_TYPE_LE;
    case EMEUS_CONSTRAINT_RELATION_GE:
      return OPERATOR_TYPE_GE;
    }

  return OPERATOR_TYPE_EQ;
}

StrengthType
strength_to_value (EmeusConstraintStrength strength)
{
  switch (strength)
    {
    case EMEUS_CONSTRAINT_STRENGTH_REQUIRED:
      return STRENGTH_REQUIRED;
    case EMEUS_CONSTRAINT_STRENGTH_WEAK:
      return STRENGTH_WEAK;
    case EMEUS_CONSTRAINT_STRENGTH_MEDIUM:
      return STRENGTH_MEDIUM;
    case EMEUS_CONSTRAINT_STRENGTH_STRONG:
      return STRENGTH_STRONG;
    }

  return STRENGTH_REQUIRED;
}
