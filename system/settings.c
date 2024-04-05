/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>
#include <cjson/cJSON.h>

#include "settings.h"
#include "config.h"

struct _SettingsPrivate {
    gchar* sysfs_suspend_input_path;
    gint   sysfs_suspend_input_value;
    gint   sysfs_resume_input_value;
};

G_DEFINE_TYPE_WITH_CODE (
    Settings,
    settings,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Settings)
)


static void
settings_dispose (GObject *settings)
{
    Settings *self = SETTINGS (settings);

    g_free (self->priv->sysfs_suspend_input_path);
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
    cJSON *root = NULL;
    cJSON *device = NULL;
    cJSON *path = NULL;
    cJSON *suspend = NULL;
    cJSON *resume = NULL;
    GFile *devices_json = g_file_new_for_path (DEVICES_JSON);
    GError *error = NULL;
	gchar *content = NULL;
    gint size, i;

    self->priv = settings_get_instance_private (self);
    self->priv->sysfs_suspend_input_path = NULL;

    if (!g_file_query_exists (devices_json, NULL)) {
        g_error("Devices json file missing");
        goto end;
    }
    
    if (!g_file_load_contents (
            devices_json, NULL, &content, NULL, NULL, &error)) {
        g_error("Can't load devices json file: %s", error->message);
        g_clear_error (&error);
        goto end;
    }
        
    root = cJSON_Parse (content);
    size = cJSON_GetArraySize(root);
    
    g_object_unref (devices_json);
    g_free (content);
        
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr ();
        if (error_ptr != NULL) {
            g_error ("Error before: %s", error_ptr);
        }
        goto error;
    }
    
    for (i = 0; i < size; i++) {
        device = cJSON_GetArrayItem (root, i);
        path = cJSON_GetObjectItem (device, "path");
        suspend = cJSON_GetObjectItem (device, "suspend");
        resume = cJSON_GetObjectItem (device, "resume");

        if (!cJSON_IsString (path) || (path->valuestring == NULL)) {
            continue;
        }
        if (!cJSON_IsNumber (suspend)) {
            continue;
        }
        if (!cJSON_IsNumber (resume)) {
            continue;
        }
        if (g_file_test (path->valuestring, G_FILE_TEST_EXISTS)) {
            self->priv->sysfs_suspend_input_path = g_strdup(path->valuestring);
            self->priv->sysfs_suspend_input_value = suspend->valueint;
            self->priv->sysfs_resume_input_value = resume->valueint;
        }
    }
    
error:
    cJSON_Delete (root);
end:
    g_message("Detected input sysfs node: %s", self->priv->sysfs_suspend_input_path);
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

/**
 * settings_get_input_suspend_sysfs_node:
 * 
 * Get sysfs node we need to write to suspend input
 * 
 * Returns: (transfer full): path to node
 */
gchar*
settings_get_sysfs_suspend_input_path (Settings *settings) {
    return settings->priv->sysfs_suspend_input_path;
}

/**
 * settings_get_sysfs_suspend_input_value:
 * 
 * Get sysfs node value we need to write to suspend input
 * 
 * Returns: (transfer full): node value for suspend
 */
gint
settings_get_sysfs_suspend_input_value (Settings *settings) {
    return settings->priv->sysfs_suspend_input_value;
}

/**
 * settings_get_sysfs_resume_input_value:
 * 
 * Get sysfs node value we need to write to resume input
 * 
 * Returns: (transfer full): node value for resume
 */
gint
settings_get_sysfs_resume_input_value (Settings *settings) {
    return settings->priv->sysfs_resume_input_value;
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
