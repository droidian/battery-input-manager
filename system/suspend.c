/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "d-bus.h"
#include "settings.h"
#include "suspend.h"


#define UPOWER_DBUS_NAME       "org.freedesktop.UPower"
#define UPOWER_DBUS_PATH       "/org/freedesktop/UPower/devices/DisplayDevice"
#define UPOWER_DBUS_INTERFACE  "org.freedesktop.UPower.Device"


#define INPUT_THRESHOLD_START  60
#define INPUT_THRESHOLD_END    80
#define INPUT_THRESHOLD_MAX    100

#define REFRESH_RATE           600000
#define REFRESH_RATE_SIMULATE  1000

#define SIMULATE_CYCLE_START   70


enum {
    PROP_0,
    PROP_SIMULATE
};


struct _SuspendPrivate {
    GDBusProxy *upower_proxy;

    gint threshold_max;
    gint threshold_start;
    gint threshold_end;

    guint64 next_alarm;
    guint64 time_to_full;

    gint percentage;
    gint previous_percentage;

    gboolean suspended;
    gboolean suspend_lock;

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

    g_message ("Suspending input");
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

    g_message ("Resuming input");
    self->priv->suspended = FALSE;
    fprintf (sysfs, "%d", settings_get_sysfs_resume_input_value (settings));

    fclose (sysfs);
}

static gboolean
handle_input_threshold_start (Suspend *self) {
    if (self->priv->percentage <= self->priv->threshold_start) {
        g_message ("Reached start threshold");
        resume_input (self);
        return TRUE;
    }
    return FALSE;
}


static gboolean
handle_input_threshold_end (Suspend *self) {
    if (self->priv->percentage >= self->priv->threshold_end) {
        g_message ("Reached end threshold");
        suspend_input (self);
        return TRUE;
    }
    return FALSE;
}


static gboolean
handle_input_threshold_max (Suspend *self) {
    if (self->priv->percentage >= self->priv->threshold_max) {
        g_message ("Reached max threshold");
        suspend_input (self);
        self->priv->next_alarm = 0;
        return TRUE;
    }
    return FALSE;
}

static gboolean
has_alarm_pending (Suspend *self) {
    if (self->priv->next_alarm != 0 &&
            self->priv->time_to_full != 0) {
        g_autoptr(GDateTime) datetime;
        gint64 timestamp;

        datetime = g_date_time_new_now_utc ();
        timestamp = g_date_time_to_unix (datetime);

        if (self->priv->next_alarm - timestamp < self->priv->time_to_full)
            return TRUE;
    }
    return FALSE;
}
static gboolean
handle_input_threshold_alarm (Suspend *self) {
    if (has_alarm_pending (self)) {
        g_message ("Alarm pending: %ld", self->priv->next_alarm);
        resume_input (self);
        return TRUE;
    }
    return FALSE;
}


static gboolean
handle_input (Suspend *self) {
    if (self->priv->suspended) {
        if (handle_input_threshold_start (self)) {
            self->priv->suspend_lock = FALSE;
            self->priv->next_alarm = bim_bus_get_next_alarm (
                bim_bus_get_default ()
            );
            return TRUE;
        }
        if (handle_input_threshold_alarm (self)) {
            self->priv->suspend_lock = TRUE;
            return TRUE;
        }
    } else {
        if (!self->priv->suspend_lock) {
            if (has_alarm_pending (self)) {
                g_message ("Alarm pending: %ld", self->priv->next_alarm);
                self->priv->suspend_lock = TRUE;
                return TRUE;
            }
            if (handle_input_threshold_end (self))
                return TRUE;
        }
        handle_input_threshold_max (self);
    }
    return TRUE;
}


static void
log_percentage (Suspend *self) {
    g_autoptr(GDateTime) datetime;
    const gchar *status;
    gint64 timestamp;

    datetime = g_date_time_new_now_utc ();
    timestamp = g_date_time_to_unix (datetime);

    if (self->priv->percentage > self->priv->previous_percentage)
        status = "Charging";
    else
        status = "Discharging";

    timestamp = self->priv->next_alarm - timestamp;
    if (timestamp > 0) {
        g_message (
            "%s: %d (%ld)",
            status,
            self->priv->percentage,
            timestamp
        );
    } else {
        g_message (
            "%s: %d",
            status,
            self->priv->percentage
        );
    }
}


static void
handle_time_to_full (Suspend  *self,
                     GVariant *data) {
    if (!self->priv->suspended)
        self->priv->time_to_full = g_variant_get_int64 (data);
}


static void
handle_percentage (Suspend  *self,
                   GVariant *data) {
    self->priv->previous_percentage = self->priv->percentage;

    self->priv->percentage = g_variant_get_int32 (data);
}


static gboolean
simulate_charging_cycle (Suspend *self) {
    GRand *rand = g_rand_new ();

    if (self->priv->suspended)
        self->priv->percentage -= 1;
    else
        self->priv->percentage += 1;

    self->priv->time_to_full =  g_rand_int_range (rand, 20, 40);

    log_percentage (self);
    handle_input (self);

    g_free (rand);
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
on_alarm_updated (BimBus  *bim_bus,
                  gpointer user_data) {
    Suspend *self = SUSPEND (user_data);

    self->priv->next_alarm = bim_bus_get_next_alarm (bim_bus);
}


static void
on_setting_changed (BimBus   *bim_bus,
                    gchar    *setting,
                    gint      value,
                    gpointer  user_data) {
    Suspend *self = SUSPEND (user_data);

    g_message ("Setting changed: %s -> %d", setting, value);

    if (g_strcmp0 (setting, "threshold-max") == 0)
        self->priv->threshold_max = value;
    else if (g_strcmp0 (setting, "threshold-start") == 0)
        self->priv->threshold_start = value;
    else if (g_strcmp0 (setting, "threshold-end") == 0)
        self->priv->threshold_end = value;
}


static void
suspend_connect_upower (Suspend *self) {
    if (self->priv->simulate) {
        self->priv->percentage = SIMULATE_CYCLE_START;
        g_timeout_add (
            REFRESH_RATE_SIMULATE,
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
        g_timeout_add (
            REFRESH_RATE,
            (GSourceFunc) handle_input,
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
    
    self->priv->percentage = 0;
    self->priv->previous_percentage = 0;

    self->priv->suspended = FALSE;
    self->priv->suspend_lock = FALSE;

    self->priv->next_alarm = 0;
    self->priv->time_to_full = 0;

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
