/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>

#include "d-bus.h"

#define DBUS_NAME      "org.adishatz.Bim"
#define DBUS_PATH      "/org/adishatz/Bim"
#define DBUS_INTERFACE "org.adishatz.Bim"


struct _BimBusPrivate {
    GDBusProxy *bim_proxy;
};


G_DEFINE_TYPE_WITH_CODE (BimBus, bim_bus, G_TYPE_OBJECT,
    G_ADD_PRIVATE (BimBus))


static void
bim_bus_dispose (GObject *bim_bus)
{
    BimBus *self = BIM_BUS (bim_bus);

    g_clear_object (&self->priv->bim_proxy);
    g_free (self->priv);

    G_OBJECT_CLASS (bim_bus_parent_class)->dispose (bim_bus);
}

static void
bim_bus_class_init (BimBusClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = bim_bus_dispose;
}

static void
bim_bus_init (BimBus *self)
{
    self->priv = bim_bus_get_instance_private (self);

    self->priv->bim_proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM,
        0,
        NULL,
        DBUS_NAME,
        DBUS_PATH,
        DBUS_INTERFACE,
        NULL,
        NULL
    );
}

/**
 * bim_bus_new:
 *
 * Creates a new #BimBus
 *
 * Returns: (transfer full): a new #BimBus
 *
 **/
GObject *
bim_bus_new (void)
{
    GObject *bim_bus;

    bim_bus = g_object_new (
        TYPE_BIM_BUS,
        NULL
    );

    return bim_bus;
}


/**
 * bim_bus_add_alarm:
 *
 * Add a new alarm.
 *
 * @self: a #BimBus
 * @app_id: an app id
 * @time: a timestamp
 */
void
bim_bus_add_alarm (BimBus      *self,
                   const gchar *app_id,
                   gint64       time) {
    g_autoptr (GError) error = NULL;
    GVariant *result = NULL;

    result = g_dbus_proxy_call_sync (
        self->priv->bim_proxy,
        "AddAlarm",
        g_variant_new ("(&sx)", app_id, time),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error == NULL)
        g_variant_unref (result);
    else
        g_warning ("Error adding an alarm: %s", error->message);
}


/**
 * bim_bus_remove_alarms:
 *
 * Remove alarms our alarms.
 *
 * @self: a #BimBus
 */
void
bim_bus_remove_alarms (BimBus      *self,
                       const gchar *app_id) {
    g_autoptr (GError) error = NULL;
    GVariant *result = NULL;

    result = g_dbus_proxy_call_sync (
        self->priv->bim_proxy,
        "RemoveAlarms",
        g_variant_new ("(&s)", app_id),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error == NULL)
        g_variant_unref (result);
    else
        g_warning ("Error removing alarms: %s", error->message);
}


/**
 * bim_bus_set_value:
 *
 * Gets the next pending alarm.
 *
 * @self: a #BimBus
 * @key: a setting key
 * @value: a setting value
 */
void
bim_bus_set_value (BimBus *self, const gchar *key, gint value) {
    g_autoptr (GError) error = NULL;
    GVariant *result = NULL;

    result = g_dbus_proxy_call_sync (
        self->priv->bim_proxy,
        "Set",
        g_variant_new ("(&si)", key, value),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error == NULL)
        g_variant_unref (result);
    else
        g_warning ("Error updating setting: %s", error->message);
}


static BimBus *default_bim_bus = NULL;
/**
 * bim_bus_get_default:
 *
 * Gets the default #BimBus.
 *
 * Return value: (transfer full): the default #BimBus.
 */
BimBus *
bim_bus_get_default (void)
{
    if (!default_bim_bus) {
        default_bim_bus = BIM_BUS (bim_bus_new ());
    }
    return default_bim_bus;
}
