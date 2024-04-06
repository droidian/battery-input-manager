/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef CLOCKS_SETTINGS_H
#define CLOCKS_SETTINGS_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_CLOCKS_SETTINGS (clocks_settings_get_type ())

#define CLOCKS_SETTINGS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_CLOCKS_SETTINGS, ClocksSettings))
#define CLOCKS_SETTINGS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_CLOCKS_SETTINGS, ClocksSettingsClass))
#define IS_CLOCKS_SETTINGS(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_CLOCKS_SETTINGS))
#define IS_CLOCKS_SETTINGS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_CLOCKS_SETTINGS))
#define CLOCKS_SETTINGS_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_CLOCKS_SETTINGS, ClocksSettingsClass))

G_BEGIN_DECLS

typedef struct _ClocksSettings ClocksSettings;
typedef struct _ClocksSettingsClass ClocksSettingsClass;
typedef struct _ClocksSettingsPrivate ClocksSettingsPrivate;

struct _ClocksSettings {
    GObject parent;
    ClocksSettingsPrivate *priv;
};

struct _ClocksSettingsClass {
    GObjectClass parent_class;
};

GType       clocks_settings_get_type       (void) G_GNUC_CONST;
GObject*    clocks_settings_new            (void);
GVariant   *clocks_settings_get_alarms     (ClocksSettings *self);
G_END_DECLS

#endif
