/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>

#include "d-bus.h"
#include "settings.h"
#include "config.h"


struct _SettingsPrivate {
    GSettings *settings;
};


G_DEFINE_TYPE_WITH_CODE (
    Settings,
    settings,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Settings)
)


static void
on_threshold_changed (GSettings   *settings,
                      const gchar *key,
                      gpointer     user_data) {
    BimBus *bim_bus = bim_bus_get_default ();
    gint value = g_settings_get_int (settings, key);

    g_message ("Setting changed: %s -> %d", key, value);

    bim_bus_set_value (bim_bus, key, value);
}


static void
settings_dispose (GObject *settings)
{
    Settings *self = SETTINGS (settings);

    g_free (self->priv);

    G_OBJECT_CLASS (settings_parent_class)->dispose (settings);
}


static void
settings_class_init (SettingsClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = settings_dispose;
}


static void
settings_init (Settings *self)
{
    self->priv = settings_get_instance_private (self);

    self->priv->settings = g_settings_new (APP_ID);

    g_signal_connect (
        self->priv->settings,
        "changed::threshold-max",
        G_CALLBACK (on_threshold_changed),
        self
    );
    on_threshold_changed (
        self->priv->settings,
        "threshold-max",
        self
    );

    g_signal_connect (
        self->priv->settings,
        "changed::threshold-start",
        G_CALLBACK (on_threshold_changed),
        self
    );
    on_threshold_changed (
        self->priv->settings,
        "threshold-start",
        self
    );

    g_signal_connect (
        self->priv->settings,
        "changed::threshold-end",
        G_CALLBACK (on_threshold_changed),
        self
    );
    on_threshold_changed (
        self->priv->settings,
        "threshold-end",
        self
    );
}


/**
 * settings_new:
 *
 * Creates a new #Settings
 *
 * Returns: (transfer full): a new #Settings
 *
 **/
GObject *
settings_new (void)
{
    GObject *settings;

    settings = g_object_new (TYPE_SETTINGS, NULL);

    return settings;
}


static Settings *default_settings = NULL;
/**
 * settings_get_default:
 *
 * Gets the default #Settings.
 *
 * Return value: (transfer full): the default #Settings.
 */
Settings *
settings_get_default (void)
{
    if (!default_settings) {
        default_settings = SETTINGS (settings_new ());
    }
    return g_object_ref (default_settings);
}
