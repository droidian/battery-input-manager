/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef SUSPEND_H
#define SUSPEND_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_SUSPEND \
    (suspend_get_type ())
#define SUSPEND(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_SUSPEND, Suspend))
#define SUSPEND_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_SUSPEND, SuspendClass))
#define IS_SUSPEND(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_SUSPEND))
#define IS_SUSPEND_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_SUSPEND))
#define SUSPEND_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_SUSPEND, SuspendClass))

G_BEGIN_DECLS

typedef struct _Suspend Suspend;
typedef struct _SuspendClass SuspendClass;
typedef struct _SuspendPrivate SuspendPrivate;

struct _Suspend {
    GObject parent;
    SuspendPrivate *priv;
};

struct _SuspendClass {
    GObjectClass parent_class;
};

GType           suspend_get_type            (void) G_GNUC_CONST;

GObject*        suspend_new                 (gboolean simulate);

G_END_DECLS

#endif

