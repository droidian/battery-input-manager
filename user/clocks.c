/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>

#include "clocks.h"
#include "config.h"
#include "d-bus.h"

#define CLOCKS_ID "org.gnome.clocks"

enum {
    PROP_0,
    PROP_SIMULATE
};


struct _ClocksPrivate {
    GSettings *settings;
    GList *alarms;
    gboolean simulate;
};


G_DEFINE_TYPE_WITH_CODE (Clocks, clocks, G_TYPE_OBJECT,
    G_ADD_PRIVATE (Clocks))


static void
add_new_alarm (Clocks   *self,
               GVariant *alarm) {
    GVariantIter alarm_iter;
    gchar *alarm_key;
    GVariant *alarm_value;
    gint state;
    gint64 time;

    g_variant_iter_init (&alarm_iter, alarm);
    while (g_variant_iter_next (&alarm_iter, "{sv}", &alarm_key, &alarm_value)) {
        if (g_strcmp0 (alarm_key, "time") == 0)
            g_variant_get (alarm_value, "x", &time);
        else if (g_strcmp0 (alarm_key, "state") == 0)
            g_variant_get (alarm_value, "i", &state);
    }

    if (state == 1) {
        g_message(
            "Adding new alarm at: %ld", time
        );
        bim_bus_add_alarm (bim_bus_get_default (), APP_ID, time);
    }
}


static void
remove_alarms (void) {
    g_message("Removing alarms");
    bim_bus_remove_alarms (bim_bus_get_default (), APP_ID);
}


static void
add_fake_alarms (Clocks *self) {
    g_autoptr(GDateTime) datetime;
    gint64 timestamp;
    GRand *rand = g_rand_new ();

    datetime = g_date_time_new_now_utc ();
    timestamp = g_date_time_to_unix (datetime);

    bim_bus_add_alarm (
        bim_bus_get_default (),
        APP_ID,
        timestamp + g_rand_int_range (rand, 30, 60)
    );
    bim_bus_add_alarm (
        bim_bus_get_default (),
        APP_ID,
        timestamp + g_rand_int_range (rand, 80, 120)
    );

    g_free (rand);
}

static void
on_alarms_changed (GSettings   *settings,
                   const gchar *key,
                   gpointer     user_data) {
    Clocks *self = CLOCKS (user_data);
    GVariant *alarms = NULL;
    GVariantIter alarms_iter;
    GVariant *alarm;

    remove_alarms ();

    alarms = g_settings_get_value (settings, key);

    if (alarms == NULL)
        return;

    g_variant_iter_init (&alarms_iter, alarms);
    while ((alarm = g_variant_iter_next_value (&alarms_iter))) {
        add_new_alarm (self, alarm);
        g_variant_unref (alarm);
    }
    g_variant_unref (alarms);
}

static void
clocks_connect_settings (Clocks *self) {
    if (self->priv->simulate) {
        add_fake_alarms (self);
    } else {
        g_signal_connect (
            self->priv->settings,
            "changed::alarms",
            G_CALLBACK (on_alarms_changed),
            self
        );
        on_alarms_changed (
            self->priv->settings,
            "alarms",
            self
        );
    }
}


static void
clocks_set_property (GObject *object,
                     guint property_id,
                     const GValue *value,
                     GParamSpec *pspec)
{
    Clocks *self = CLOCKS (object);

    switch (property_id) {
        case PROP_SIMULATE:
            self->priv->simulate = g_value_get_boolean (value);
            clocks_connect_settings (self);
            return;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}


static void
clocks_get_property (GObject *object,
                     guint property_id,
                     GValue *value,
                     GParamSpec *pspec)
{
    Clocks *self = CLOCKS (object);

    switch (property_id) {
        case PROP_SIMULATE:
            g_value_set_boolean (
                value, self->priv->simulate);
            return;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
clocks_dispose (GObject *clocks)
{
    Clocks *self = CLOCKS (clocks);

    g_list_free_full (self->priv->alarms, g_free);
    g_clear_object (&self->priv->settings);
    g_free (self->priv);

    G_OBJECT_CLASS (clocks_parent_class)->dispose (clocks);
    g_warning("dispose");
}

static void
clocks_class_init (ClocksClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = clocks_dispose;
    object_class->set_property = clocks_set_property;
    object_class->get_property = clocks_get_property;


    g_object_class_install_property (
        object_class,
        PROP_SIMULATE,
        g_param_spec_boolean (
            "simulate",
            "Simulate alarms",
            "Simulate alarms",
            FALSE,
            G_PARAM_WRITABLE|
            G_PARAM_CONSTRUCT_ONLY
        )
    );
}

static void
clocks_init (Clocks *self)
{
    self->priv = clocks_get_instance_private (self);
    self->priv->settings = g_settings_new (CLOCKS_ID);
    self->priv->alarms = NULL;
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
clocks_new (gboolean simulate)
{
    GObject *clocks;

    clocks = g_object_new (TYPE_CLOCKS, "simulate", simulate, NULL);

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
clocks_get_default (gboolean simulate)
{
    if (!default_clocks) {
        default_clocks = CLOCKS (clocks_new (simulate));
    }
    return default_clocks;
}
