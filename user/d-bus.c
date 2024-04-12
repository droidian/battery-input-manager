/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>
#include <glib/gi18n.h>

#include "config.h"
#include "d-bus.h"
#include "settings.h"

#define DBUS_BIM_NAME                "org.adishatz.Bim"
#define DBUS_BIM_PATH                "/org/adishatz/Bim"
#define DBUS_BIM_INTERFACE           "org.adishatz.Bim"

#define DBUS_NOTIFICATIONS_NAME      "org.freedesktop.Notifications"
#define DBUS_NOTIFICATIONS_PATH      "/org/freedesktop/Notifications"
#define DBUS_NOTIFICATIONS_INTERFACE "org.freedesktop.Notifications"
#define DBUS_NOTIFICATIONS_TIMEOUT   60000

struct _BimBusPrivate {
    GDBusProxy *bim_proxy;
    GDBusProxy *notification_proxy;

    guint notification_id;
};


G_DEFINE_TYPE_WITH_CODE (BimBus, bim_bus, G_TYPE_OBJECT,
    G_ADD_PRIVATE (BimBus))


static gboolean
hide_notification (BimBus *self) {
    g_autoptr (GError) error = NULL;

    g_dbus_proxy_call_sync (
        self->priv->notification_proxy,
        "CloseNotification",
        g_variant_new ("(u)", self->priv->notification_id),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL)
        g_warning ("Error hiding notification: %s", error->message);

    return FALSE;
}

static void
show_notification (BimBus *self,
                   gboolean suspended,
                   gint64   timestamp) {
    g_autoptr (GError) error = NULL;
    g_autoptr(GVariant) result = NULL;
    g_autofree char *description = NULL;
    g_autofree char *value = NULL;

    if (!suspended)
        return;

    if (timestamp > 0) {
        g_autoptr(GDateTime) datetime;

        datetime = g_date_time_new_from_unix_local (timestamp);
        value = g_date_time_format (datetime, "%X");
    } else {
        value = g_strdup_printf (
            "%i%%", settings_get_resume_input_value (
                settings_get_default ()
            )
        );
    }

    description =  g_strdup_printf (_("Charging will start again at %s"), value);

    result = g_dbus_proxy_call_sync (
        self->priv->notification_proxy,
        "Notify",
        g_variant_new (
            "(susssasa{sv}i)",
            _("Battery manager"),
            0,
            "battery-symbolic",
            _("Charging control"),
            description,
            0,
            0,
            -1
        ),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (result != NULL) {
        g_variant_get (result, "(u)", &self->priv->notification_id);
        g_timeout_add (
            DBUS_NOTIFICATIONS_TIMEOUT,
            (GSourceFunc) hide_notification,
            self
        );
    } else {
        g_warning ("Error showing notification: %s", error->message);
    }
}

static void
on_bim_input_suspended (GDBusProxy  *proxy,
                        const gchar *sender_name,
                        const gchar *signal_name,
                        GVariant    *parameters,
                        gpointer     user_data)
{
    BimBus *self = BIM_BUS (user_data);

    if (g_strcmp0 (signal_name, "InputSuspended") == 0) {
        gboolean suspended;
        gint64 timestamp;

        g_variant_get (parameters, "(bx)", &suspended, &timestamp);

        show_notification (self, suspended, timestamp);
    }
}

static void
bim_bus_dispose (GObject *bim_bus)
{
    BimBus *self = BIM_BUS (bim_bus);

    g_clear_object (&self->priv->bim_proxy);
    g_clear_object (&self->priv->notification_proxy);
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
        DBUS_BIM_NAME,
        DBUS_BIM_PATH,
        DBUS_BIM_INTERFACE,
        NULL,
        NULL
    );

    self->priv->notification_proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SESSION,
        0,
        NULL,
        DBUS_NOTIFICATIONS_NAME,
        DBUS_NOTIFICATIONS_PATH,
        DBUS_NOTIFICATIONS_INTERFACE,
        NULL,
        NULL
    );

    if (self->priv->bim_proxy != NULL)
        g_signal_connect (
            self->priv->bim_proxy,
            "g-signal",
            G_CALLBACK (on_bim_input_suspended),
            self
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
                   const gchar *alarm_id,
                   gint64       time) {
    g_autoptr (GError) error = NULL;
    g_autoptr(GVariant) result = NULL;

    result = g_dbus_proxy_call_sync (
        self->priv->bim_proxy,
        "AddAlarm",
        g_variant_new ("(&sx)", alarm_id, time),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL)
        g_warning ("Error adding an alarm: %s", error->message);
}


/**
 * bim_bus_remove_alarm:
 *
 * Remove alarms our alarms.
 *
 * @self: a #BimBus
 */
void
bim_bus_remove_alarm (BimBus      *self,
                      const gchar *alarm_id) {
    g_autoptr (GError) error = NULL;
    g_autoptr(GVariant) result = NULL;

    result = g_dbus_proxy_call_sync (
        self->priv->bim_proxy,
        "RemoveAlarm",
        g_variant_new ("(&s)", alarm_id),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL)
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
    g_autoptr(GVariant) result = NULL;

    result = g_dbus_proxy_call_sync (
        self->priv->bim_proxy,
        "Set",
        g_variant_new ("(&si)", key, value),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL)
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
