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

static int opt_hspacing = -1;
static int opt_vspacing = -1;
static char **opt_views;
static char **opt_vfl;

static GOptionEntry options[] = {
  { "hspacing", 'H', 0, G_OPTION_ARG_INT, &opt_hspacing, "Default horizontal spacing", "SPACING" },
  { "vspacing", 'V', 0, G_OPTION_ARG_INT, &opt_vspacing, "Default vertical spacing", "SPACING" },
  { "view", 0, 0, G_OPTION_ARG_STRING_ARRAY, &opt_views, "Views", "NAME" },

  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &opt_vfl, "Visual Format Language strings", "FORMAT" },

  { NULL, },
};

int
main (int argc, char *argv[])
{
  g_set_prgname ("emeus-gen-constraints");

  if (argc < 2)
    {
      print_usage (argv[0]);
      die ();
    }

  GOptionContext *context = g_option_context_new (" - Generate constraint descriptions");
  g_option_context_add_main_entries (context, options, NULL);
  g_option_context_set_help_enabled (context, TRUE);
  g_option_context_set_ignore_unknown_options (context, FALSE);

  GError *error = NULL;
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      fprintf (stderr, "ERROR: %s\n", error->message);
      print_usage (argv[0]);
      die ();
    }

  GHashTable *views = g_hash_table_new (g_str_hash, g_str_equal);

  for (int i = 0; opt_views[i] != NULL; i++)
    g_hash_table_add (views, opt_views[i]);

  VflParser *parser = vfl_parser_new ();

  vfl_parser_set_default_spacing (parser, opt_hspacing, opt_vspacing);
  vfl_parser_set_metrics (parser, NULL);
  vfl_parser_set_views (parser, views);

  fprintf (stdout, "<constraints>\n");

  for (int i = 0; opt_vfl[i] != NULL; i++)
    {
      GError *error = NULL;

      vfl_parser_parse_line (parser, opt_vfl[i], -1, &error);
      if (error != NULL)
        {
          int offset = vfl_parser_get_error_offset (parser);
          int range = vfl_parser_get_error_range (parser);

          char *squiggly = NULL;

          if (range > 0)
            {
              squiggly = g_new (char, range + 1);
              for (int s = 0; s < range; s++)
                squiggly[s] = '~';
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

      for (int j = 0; j < n_constraints; j++)
        {
          VflConstraint *c = &constraints[j];

          fprintf (stdout, "  <constraint target-object=\"%s\" target-attr=\"%s\"\n", c->view1, c->attr1);
          fprintf (stdout, "              relation=\"%s\"\n", relation_to_string (c->relation));

          if (c->view2 != NULL)
            fprintf (stdout, "              source-object=\"%s\" source-attr=\"%s\"\n", c->view2, c->attr2);

          fprintf (stdout, "              constant=\"%g\" multiplier=\"%g\"\n", c->constant, c->multiplier);
          fprintf (stdout, "              strength=\"%s\"/>\n", strength_to_string (c->strength));
        }

      g_free (constraints);
    }

  fprintf (stdout, "</constraints>\n");

  vfl_parser_free (parser);
}
