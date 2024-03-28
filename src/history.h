/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef HISTORY_H
#define HISTORY_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_HISTORY \
    (history_get_type ())
#define HISTORY(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_HISTORY, History))
#define HISTORY_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_HISTORY, HistoryClass))
#define IS_HISTORY(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_HISTORY))
#define IS_HISTORY_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_HISTORY))
#define HISTORY_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_HISTORY, HistoryClass))

G_BEGIN_DECLS

typedef struct _History History;
typedef struct _HistoryClass HistoryClass;
typedef struct _HistoryPrivate HistoryPrivate;

typedef enum {
    HISTORY_EVENT_ALARM_ADDED,
    HISTORY_EVENT_ALARM_DELETED,
    HISTORY_EVENT_INPUT_RESUMED,
    HISTORY_EVENT_INPUT_SUSPENDED,
    HISTORY_EVENT_BATTERY_PERCENT
} HistoryEvent;

struct _History {
    GObject parent;
    HistoryPrivate *priv;
};

struct _HistoryClass {
    GObjectClass parent_class;
};

GType           history_get_type            (void) G_GNUC_CONST;
History        *history_get_default         (void);
GObject*        history_new                 (void);
void            history_add_event           (History *self,
                                             HistoryEvent event,
                                             const gchar *custom_field,
                                             ...);

G_END_DECLS

#endif


