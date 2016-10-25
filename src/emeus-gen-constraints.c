#include "config.h"

#include <glib.h>

#include "emeus-vfl-parser-private.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void die (void) G_GNUC_NORETURN;

static void
print_usage (const char *bin)
{
  fprintf (stderr, "%s - Generate constraints from VFL\n", bin);
  fprintf (stderr, "Usage: %s FORMAT\n", bin);
}

static void
die (void)
{
  exit (EXIT_FAILURE);
}

static const char *
relation_to_string (OperatorType rel)
{
  if (rel == OPERATOR_TYPE_LE)
    return "le";

  if (rel == OPERATOR_TYPE_GE)
    return "ge";

  return "eq";
}

static const char *
strength_to_string (StrengthType strength)
{
  if (strength == STRENGTH_REQUIRED)
    return "required";

  if (strength >= STRENGTH_STRONG)
    return "strong";

  if (strength >= STRENGTH_MEDIUM)
    return "medium";

  return "weak";
}

int
main (int argc, char *argv[])
{
  g_set_prgname ("emeus-gen-constraints");

  if (argc < 2)
    {
      print_usage (argv[0]);
      die ();
    }

  VflParser *parser = vfl_parser_new ();
  const char *format = argv[1];
  GError *error = NULL;

  vfl_parser_parse_line (parser, format, -1, &error);
  if (error != NULL)
    {
      int offset = vfl_parser_get_error_offset (parser);
      int range = vfl_parser_get_error_range (parser);

      char *squiggly = NULL;

      if (range > 0)
        {
          squiggly = g_new (char, range + 1);
          for (int i = 0; i < range; i++)
            squiggly[i] = '~';
          squiggly[range] = '\0';
        }

      fprintf (stderr, "%s: error: %s\n", argv[0], error->message);
      fprintf (stderr, "%s\n", argv[1]);
      fprintf (stderr, "%*s^%s\n", offset, " ", squiggly != NULL ? squiggly : "");

      g_free (squiggly);

      die ();
    }

  int n_constraints = 0;
  VflConstraint *constraints = vfl_parser_get_constraints (parser, &n_constraints);

  fprintf (stdout, "<constraints>\n");
  for (int i = 0; i < n_constraints; i++)
    {
      VflConstraint *c = &constraints[i];

      fprintf (stdout, "  <constraint target-object=\"%s\" target-attr=\"%s\"\n", c->view1, c->attr1);
      fprintf (stdout, "              relation=\"%s\"\n", relation_to_string (c->relation));

      if (c->view2 != NULL)
        fprintf (stdout, "              source-object=\"%s\" source-attr=\"%s\"\n", c->view2, c->attr2);

      fprintf (stdout, "              constant=\"%g\" multiplier=\"%g\"\n", c->constant, c->multiplier);
      fprintf (stdout, "              strength=\"%s\"/>\n", strength_to_string (c->strength));
    }
  fprintf (stdout, "</constraints>\n");

  g_free (constraints);
  vfl_parser_free (parser);
}
