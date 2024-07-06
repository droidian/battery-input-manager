/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef CLOCKS_H
#define CLOCKS_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_CLOCKS (clocks_get_type ())

#define CLOCKS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_CLOCKS, Clocks))
#define CLOCKS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_CLOCKS, ClocksClass))
#define IS_CLOCKS(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_CLOCKS))
#define IS_CLOCKS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_CLOCKS))
#define CLOCKS_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_CLOCKS, ClocksClass))

G_BEGIN_DECLS

typedef struct _Clocks Clocks;
typedef struct _ClocksClass ClocksClass;
typedef struct _ClocksPrivate ClocksPrivate;

struct _Clocks {
    GObject parent;
    ClocksPrivate *priv;
};

struct _ClocksClass {
    GObjectClass parent_class;
};

GType       clocks_get_type       (void) G_GNUC_CONST;
Clocks     *clocks_get_default    (gboolean simulate);
GObject*    clocks_new            (gboolean simulate);
void        clocks_update         (Clocks *self);

G_END_DECLS

#endif
