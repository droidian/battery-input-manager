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
on_enabled_changed (GSettings   *settings,
                    const gchar *key,
                    gpointer     user_data)
{
    Settings *self = SETTINGS (user_data);

    if (g_settings_get_boolean (self->priv->settings, "enabled")) {
        bim_bus_open_proxy (bim_bus_get_default ());
    } else {
        bim_bus_close_proxy (bim_bus_get_default ());
        return;
    }

    on_threshold_changed (
        self->priv->settings,
        "threshold-max",
        self
    );

    on_threshold_changed (
        self->priv->settings,
        "threshold-start",
        self
    );

    on_threshold_changed (
        self->priv->settings,
        "threshold-end",
        self
    );
}

static void
settings_dispose (GObject *settings)
{
    Settings *self = SETTINGS (settings);

    g_clear_object (&self->priv->settings);
    g_free (self->priv);

    G_OBJECT_CLASS (settings_parent_class)->dispose (settings);
}


static void
settings_finalize (GObject *settings)
{
    G_OBJECT_CLASS (settings_parent_class)->finalize (settings);
}


static void
settings_class_init (SettingsClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = settings_dispose;
    object_class->finalize = settings_finalize;
}


static void
settings_init (Settings *self)
{
    self->priv = settings_get_instance_private (self);

    self->priv->settings = g_settings_new (APP_ID);

    if (g_settings_get_boolean (self->priv->settings, "enabled")) {
        on_enabled_changed (
            self->priv->settings,
            "enabled",
            self
        );
    }

    g_signal_connect (
        self->priv->settings,
        "changed::enabled",
        G_CALLBACK (on_enabled_changed),
        self
    );

    g_signal_connect (
        self->priv->settings,
        "changed::threshold-max",
        G_CALLBACK (on_threshold_changed),
        self
    );

    g_signal_connect (
        self->priv->settings,
        "changed::threshold-start",
        G_CALLBACK (on_threshold_changed),
        self
    );

    g_signal_connect (
        self->priv->settings,
        "changed::threshold-end",
        G_CALLBACK (on_threshold_changed),
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
    return default_settings;
}

/**
 * settings_get_enabled:
 *
 * Check if BIM is enabled
 *
 * Returns: True if we need to preserve battery health
 */
gboolean
settings_get_enabled (Settings *self) {
    return  g_settings_get_boolean (self->priv->settings, "enabled");
}

/**
 * settings_get_resume_input_value:
 *
 * Get sysfs node value we need to write to resume input
 *
 * Returns: (transfer full): node value for resume
 */
gint
settings_get_resume_input_value (Settings *self) {
    return  g_settings_get_int (self->priv->settings, "threshold-start");
}