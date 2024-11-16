/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>

#include "clocks.h"
#include "clocks_settings.h"
#include "config.h"
#include "d-bus.h"
#include "../common/utils.h"

#define CLOCKS_ID "org.gnome.clocks"

enum {
    PROP_0,
    PROP_SIMULATE
};

struct _ClocksPrivate {
    ClocksSettings *settings;
    GList *alarms;
    gboolean simulate;
};

G_DEFINE_TYPE_WITH_CODE (Clocks, clocks, G_TYPE_OBJECT,
    G_ADD_PRIVATE (Clocks))

static void
add_alarm (Clocks   *self,
           const gchar *clock_id,
           gint64 timestamp) {
    g_message("Adding alarm: %s", clock_id);

    self->priv->alarms = g_list_append (self->priv->alarms, g_strdup (clock_id));

    bim_bus_add_alarm (bim_bus_get_default (), clock_id, timestamp);
}

static void
remove_alarm (Clocks   *self,
              const gchar *clock_id) {
    gchar *id;

    GFOREACH (self->priv->alarms, id) {
        if (g_strcmp0 (id, clock_id) == 0) {
            g_message("Removing alarm: %s", id);
            self->priv->alarms = g_list_remove (self->priv->alarms, id);
            g_free (id);
            break;
        }
    };

    bim_bus_remove_alarm (bim_bus_get_default (), clock_id);
}

static void
update_alarm (Clocks   *self,
              GVariant *alarm) {
    GVariantIter alarm_iter;
    gchar *alarm_key;
    GVariant *alarm_value;
    gchar *clock_id;
    g_autofree gchar *new_clock_id = NULL;
    g_autofree gchar *ring_time = NULL;
    gboolean alarm_exists = FALSE;

    g_variant_iter_init (&alarm_iter, alarm);
    while (g_variant_iter_next (&alarm_iter, "{sv}", &alarm_key, &alarm_value)) {
        if (g_strcmp0 (alarm_key, "ring_time") == 0)
            g_variant_get (alarm_value, "s", &ring_time);
        else if (g_strcmp0 (alarm_key, "id") == 0)
            g_variant_get (alarm_value, "s", &new_clock_id);

        g_variant_unref (alarm_value);
        g_free (alarm_key);
    }

    if (new_clock_id == NULL) {
        return;
    }

    GFOREACH (self->priv->alarms, clock_id) {
        if (g_strcmp0 (clock_id, new_clock_id) == 0) {
            alarm_exists = TRUE;
            break;
        }
    };

    if (!alarm_exists && ring_time != NULL) {
        g_autoptr(GDateTime) datetime;
        gint64 timestamp;
        datetime = g_date_time_new_from_iso8601 (ring_time, NULL);
        timestamp = g_date_time_to_unix (datetime);
        add_alarm (self, new_clock_id, timestamp);

    } else if (alarm_exists && ring_time == NULL) {
        remove_alarm (self, new_clock_id);
    }
}

static void
on_alarms_changed (ClocksSettings   *settings,
                   gpointer     user_data) {
    Clocks *self = CLOCKS (user_data);
    GVariant *alarms = NULL;
    GVariantIter alarms_iter;
    GVariant *alarm;

    alarms = clocks_settings_get_alarms (settings);

    if (alarms == NULL)
        return;

    g_variant_iter_init (&alarms_iter, alarms);
    while ((alarm = g_variant_iter_next_value (&alarms_iter))) {
        update_alarm (self, alarm);
        g_variant_unref (alarm);
    }
    g_variant_unref (alarms);
}

static void
clocks_dispose (GObject *clocks)
{
    Clocks *self = CLOCKS (clocks);

    g_list_free_full (self->priv->alarms, g_free);
    g_clear_object (&self->priv->settings);

    G_OBJECT_CLASS (clocks_parent_class)->dispose (clocks);
}

static void
clocks_finalize (GObject *clocks)
{
    G_OBJECT_CLASS (clocks_parent_class)->finalize (clocks);
}

static void
clocks_class_init (ClocksClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = clocks_dispose;
    object_class->finalize = clocks_finalize;
}

static void
clocks_init (Clocks *self)
{
    self->priv = clocks_get_instance_private (self);

    self->priv->settings = CLOCKS_SETTINGS (clocks_settings_new ());
    self->priv->alarms = NULL;

    g_signal_connect (
        self->priv->settings,
        "alarms-changed",
        G_CALLBACK (on_alarms_changed),
        self
    );
}

/**
 * clocks_new:
 *
 * Creates a new #Clocks
 *
 * Returns: (transfer full): a new #Clocks
 *
 **/
GObject *
clocks_new (void)
{
    GObject *clocks;

    clocks = g_object_new (TYPE_CLOCKS, NULL);

    return clocks;
}

static Clocks *default_clocks = NULL;
/**
 * clocks_get_default:
 *
 * Gets the default #Clocks.
 *
 * Return value: (transfer full): the default #Clocks.
 */
Clocks *
clocks_get_default (void)
{
    if (!default_clocks) {
        default_clocks = CLOCKS (clocks_new ());
    }
    return default_clocks;
}

/**
 * clocks_update:
 *
 * Update available alarms
 *
 * @self: #Clocks
 */
void
clocks_update (Clocks *self)
{
    on_alarms_changed (
        self->priv->settings,
        self
    );
}