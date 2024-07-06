/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>

#include "clocks_settings.h"
#include "../common/utils.h"

#define CLOCKS_ID "org.gnome.clocks"
#define CLOCKS_PATH "org/gnome/clocks"
#define CLOCKS_KEY "alarms"
#define KEY_FILE ".var/app/org.gnome.clocks/config/glib-2.0/settings/keyfile"

/* signals */
enum
{
    ALARMS_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

struct _ClocksSettingsPrivate {
    GSettings *g_settings;
    GFile *k_settings;
    GFileMonitor *file_monitor;
};

G_DEFINE_TYPE_WITH_CODE (ClocksSettings, clocks_settings, G_TYPE_OBJECT,
    G_ADD_PRIVATE (ClocksSettings))

static gboolean
g_settings_schema_exist (const char * id)
{
    gboolean exist = FALSE;
    GSettingsSchema *res = g_settings_schema_source_lookup (
        g_settings_schema_source_get_default(), id, FALSE
    );

    if (res != NULL) {
        g_free (res);
        exist = TRUE;
    }

    return exist;
}

static void
on_keyfile_changed (GFileMonitor   *file_monitor,
                    GFile *file,
                    GFile *other_file,
                    GFileMonitorEvent event,
                    gpointer     user_data) {
    ClocksSettings *self = CLOCKS_SETTINGS (user_data);

    if (event & G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT)
        g_signal_emit(self, signals[ALARMS_CHANGED], 0);
}

static void
clocks_settings_dispose (GObject *clocks_settings)
{
    ClocksSettings *self = CLOCKS_SETTINGS (clocks_settings);

    if (self->priv->g_settings != NULL)
        g_clear_object (&self->priv->g_settings);
    if (self->priv->k_settings != NULL)
        g_clear_object (&self->priv->k_settings);
    if (self->priv->file_monitor != NULL)
        g_clear_object (&self->priv->file_monitor);

    G_OBJECT_CLASS (clocks_settings_parent_class)->dispose (clocks_settings);
}

static void
clocks_settings_finalize (GObject *clocks_settings)
{
    G_OBJECT_CLASS (clocks_settings_parent_class)->finalize (clocks_settings);
}

static void
clocks_settings_class_init (ClocksSettingsClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = clocks_settings_dispose;
    object_class->finalize = clocks_settings_finalize;

    signals[ALARMS_CHANGED] = g_signal_new (
        "alarms-changed",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL, NULL,
        G_TYPE_NONE,
        0
    );
}

static void
clocks_settings_init (ClocksSettings *self)
{
    self->priv = clocks_settings_get_instance_private (self);
    self->priv->g_settings = NULL;
    self->priv->k_settings = NULL;
    self->priv->file_monitor = NULL;

    if (g_settings_schema_exist (CLOCKS_ID)) {
        self->priv->g_settings = g_settings_new (CLOCKS_ID);
    } else {
        g_autoptr(GStrvBuilder) builder = g_strv_builder_new ();
        g_autoptr(GError) error = NULL;
        g_autofree gchar *filepath;
        const g_autofree gchar *homedir;
        GStrv strv;

        homedir = g_get_home_dir ();
        g_strv_builder_add (builder, homedir);
        g_strv_builder_add (builder, KEY_FILE);
        strv = g_strv_builder_end (builder);
        filepath = g_strjoinv ("/", strv);
        g_strfreev (strv);

        if (g_file_test (filepath, G_FILE_TEST_EXISTS)) {
            self->priv->k_settings = g_file_new_for_path (filepath);
            self->priv->file_monitor = g_file_monitor_file (
                self->priv->k_settings,
                G_FILE_MONITOR_NONE,
                NULL,
                &error
            );
            if (self->priv->file_monitor != NULL) {
                g_signal_connect (
                    self->priv->file_monitor,
                    "changed",
                    G_CALLBACK (on_keyfile_changed),
                    self
                );
            } else {
                g_warning ("Can't monitor %s: %s", filepath, error->message);
            }
        }
    }
}

/**
 * clocks_settings_new:
 *
 * Creates a new #ClocksSettings
 *
 * Returns: (transfer full): a new #ClocksSettings
 *
 **/
GObject *
clocks_settings_new (void)
{
    GObject *clocks_settings;

    clocks_settings = g_object_new (TYPE_CLOCKS_SETTINGS, NULL);

    return clocks_settings;
}

/**
 * clocks_settings_get_alarms:
 *
 * Get alarms from Clocks
 *
 * @self: #ClocksSettings
 *
 * Returns: (transfer full): a new #GVariant or NULL;
 *
 **/
GVariant
*clocks_settings_get_alarms (ClocksSettings *self) {
    if (self->priv->g_settings != NULL) {
        return g_settings_get_value (self->priv->g_settings, CLOCKS_KEY);
    } else {
        GVariant *alarms;
        g_autoptr(GVariantType) type;
        g_autoptr(GKeyFile) key_file;
        g_autoptr(GError) error = NULL;
        g_autofree gchar *data;
        g_autofree gchar *filepath;

        key_file = g_key_file_new ();
        filepath = g_file_get_path (self->priv->k_settings);

        if (!g_key_file_load_from_file (key_file,
                                        filepath,
                                        G_KEY_FILE_NONE,
                                        &error)) {
            g_warning ("Error loading Clocks settings: %s", error->message);
            return NULL;
        }

        data = g_key_file_get_value (
            key_file,
            CLOCKS_PATH,
            CLOCKS_KEY,
            &error
        );

        if (data == NULL) {
            g_warning ("Can't find group: %s", error->message);
            return NULL;
        }

        type = g_variant_type_new ("aa{sv}");
        alarms = g_variant_parse (
            type,
            data,
            NULL,
            NULL,
            &error
        );

        if (alarms == NULL) {
            g_warning ("Can't load alarms: %s", error->message);
            return NULL;
        }
        return alarms;
    }
}