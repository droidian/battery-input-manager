/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "d-bus.h"
#include "history.h"
#include "settings.h"
#include "suspend.h"


#define UPOWER_DBUS_NAME       "org.freedesktop.UPower"
#define UPOWER_DBUS_PATH       "/org/freedesktop/UPower/devices/DisplayDevice"
#define UPOWER_DBUS_INTERFACE  "org.freedesktop.UPower.Device"


#define INPUT_THRESHOLD_MIN   40
#define INPUT_THRESHOLD_START 60
#define INPUT_THRESHOLD_END   80
#define INPUT_THRESHOLD_MAX   100


#define SIMULATE_CYCLE_FULL  14
#define SIMULATE_CYCLE_ALARM 15
#define SIMULATE_CYCLE_START 70
#define SIMULATE_CYCLE_TIMEOUT 500


enum {
    PROP_0,
    PROP_SIMULATE
};


struct _SuspendPrivate {
    GDBusProxy *upower_proxy;

    gint threshold_min;
    gint threshold_max;
    gint threshold_start;
    gint threshold_end;

    guint64 next_alarm;
    guint64 time_to_full;

    gdouble percentage;

    gboolean suspended;
    gboolean cycle_running;
    gboolean simulate;
};


G_DEFINE_TYPE_WITH_CODE (
    Suspend,
    suspend,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Suspend)
)


static void
suspend_input (Suspend *self) {
    Settings *settings = settings_get_default ();
    FILE *sysfs;

    sysfs = fopen(
        settings_get_sysfs_suspend_input_path (settings),
        "w"
    );

    g_return_if_fail (sysfs != NULL);

    g_debug ("suspend_input");
    self->priv->suspended = TRUE;
    fprintf (sysfs, "%d", settings_get_sysfs_suspend_input_value (settings));

    fclose (sysfs);
}


static void
resume_input (Suspend *self) {
    Settings *settings = settings_get_default ();
    FILE *sysfs;

    sysfs = fopen(
        settings_get_sysfs_suspend_input_path (settings),
        "w"
    );

    g_return_if_fail (sysfs != NULL);

    g_debug ("resume_input");
    self->priv->suspended = FALSE;
    fprintf (sysfs, "%d", settings_get_sysfs_resume_input_value (settings));

    fclose (sysfs);
}


static void
handle_input (Suspend *self) {
    if (self->priv->suspended) {
        if (self->priv->next_alarm != 0 &&
                self->priv->time_to_full != 0) {
            g_autoptr(GDateTime) datetime;
            gint64 timestamp;

            datetime = g_date_time_new_now_utc ();
            timestamp = g_date_time_to_unix (datetime);

            if (self->priv->next_alarm - timestamp < self->priv->time_to_full) {
                BimBus *bim_bus = bim_bus_get_default ();

                g_debug (
                    "handle_input: %ld - %ld  < %ld",
                    self->priv->next_alarm,
                    timestamp,
                    self->priv->time_to_full
                );
                resume_input (self);

                self->priv->next_alarm = bim_bus_get_next_alarm (bim_bus);
            }

        } else if (self->priv->next_alarm == 0 &&
                  !self->priv->cycle_running &&
                   self->priv->percentage <= self->priv->threshold_start) {
            g_debug ("handle_input: INPUT_THRESHOLD_START");
            resume_input (self);

        } else if (self->priv->percentage < self->priv->threshold_min) {
            g_debug ("handle_input: INPUT_THRESHOLD_MIN");
            self->priv->cycle_running = FALSE;
            resume_input (self);
        }
    } else {
        if (!self->priv->cycle_running &&
               self->priv->percentage >= self->priv->threshold_end) {
            g_debug ("handle_input: INPUT_THRESHOLD_END");
            self->priv->cycle_running = TRUE;
            suspend_input (self);

        } else if (self->priv->percentage == self->priv->threshold_max) {
            g_debug ("handle_input: INPUT_THRESHOLD_MAX");
            suspend_input (self);
        }
    }
}


static void
on_alarm_updated (BimBus  *bim_bus,
                  gpointer user_data) {
    Suspend *self = SUSPEND (user_data);

    self->priv->next_alarm = bim_bus_get_next_alarm (bim_bus);
    handle_input (user_data);
}


static void
on_setting_changed (BimBus   *bim_bus,
                    gchar    *setting,
                    GVariant *variant,
                    gpointer  user_data) {
    Suspend *self = SUSPEND (user_data);

    g_warning ("%s\n", setting);
    if (g_strcmp0 (setting, "threshold-max") == 0)
        self->priv->threshold_max = g_variant_get_int32(variant);
    else if (g_strcmp0 (setting, "threshold-min") == 0)
        self->priv->threshold_min = g_variant_get_int32(variant);
    else if (g_strcmp0 (setting, "threshold-start") == 0)
        self->priv->threshold_start = g_variant_get_int32(variant);
    else if (g_strcmp0 (setting, "threshold-end") == 0)
        self->priv->threshold_end = g_variant_get_int32(variant);
}


static void
handle_time_to_full (Suspend  *self,
                     GVariant *data) {
    if (!self->priv->suspended) {
        self->priv->time_to_full = g_variant_get_int64 (data);
        handle_input (self);
    }
}


static void
handle_percentage (Suspend  *self,
                   GVariant *data) {
    History *history = history_get_default ();
    g_autofree gchar *percentage;

    self->priv->percentage = g_variant_get_double (data);

    percentage = g_strdup_printf ("%f", self->priv->percentage);
    history_add_event (
        history,
        HISTORY_EVENT_BATTERY_PERCENT,
        percentage,
        NULL
    );

    handle_input (self);
}


static gboolean
simulate_charging_cycle (Suspend *self) {
    g_autoptr(GDateTime) datetime;
    gint64 timestamp;

    datetime = g_date_time_new_now_utc ();
    timestamp = g_date_time_to_unix (datetime);

    if (self->priv->suspended) {
        self->priv->percentage -= 1;
    } else {
        self->priv->percentage += 1;
    }

    if (self->priv->percentage >= INPUT_THRESHOLD_END &&
            !self->priv->cycle_running &&
            self->priv->next_alarm == 0) {
        self->priv->next_alarm = timestamp + SIMULATE_CYCLE_ALARM;
        self->priv->time_to_full = SIMULATE_CYCLE_FULL;
        g_debug ("next_alarm: %ld", self->priv->next_alarm);

    } else if (self->priv->percentage >= INPUT_THRESHOLD_MAX) {
        self->priv->next_alarm = 0;
        self->priv->time_to_full = 0;
    }

    g_debug (
        "simulate_charging_cycle: %f -> %ld",
        self->priv->percentage,
        timestamp
    );
    handle_input (self);

    return TRUE;
}

static void
on_upower_proxy_properties (GDBusProxy  *proxy,
                            GVariant    *changed_properties,
                            char       **invalidated_properties,
                            gpointer     user_data)
{
    Suspend *self = user_data;
    GVariant *value;
    char *property;
    GVariantIter i;

    g_variant_iter_init (&i, changed_properties);
    while (g_variant_iter_next (&i, "{&sv}", &property, &value)) {
        if (g_strcmp0 (property, "TimeToFull") == 0) {
            handle_time_to_full (self, value);
        } else if (g_strcmp0 (property, "Percentage") == 0) {
            handle_percentage (self, value);
        }

        g_variant_unref (value);
    }
}


static void
suspend_connect_upower (Suspend *self) {
    if (self->priv->simulate) {
        self->priv->percentage = SIMULATE_CYCLE_START;
        g_timeout_add (
            SIMULATE_CYCLE_TIMEOUT,
            (GSourceFunc) simulate_charging_cycle,
            self
        );
    } else {
        g_autoptr (GError) error = NULL;

        self->priv->upower_proxy = g_dbus_proxy_new_for_bus_sync (
            G_BUS_TYPE_SYSTEM,
            0,
            NULL,
            UPOWER_DBUS_NAME,
            UPOWER_DBUS_PATH,
            UPOWER_DBUS_INTERFACE,
            NULL,
            &error
        );

        if (self->priv->upower_proxy == NULL)
            g_error("Can't contact UPower: %s", error->message);

        g_signal_connect (
            self->priv->upower_proxy,
            "g-properties-changed",
            G_CALLBACK (on_upower_proxy_properties),
            self
        );
    }
}

static void
suspend_set_property (GObject *object,
                      guint property_id,
                      const GValue *value,
                      GParamSpec *pspec)
{
    Suspend *self = SUSPEND (object);

    switch (property_id) {
        case PROP_SIMULATE:
            self->priv->simulate = g_value_get_boolean (value);
            suspend_connect_upower (self);
            return;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}


static void
suspend_get_property (GObject *object,
                      guint property_id,
                      GValue *value,
                      GParamSpec *pspec)
{
    Suspend *self = SUSPEND (object);

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
suspend_dispose (GObject *suspend)
{
    Suspend *self = SUSPEND (suspend);

    if (!self->priv->simulate)
        g_clear_object (&self->priv->upower_proxy);
    
    g_free (self->priv);

    G_OBJECT_CLASS (suspend_parent_class)->dispose (suspend);
}

static void
suspend_class_init (SuspendClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = suspend_dispose;
    object_class->set_property = suspend_set_property;
    object_class->get_property = suspend_get_property;

    g_object_class_install_property (
        object_class,
        PROP_SIMULATE,
        g_param_spec_boolean (
            "simulate",
            "Simulate a charging cycle",
            "Simulate a charging cycle",
            FALSE,
            G_PARAM_WRITABLE|
            G_PARAM_CONSTRUCT_ONLY
        )
    );
}

static void
suspend_init (Suspend *self)
{
    self->priv = suspend_get_instance_private (self);
    
    self->priv->suspended = FALSE;
    self->priv->cycle_running = FALSE;

    self->priv->next_alarm = 0;
    self->priv->time_to_full = 0;

    self->priv->threshold_min = INPUT_THRESHOLD_MIN;
    self->priv->threshold_max = INPUT_THRESHOLD_MAX;
    self->priv->threshold_start = INPUT_THRESHOLD_START;
    self->priv->threshold_end = INPUT_THRESHOLD_END;

    g_signal_connect (
        bim_bus_get_default (),
        "alarm-added",
        G_CALLBACK (on_alarm_updated),
        self
    );

    g_signal_connect (
        bim_bus_get_default (),
        "alarm-removed",
        G_CALLBACK (on_alarm_updated),
        self
    );

    g_signal_connect (
        bim_bus_get_default (),
        "setting-changed",
        G_CALLBACK (on_setting_changed),
        self
    );
}

/**
 * suspend_new:
 *
 * Creates a new #Suspend
 *
 * Returns: (transfer full): a new #Suspend
 *
 **/
GObject *
suspend_new (gboolean simulate)
{
    GObject *suspend;

    suspend = g_object_new (TYPE_SUSPEND, "simulate", simulate, NULL);

    return suspend;
}
