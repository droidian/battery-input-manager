/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef DBUS_H
#define DBUS_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_BIM_BUS (bim_bus_get_type ())

#define BIM_BUS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_BIM_BUS, BimBus))
#define BIM_BUS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_BIM_BUS, BimBusClass))
#define IS_BIM_BUS(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_BIM_BUS))
#define IS_BIM_BUS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_BIM_BUS))
#define BIM_BUS_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_BIM_BUS, BimBusClass))

G_BEGIN_DECLS

typedef struct _BimBus BimBus;
typedef struct _BimBusClass BimBusClass;
typedef struct _BimBusPrivate BimBusPrivate;

struct _BimBus {
    GObject parent;
    BimBusPrivate *priv;
};

struct _BimBusClass {
    GObjectClass parent_class;
};

GType       bim_bus_get_type       (void) G_GNUC_CONST;
BimBus     *bim_bus_get_default    (void);
GObject*    bim_bus_new            (void);
gint64      bim_bus_get_next_alarm (BimBus *self);

G_END_DECLS

#endif
