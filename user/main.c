/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <stdlib.h>

#include "clocks.h"
#include "settings.h"


gint
main (gint argc, gchar * argv[])
{
    GMainLoop *loop;
    g_autoptr (GOptionContext) context = NULL;
    g_autoptr (GError) error = NULL;
    gboolean version = FALSE;
    gboolean simulate = FALSE;
    GOptionEntry main_entries[] = {
        {"simulate", 0, 0, G_OPTION_ARG_NONE, &simulate, "Simulate alarms"},
        {"version", 0, 0, G_OPTION_ARG_NONE, &version, "Show version"},
        {NULL}
    };

    context = g_option_context_new ("Battery Input Manager");
    g_option_context_add_main_entries (context, main_entries, NULL);

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("%s\n", error->message);
        return EXIT_FAILURE;
    }

    if (version) {
        g_printerr ("%s\n", PACKAGE_VERSION);
        return EXIT_SUCCESS;
    }

    settings_get_default ();
    clocks_get_default (simulate);

    loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (loop);

    g_clear_pointer (&loop, g_main_loop_unref);

    return EXIT_SUCCESS;
}
