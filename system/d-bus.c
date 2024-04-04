/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>

#include "d-bus.h"
#include "history.h"
#include "utils.h"

#define DBUS_NAME "org.adishatz.Bim"
#define DBUS_PATH "/org/adishatz/Bim"


/* signals */
enum
{
    ALARM_ADDED,
    ALARM_REMOVED,
    SETTING_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


struct _BimBusPrivate {
    GDBusConnection *connection;
    GDBusNodeInfo *introspection_data;
    guint owner_id;
    
    GList *alarms;
};

G_DEFINE_TYPE_WITH_CODE (BimBus, bim_bus, G_TYPE_OBJECT,
    G_ADD_PRIVATE (BimBus))


static gint
sort_alarms (gconstpointer a,
             gconstpointer b)
{
    GVariant* va = (GVariant *) a;
    GVariant* vb = (GVariant *) b;
    gint64 a_time;
    gint64 b_time;

    g_variant_get (va, "(&sx)", NULL, &a_time);
    g_variant_get (vb, "(&sx)", NULL, &b_time);

    return a_time - b_time;
}


static void
handle_method_call (GDBusConnection *connection,
                    const gchar *sender,
                    const gchar *object_path,
                    const gchar *interface_name,
                    const gchar *method_name,
                    GVariant *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer user_data)
{
    BimBus *self = user_data;

    if (g_strcmp0 (method_name, "AddAlarm") == 0) {
        const gchar* app_id;
        gint64 time;

        g_variant_get (parameters, "(&sx)", &app_id, &time);

        g_message ("Adding alarm: %ld", time);

        self->priv->alarms = g_list_insert_sorted (
            self->priv->alarms,
            g_variant_new ("(&sx)", app_id, time),
            sort_alarms
        );

        g_dbus_method_invocation_return_value (
            invocation, NULL
        );

        g_signal_emit(self, signals[ALARM_ADDED], 0);
    } else if (g_strcmp0 (method_name, "RemoveAlarm") == 0) {
        const gchar* app_id;
        GVariant *data;
        gint64 time;

        g_variant_get (parameters, "(&sv)", &app_id, &time);

        GFOREACH (self->priv->alarms, data) {
            gint64 _time = g_variant_get_int64 (data);
            if (time == _time) {
                g_message ("%s removing alarm at %ld", app_id, time);
                self->priv->alarms = g_list_remove (self->priv->alarms, data);
                g_free (data);
                break;
            }
        };

        g_dbus_method_invocation_return_value (
            invocation, NULL
        );
        g_signal_emit(self, signals[ALARM_REMOVED], 0);

    } else if (g_strcmp0 (method_name, "Set") == 0) {
        const gchar* setting;
        gint value;

        g_variant_get (parameters, "(&si)", &setting, &value);
        g_signal_emit(self, signals[SETTING_CHANGED], 0, setting, value);

        g_dbus_method_invocation_return_value (
            invocation, NULL
        );
    }
}

static const GDBusInterfaceVTable interface_vtable = {
    handle_method_call,
    NULL,
    NULL
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar *name,
                 gpointer user_data)
{
    BimBus *self = user_data;
    guint registration_id;

    registration_id = g_dbus_connection_register_object (
        connection,
        DBUS_PATH,
        self->priv->introspection_data->interfaces[0],
        &interface_vtable,
        user_data,
        NULL,
        NULL
    );

    self->priv->connection = g_object_ref (connection);

    g_assert (registration_id > 0);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{}


static void
on_name_lost (GDBusConnection *connection,
              const gchar *name,
              gpointer user_data)
{
    g_error ("Cannot own D-Bus name. Verify installation: %s\n", name);
}

static void
bim_bus_set_property (GObject *object,
                        guint property_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
bim_bus_get_property (GObject *object,
                      guint property_id,
                      GValue *value,
                      GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
bim_bus_dispose (GObject *bim_bus)
{
    BimBus *self = BIM_BUS (bim_bus);

    if (self->priv->owner_id != 0) {
        g_bus_unown_name (self->priv->owner_id);
    }

    g_clear_pointer (&self->priv->introspection_data, g_dbus_node_info_unref);
    g_clear_object (&self->priv->connection);

    g_free (self->priv);

    G_OBJECT_CLASS (bim_bus_parent_class)->dispose (bim_bus);
}

static void
bim_bus_class_init (BimBusClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = bim_bus_set_property;
    object_class->get_property = bim_bus_get_property;
    object_class->dispose = bim_bus_dispose;

    signals[ALARM_ADDED] = g_signal_new (
        "alarm-added",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL, NULL,
        G_TYPE_NONE,
        0
    );

    signals[ALARM_REMOVED] = g_signal_new (
        "alarm-removed",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL, NULL,
        G_TYPE_NONE,
        0
    );

    signals[SETTING_CHANGED] = g_signal_new (
        "setting-changed",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL, NULL,
        G_TYPE_NONE,
        2,
        G_TYPE_STRING,
        G_TYPE_INT
    );

}

static void
bim_bus_init (BimBus *self)
{
    GBytes *bytes;

    self->priv = bim_bus_get_instance_private (self);

    bytes = g_resources_lookup_data (
        "/org/adishatz/Bim/org.adishatz.Bim.xml",
        G_RESOURCE_LOOKUP_FLAGS_NONE,
        NULL
    );
    self->priv->introspection_data = g_dbus_node_info_new_for_xml (
        g_bytes_get_data (bytes, NULL),
        NULL
    );
    g_bytes_unref (bytes);

    g_assert (self->priv->introspection_data != NULL);

    self->priv->owner_id = g_bus_own_name (
        G_BUS_TYPE_SYSTEM,
        DBUS_NAME,
        G_BUS_NAME_OWNER_FLAGS_NONE,
        on_bus_acquired,
        on_name_acquired,
        on_name_lost,
        self,
        NULL
    );

    self->priv->alarms = NULL;
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
    return g_object_ref (default_bim_bus);
}


/**
 * bim_bus_get_next_alarm:
 *
 * Gets the next pending alarm.
 *
 * @self: a #BimBus
 *
 * Return value: the next pending alarm or 0 if none.
 */
gint64
bim_bus_get_next_alarm (BimBus *self) {
    gint64 timestamp = 0;
    GVariant *alarm;
    g_autoptr(GDateTime) datetime;
    gint64 current_timestamp;

    datetime = g_date_time_new_now_utc ();
    current_timestamp = g_date_time_to_unix (datetime);

    GFOREACH (self->priv->alarms, alarm) {
        g_variant_get (alarm, "(&sx)", NULL, &timestamp);
        if (timestamp > current_timestamp) {
            g_message ("Next alarm: %ld", timestamp);
            return timestamp;
        }
    }

    return 0;
}
