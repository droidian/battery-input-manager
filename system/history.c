/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "history.h"
#include "config.h"

#define HISTORY_PATH VAR_LIB_DIR "/history.dat"

struct _HistoryPrivate {
    void *null;
};

G_DEFINE_TYPE_WITH_CODE (
    History,
    history,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (History)
)

static void
append_string_to_log (GString *string)
{
    g_autoptr(GFile) file;
    g_autoptr(GOutputStream) stream;
    GError *error;

    file = g_file_new_for_path (HISTORY_PATH);
    stream = G_OUTPUT_STREAM (
        g_file_append_to (file, G_FILE_CREATE_NONE, NULL, &error)
    );

    if (stream == NULL) {
        g_error ("Can't write to history: %s", error->message);
        g_error_free (error);
    } else {
        gboolean success = g_output_stream_write_all (
            stream,
            string->str,
            string->len,
            NULL,
            NULL,
            &error
        );
        if (!success) {
            g_error ("Can't write to history: %s", error->message);
            g_error_free (error);
        }
    }
}

static const gchar*
event_to_string (HistoryEvent event)
{
    switch (event) {
        case HISTORY_EVENT_ALARM_ADDED:
            return "alarm_added";
        case HISTORY_EVENT_ALARM_DELETED:
            return "alarm_deleted";
        case HISTORY_EVENT_INPUT_RESUMED:
            return "input_resumed";
        case HISTORY_EVENT_INPUT_SUSPENDED:
            return "input_suspended";
        case HISTORY_EVENT_BATTERY_PERCENT:
            return "battery_percent";
        default:
            return "invalid";
    }
}

static void
history_dispose (GObject *history)
{
    History *self = HISTORY (history);

    g_free (self->priv);

    G_OBJECT_CLASS (history_parent_class)->dispose (history);
}

static void
history_class_init (HistoryClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = history_dispose;
}

static void
history_init (History *self)
{
    g_autoptr(GFile) file;
    g_autoptr(GOutputStream) stream;
    GError *error;

    file = g_file_new_for_path (HISTORY_PATH);
    stream = G_OUTPUT_STREAM (
        g_file_replace (file,
                        NULL,
                        FALSE,
                        G_FILE_CREATE_REPLACE_DESTINATION,
                        NULL,
                        &error
        )
    );

    if (stream == NULL) {
        g_error ("Can't reset history: %s", error->message);
        g_error_free (error);
    }

    self->priv = history_get_instance_private (self);
}

/**
 * history_new:
 *
 * Creates a new #History
 *
 * Returns: (transfer full): a new #History
 *
 **/
GObject *
history_new (void)
{
    GObject *history;

    history = g_object_new (TYPE_HISTORY, NULL);

    return history;
}

/**
 * history_add_event:
 * @self: a #History
 * @event: a #HistoryEvent
 * @custom_field: the value of the first custom field
 * @...: the value of next custom fields, followed by %NULL
 *
 * Add a new event
 *
 **/
void
history_add_event (History *self, HistoryEvent event,
                   const gchar *custom_field, ...)
{
    g_autoptr(GString) string;
    g_autoptr(GDateTime) datetime;
    g_autofree gchar *timestamp;
    const gchar *value;
    va_list var_args;

    datetime = g_date_time_new_now_utc ();
    timestamp = g_strdup_printf ("%ld", g_date_time_to_unix (datetime));

    string = g_string_new (timestamp);
    g_string_append (string, ":");
    g_string_append (string, event_to_string (event));

    va_start (var_args, custom_field);
    value = custom_field;

    do {
        if (value == NULL)
            break;

        g_string_append (string, ":");
        g_string_append (string, value);
    } while ((value = va_arg (var_args, const gchar *)));

    g_string_append (string, "\n");

    append_string_to_log (string);

    va_end (var_args);
}

static History *default_history = NULL;
/**
 * history_get_default:
 *
 * Gets the default #History.
 *
 * Return value: (transfer full): the default #History.
 */
History *
history_get_default (void)
{
    if (!default_history) {
        default_history = HISTORY (history_new ());
    }
    return g_object_ref (default_history);
}
