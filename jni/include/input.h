#ifndef _INPUT_H
#define _INPUT_H

#include "events.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus\


typedef void QEMUPutKBDEvent(void *opaque, int keycode);
typedef void QEMUPutLEDEvent(void *opaque, int ledstate);
typedef void QEMUPutMouseEvent(void *opaque, int dx, int dy, int dz, int buttons_state);

typedef struct QEMUPutMouseEntry {
    QEMUPutMouseEvent *qemu_put_mouse_event;
    void *qemu_put_mouse_event_opaque;
    int qemu_put_mouse_event_absolute;
    char *qemu_put_mouse_event_name;

    int index;

    /* used internally by qemu for handling mice */
    QTAILQ_ENTRY(QEMUPutMouseEntry) node;
} QEMUPutMouseEntry;

typedef struct QEMUPutKBDEntry {
    QEMUPutKBDEvent *put_kbd_event;
    void *opaque;

    /* used internally by qemu for handling keyboards */
    QTAILQ_ENTRY(QEMUPutKBDEntry) next;
} QEMUPutKBDEntry;


typedef struct Notifier Notifier;

struct Notifier
{
    void (*notify)(Notifier *notifier);
    QTAILQ_ENTRY(Notifier) node;
};

typedef struct NotifierList
{
    QTAILQ_HEAD(, Notifier) notifiers;
} NotifierList;

#define NOTIFIER_LIST_INITIALIZER(head) \
    { QTAILQ_HEAD_INITIALIZER((head).notifiers) }

void notifier_list_init(NotifierList *list);

void notifier_list_add(NotifierList *list, Notifier *notifier);

void notifier_list_remove(NotifierList *list, Notifier *notifier);

void notifier_list_notify(NotifierList *list);


#ifdef __cplusplus
}
#endif // __cplusplus


#endif//_INPUT_H

