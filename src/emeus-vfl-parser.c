#include "config.h"

#include "emeus-vfl-parser-private.h"

#include <string.h>

typedef enum {
  VFL_HORIZONTAL,
  VFL_VERTICAL
} VflOrientation;

typedef struct {
  OperatorType relation;

  double constant;
  char *object;
  const char *attr;

  StrengthType priority;
} VflPredicate;

typedef struct {
  double size;
  bool is_set;
  bool is_default;
  bool is_predicate;
  VflPredicate predicate;
} VflSpacing;

typedef struct _VflView VflView;

struct _VflView
{
  char *name;

  /* Decides which attributes are admissible */
  VflOrientation orientation;

  /* A set of predicates, which will be used to
   * set up constraints
   */
  GArray *predicates;

  VflSpacing spacing;

  VflView *prev_view;
  VflView *next_view;
};

struct _VflParser
{
  char *buffer;
  gsize buffer_len;

  int default_spacing[2];

  const char *cursor;

  /* Decides which attributes are admissible */
  VflOrientation orientation;

  VflView *leading_super;
  VflView *trailing_super;

  VflView *current_view;
  VflView *views;
};

GQuark
vfl_error_quark (void)
{
  return g_quark_from_static_string ("vfl-error-quark");
}

VflParser *
vfl_parser_new (void)
{
  VflParser *res = g_slice_new0 (VflParser);

  res->default_spacing[VFL_HORIZONTAL] = 8;
  res->default_spacing[VFL_VERTICAL] = 8;

  res->orientation = VFL_HORIZONTAL;

  return res;
}

void
vfl_parser_set_default_spacing (VflParser *parser,
                                int hspacing,
                                int vspacing)
{
  parser->default_spacing[VFL_HORIZONTAL] = hspacing;
  parser->default_spacing[VFL_VERTICAL] = vspacing;
}

static int
get_default_spacing (VflParser *parser)
{
  return parser->default_spacing[parser->orientation];
}

/* Valid attribute names, depending on the orientation */
static const struct {
  const char *attributes[8];
} valid_attributes[2] = {
  [VFL_HORIZONTAL] = {
    { "width", "centerX", "left", "right", "start", "end", NULL, },
  },

  [VFL_VERTICAL] = {
    { "height", "centerY", "top", "bottom", "baseline", "start", "end", NULL, },
  },
};

/* Default attributes, if unnamed, depending on the orientation */
static const char *default_attribute[2] = {
  [VFL_HORIZONTAL] = "width",
  [VFL_VERTICAL] = "height",
};

static bool
is_valid_attribute (VflOrientation orientation,
                    const char *str,
                    const char **attrptr,
                    char **endptr)
{
  const char * const *attributes = valid_attributes[orientation].attributes;
  const char *attr;
  int i = 0;

  attr = attributes[0];
  while (attr != NULL)
    {
      int len = strlen (attr);

      if (g_ascii_strncasecmp (attr, str, len) == 0)
        {
          *attrptr = attr;
          *endptr = (char *) str + len;
          return true;
        }

      attr = attributes[++i];
    }

  return false;
}

static bool
parse_relation (const char *str,
                OperatorType *relation,
                char **endptr,
                GError **error)
{
  const char *cur = str;

  if (*cur == '=')
    {
      cur += 1;

      if (*cur == '=')
        {
          *relation = OPERATOR_TYPE_EQ;
          *endptr = (char *) cur + 1;
          return true;
        }

      goto out;
    }
  else if (*cur == '>')
    {
      cur += 1;

      if (*cur == '=')
        {
          *relation = OPERATOR_TYPE_GE;
          *endptr = (char *) cur + 1;
          return true;
        }

      goto out;
    }
  else if (*cur == '<')
    {
      cur += 1;

      if (*cur == '=')
        {
          *relation = OPERATOR_TYPE_LE;
          *endptr = (char *) cur + 1;
          return true;
        }

      goto out;
    }

out:
  g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_RELATION,
               "Unknown relation; must be one of '==', '>=', or '<='");
  return false;
}

static bool
parse_predicate (VflParser *parser,
                 const char *cursor,
                 VflPredicate *predicate,
                 char **endptr,
                 GError **error)
{
  VflOrientation orientation = parser->orientation;
  const char *end = cursor;
  char *object = NULL;

  /*         <predicate> = (<relation>)? (<objectOfPredicate>) ('@'<priority>)?
   *          <relation> = '==' | '<=' | '>='
   * <objectOfPredicate> = <constant> | <metric>
   *          <constant> = <number>
   *            <metric> = <attrName> | <viewName> ('.'<attrName>)?
   *          <attrName> = [A-Za-z_]([A-Za-z0-9_]+)
   *          <priority> = 'weak' | 'medium' | 'strong' | 'required'
   */

  /* Parse relation */
  if (*end == '=' || *end == '>' || *end == '<')
    {
      OperatorType relation;
      char *tmp;

      if (!parse_relation (end, &relation, &tmp, error))
        return false;

      predicate->relation = relation;

      end = tmp;
    }
  else
    predicate->relation = OPERATOR_TYPE_EQ;

  /* Parse object of predicate */
  if (g_ascii_isdigit (*end))
    {
      char *tmp;

      /* <constant> */
      predicate->object = NULL;
      predicate->attr = default_attribute[orientation];
      predicate->constant = g_ascii_strtod (end, &tmp);

      end = tmp;
    }
  else if (g_ascii_isalpha (*end) || *end == '_')
    {
      const char *attr;
      char *tmp;

      /* <attrName> */
      if (is_valid_attribute (orientation, end, &attr, &tmp))
        end = tmp;
      else
        {
          char *dot = strchr (end, '.');

          if (dot != NULL)
            {
              /* <viewName>.<attrName> */
              object = g_strndup (end, dot - end);
              end = dot + 1;

              if (is_valid_attribute (orientation, end, &attr, &tmp))
                end = tmp;
              else
                {
                  g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_ATTRIBUTE,
                               "Unexpected attribute after dot notation");
                  g_free (object);
                  return false;
                }
            }
          else
            {
              tmp = (char *) end;

              /* <viewName> */
              while (g_ascii_isalnum (*tmp) || *tmp == '_')
                tmp += 1;

              object = g_strndup (end, tmp - end);

              attr = default_attribute[orientation];

              end = tmp;
            }
        }

      predicate->attr = attr;
      predicate->constant = 0.0;
    }
  else
    {
      g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                   "Expected constant, view name, or attribute");
      return false;
    }

  /* Parse priority */
  if (*end == '@')
    {
      StrengthType priority;
      end += 1;

      if (strncmp (end, "weak", 4) == 0)
        {
          priority = STRENGTH_WEAK;
          end += 4;
        }
      else if (strncmp (end, "medium", 6) == 0)
        {
          priority = STRENGTH_MEDIUM;
          end += 6;
        }
      else if (strncmp (end, "strong", 6) == 0)
        {
          priority = STRENGTH_STRONG;
          end += 6;
        }
      else if (strncmp (end, "required", 8) == 0)
        {
          priority = STRENGTH_REQUIRED;
          end += 8;
        }
      else
        {
          g_free (object);
          g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_PRIORITY,
                       "Priority must be one of 'weak', 'medium', 'strong', and 'required'");
          return false;
        }

      predicate->priority = priority;
    }
  else
    predicate->priority = STRENGTH_REQUIRED;

  predicate->object = object;

  if (endptr != NULL)
    *endptr = (char *) end;

  return true;
}

static bool
parse_view (VflParser *parser,
            const char *cursor,
            VflView *view,
            char **endptr,
            GError **error)
{
  const char *end = cursor;

  /*     <view> = '[' <viewName> (<predicateListWithParens>)? ']'
   * <viewName> = [A-Za-z_]([A-Za-z0-9_]+)
   */

  g_assert (*end == '[');

  /* Skip '[' */
  end += 1;

  if (!(g_ascii_isalpha (*end) || *end == '_'))
    {
      g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_VIEW,
                   "View identifiers must be valid C identifiers");
      return false;
    }

  while (g_ascii_isalnum (*end))
    end += 1;

  if (*end == '\0')
    {
      g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                   "A view must end with ']'");
      return false;
    }

  view->name = g_strndup (cursor + 1, end - cursor - 1);
  view->predicates = g_array_new (FALSE, FALSE, sizeof (VflPredicate));

  if (*end == ']')
    {
      if (endptr != NULL)
        *endptr = (char *) end + 1;

      return true;
    }

  /* <predicateListWithParens> = '(' <predicate> (',' <predicate>)* ')' */
  if (*end != '(')
    {
      g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                   "A predicate must follow a view name");
      return false;
    }

  end += 1;

  while (*end != '\0')
    {
      VflPredicate cur_predicate;
      char *tmp;

      if (*end == ']' || *end == '\0')
        {
          g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                   "A predicate on a view must end with ')'");
          return false;
        }

      if (!parse_predicate (parser, end, &cur_predicate, &tmp, error))
        return false;

      end = tmp;

#if 0
      g_print ("*** Found predicate: %s.%s %s %g %s\n",
               cur_predicate.object != NULL ? cur_predicate.object : view->name,
               cur_predicate.attr,
               cur_predicate.relation == OPERATOR_TYPE_EQ ? "==" :
               cur_predicate.relation == OPERATOR_TYPE_LE ? "<=" :
               cur_predicate.relation == OPERATOR_TYPE_GE ? ">=" :
               "unknown relation",
               cur_predicate.constant,
               cur_predicate.priority == STRENGTH_WEAK ? "weak" :
               cur_predicate.priority == STRENGTH_MEDIUM ? "medium" :
               cur_predicate.priority == STRENGTH_STRONG ? "strong" :
               cur_predicate.priority == STRENGTH_REQUIRED ? "required" :
               "unknown strength");
#endif

      g_array_append_val (view->predicates, cur_predicate);

      /* If the predicate is a list, iterate again */
      if (*end == ',')
        {
          end += 1;
          continue;
        }

      /* We reached the end of the predicate */
      if (*end == ')')
        {
          end += 1;
          break;
        }

      g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                   "Expected ')' at the end of a predicate, not '%c'", *end);
      return false;
    }

  if (*end != ']')
    {
      g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                   "Expected ']' at the end of a view, not '%c'", *end);
      return false;
    }

  if (endptr != NULL)
    *endptr = (char *) end + 1;

  return true;
}

static void
vfl_view_free (VflView *view)
{
  if (view == NULL)
    return;

  g_free (view->name);

  if (view->predicates != NULL)
    {
      for (int i = 0; i < view->predicates->len; i++)
        {
          VflPredicate *p = &g_array_index (view->predicates, VflPredicate, i);

          g_free (p->object);
        }

      g_array_free (view->predicates, TRUE);
      view->predicates = NULL;
    }

  view->prev_view = NULL;
  view->next_view = NULL;

  g_slice_free (VflView, view);
}

void
vfl_parser_free (VflParser *parser)
{
  if (parser == NULL)
    return;

  VflView *iter = parser->views; 
  while (iter != NULL)
    {
      VflView *next = iter->next_view;

      vfl_view_free (iter);

      iter = next;
    }

  g_free (parser->buffer);

  g_slice_free (VflParser, parser);
}

bool
vfl_parser_parse_line (VflParser *parser,
                       const char *buffer,
                       gssize len,
                       GError **error)
{
  g_free (parser->buffer);

  if (len > 0)
    {
      parser->buffer = g_strndup (buffer, len);
      parser->buffer_len = len;
    }
  else
    {
      parser->buffer = g_strdup (buffer);
      parser->buffer_len = strlen (buffer);
    }

  parser->cursor = parser->buffer;

  const char *cur = parser->cursor;

  /* Skip leading whitespace */
  while (g_ascii_isspace (*cur))
    cur += 1;

  /* Check orientation; if none is specified, then we assume horizontal */
  parser->orientation = VFL_HORIZONTAL;
  if (*cur == 'H')
    {
      cur += 1;

      if (*cur != ':')
        {
          g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                       "Expected ':' after horizontal orientation");
          return false;
        }

      parser->orientation = VFL_HORIZONTAL;
      cur += 1;
    }
  else if (*cur == 'V')
    {
      cur += 1;

      if (*cur != ':')
        {
          g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                       "Expected ':' after vertical orientation");
          return false;
        }

      parser->orientation = VFL_VERTICAL;
      cur += 1;
    }

  while (*cur != '\0')
    {
      /* Super-view */
      if (*cur == '|')
        {
          if (parser->current_view != NULL && parser->leading_super == NULL)
            {
              g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                           "Super view definitions cannot follow child views");
              return false;
            }

          if (parser->leading_super == NULL)
            {
              parser->leading_super = g_slice_new0 (VflView);

              parser->leading_super->name = g_strdup ("super");
              parser->leading_super->orientation = parser->orientation;

              parser->current_view = parser->leading_super;
              parser->views = parser->leading_super;
            }
          else if (parser->trailing_super == NULL)
            {
              parser->trailing_super = g_slice_new0 (VflView);

              parser->trailing_super->name = g_strdup ("super");
              parser->trailing_super->orientation = parser->orientation;

              parser->current_view->next_view = parser->trailing_super;
              parser->trailing_super->prev_view = parser->current_view;

              parser->current_view = parser->trailing_super;

              /* If we reached the second '|' then we completed a line
               * of layout, and we can stop
               */
              break;
            }
          else
            {
              g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                           "Super views can only appear at the beginning "
                           "and end of the layout, and not multiple times");
              return false;
            }

          cur += 1;

          continue;
        }

      /* Spacing */
      if (*cur == '-')
        {
          if (*(cur + 1) == '\0')
            {
              g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                           "Unterminated spacing");
              return false;
            }

          if (parser->current_view == NULL)
            {
              g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                           "Spacing cannot be set without a view");
              return false;
            }

          if (*(cur + 1) == '|' || *(cur + 1) == '[')
            {
              VflSpacing *spacing = &(parser->current_view->spacing);

              /* Default spacer */
              spacing->is_set = true;
              spacing->is_default = true;
              spacing->is_predicate = false;
              spacing->size = 0;

              cur += 1;

              continue;
            }
          else if (*(cur + 1) == '(')
            {
              VflPredicate *predicate;
              VflSpacing *spacing;
              char *tmp;

              /* Predicate */
              cur += 1;

              spacing = &(parser->current_view->spacing);
              spacing->is_set = true;
              spacing->is_default = false;
              spacing->is_predicate = true;
              spacing->size = 0;
              predicate = &(spacing->predicate);

              cur += 1;
              if (!parse_predicate (parser, cur, predicate, &tmp, error))
                return false;

              if (*tmp != ')')
                {
                  g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                                "Expected ')' at the end of a predicate, not '%c'", *tmp);
                  return false;
                }

              cur = tmp + 1;
              if (*cur != '-')
                {
                  g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                               "Explicit spacing must be followed by '-'");
                  return false;
                }

              cur += 1;

              continue;
            }
          else if (g_ascii_isdigit (*(cur + 1)))
            {
              VflSpacing *spacing;
              char *tmp;

              /* Explicit spacing */
              cur += 1;

              spacing = &(parser->current_view->spacing);
              spacing->is_set = true;
              spacing->is_default = false;
              spacing->is_predicate = false;
              spacing->size = g_ascii_strtod (cur, &tmp);

              if (tmp == cur)
                {
                  g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                               "Spacing must be a number");
                  return false;
                }

              if (*tmp != '-')
                {
                  g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                               "Explicit spacing must be followed by '-'");
                  return false;
                }

              cur = tmp + 1;

              continue;
            }
          else
            {
              g_set_error (error, VFL_ERROR, VFL_ERROR_INVALID_SYMBOL,
                           "Spacing can either be '-' or a number");
              return false;
            }
        }

      if (*cur == '[')
        {
          VflView *view = g_slice_new0 (VflView);
          char *tmp;

          view->orientation = parser->orientation;

          if (!parse_view (parser, cur, view, &tmp, error))
            {
              vfl_view_free (view);
              return false;
            }

          cur = tmp;

          if (parser->views == NULL)
            parser->views = view;

          view->prev_view = parser->current_view;

          if (parser->current_view != NULL)
            parser->current_view->next_view = view;

          parser->current_view = view;

          continue;
        }

      cur += 1;
    }

  return true;
}

VflConstraint *
vfl_parser_get_constraints (VflParser *parser,
                            int *n_constraints)
{
  GArray *constraints;
  VflView *iter;

  constraints = g_array_new (FALSE, FALSE, sizeof (VflConstraint));

  iter = parser->views;
  while (iter != NULL)
    {
      VflConstraint c;

      if (iter->predicates != NULL)
        {
          for (int i = 0; i < iter->predicates->len; i++)
            {
              const VflPredicate *p = &g_array_index (iter->predicates, VflPredicate, i);

              c.view1 = iter->name;
              c.attr1 = iter->orientation == VFL_HORIZONTAL ? "width" : "height";
              if (p->object != NULL)
                {
                  c.view2 = p->object;
                  c.attr2 = p->attr;
                }
              else
                {
                  c.view2 = NULL;
                  c.attr2 = NULL;
                }
              c.relation = p->relation;
              c.constant = p->constant;
              c.multiplier = 1.0;
              c.strength = p->priority;

              g_array_append_val (constraints, c);
            }
        }

      if (iter->spacing.is_set)
        {
          c.view1 = iter->name;

          /* If this is the first view, we need to anchor the leading edge */
          if (iter == parser->leading_super)
            c.attr1 = iter->orientation == VFL_HORIZONTAL ? "start" : "top";
          else
            c.attr1 = iter->orientation == VFL_HORIZONTAL ? "end" : "bottom";

          c.view2 = iter->next_view != NULL ? iter->next_view->name : "super";

          if (iter == parser->trailing_super || iter->next_view == parser->trailing_super)
            c.attr2 = iter->orientation == VFL_HORIZONTAL ? "end" : "bottom";
          else
            c.attr2 = iter->orientation == VFL_HORIZONTAL ? "start" : "top";

          if (iter->spacing.is_predicate)
            {
              const VflPredicate *p = &(iter->spacing.predicate);

              c.constant = p->constant;
              c.relation = p->relation;
              c.strength = p->priority;
            }
          else if (iter->spacing.is_default)
            {
              c.constant = get_default_spacing (parser);
              c.relation = OPERATOR_TYPE_EQ;
              c.strength = STRENGTH_REQUIRED;
            }
          else
            {
              c.constant = iter->spacing.size * -1.0;
              c.relation = OPERATOR_TYPE_EQ;
              c.strength = STRENGTH_REQUIRED;
            }

          c.multiplier = 1.0;

          g_array_append_val (constraints, c);
        }
      else if (iter->next_view != NULL)
        {
          c.view1 = iter->name;

          /* If this is the first view, we need to anchor the leading edge */
          if (iter == parser->leading_super)
            c.attr1 = iter->orientation == VFL_HORIZONTAL ? "start" : "top";
          else
            c.attr1 = iter->orientation == VFL_HORIZONTAL ? "end" : "bottom";

          c.relation = OPERATOR_TYPE_EQ;

          c.view2 = iter->next_view->name;

          if (iter == parser->trailing_super || iter->next_view == parser->trailing_super)
            c.attr2 = iter->orientation == VFL_HORIZONTAL ? "end" : "bottom";
          else
            c.attr2 = iter->orientation == VFL_HORIZONTAL ? "start" : "top";

          c.constant = 0.0;
          c.multiplier = 1.0;
          c.strength = STRENGTH_REQUIRED;

          g_array_append_val (constraints, c);
        }

      iter = iter->next_view;
    }

  if (n_constraints != NULL)
    *n_constraints = constraints->len;

#if 0
  for (int i = 0; i < constraints->len; i++)
    {
      const VflConstraint *c = &g_array_index (constraints, VflConstraint, i);

      g_print ("{\n"
               "  .view1: '%s',\n"
               "  .attr1: '%s',\n"
               "  .relation: '%d',\n"
               "  .view2: '%s',\n"
               "  .attr2: '%s',\n"
               "  .constant: %g,\n"
               "  .multiplier: %g,\n"
               "  .strength: %d\n"
               "}\n",
               c->view1, c->attr1,
               c->relation,
               c->view2 != NULL ? c->view2 : "none", c->attr2 != NULL ? c->attr2 : "none",
               c->constant,
               c->multiplier,
               c->strength);
    }
#endif

  return (VflConstraint *) g_array_free (constraints, FALSE);
}
