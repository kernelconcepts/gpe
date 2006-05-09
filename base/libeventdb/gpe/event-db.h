/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef EVENT_DB_H
#define EVENT_DB_H

#define SECONDS_IN_DAY (24*60*60)

#include <glib-object.h>
#include <glib.h>
#include <time.h>

#define MON  (1 << 0)
#define TUE  (1 << 1)
#define WED  (1 << 2)
#define THU  (1 << 3)
#define FRI  (1 << 4)
#define SAT  (1 << 5)
#define SUN  (1 << 6)

enum event_recurrence_type
{
  RECUR_NONE,
  RECUR_DAILY,
  RECUR_WEEKLY,
  RECUR_MONTHLY,
  RECUR_YEARLY
};

#define EVENT_TYPE (event_get_type ())
#define EVENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVENT_TYPE, struct _Event))
#define EVENT_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, EVENT_TYPE, struct _EventCLass)

struct _EventClass;
typedef struct _EventClass EventClass;

struct _Event;
typedef struct _Event Event;

extern GType event_get_type (void);

struct _EventDB;
typedef struct _EventDB EventDB;

#define TYPE_EVENT_DB             (event_db_get_type ())
#define EVENT_DB(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EVENT_DB, EventDB))
#define EVENT_DB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_EVENT_DB, EventDBClass))
#define IS_EVENT_DB(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EVENT_DB))
#define IS_EVENT_DB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_EVENT_DB))
#define EVENT_DB_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EVENT_DB, EventDBClass))

/* Return GType of a view.  */
extern GType event_db_get_type (void);

/* An event db signals the "alarm-fired" signal when an event's alarm
   goes off (but only after a call to
   event_db_list_unacknowledged_alarms).  */
typedef void (*EventDBAlarmFiredFunc) (EventDB *, Event *);

/* Open a new database.  */
extern EventDB *event_db_new (const char *filename);

/* Search the event database EVD for the event with the uid UID.  */
extern Event *event_db_find_by_uid (EventDB *evd, guint uid);

/* Return the event whose alarm will first go off at or after NOW.  */
extern Event *event_db_next_alarm (EventDB *edb, time_t now);

/* Return the events in the event database EVD which occur between
   START and END.  The list is not sorted.  */
extern GSList *event_db_list_for_period (EventDB *evd,
					 time_t start, time_t end);
/* Like event_db_list but returns the events whose alarm goes off
   between START and END.  The list is not sorted.  */
extern GSList *event_db_list_alarms_for_period (EventDB *evd,
						time_t start, time_t end);
/* Like event_db_list_for_period but only for untimed events
   (i.e. which with a 0 length duration).  The list is not sorted.  */
extern GSList *event_db_untimed_list_for_period (EventDB *evd,
						 time_t start, time_t end);

/* Returns the list of unacknowledged events since EDB was last shut
   down and turns on the "alarm-fire" event.  The list is not sorted.
   Only call this function once at start up after having connected to
   the "alarm-fire" signal.  */
extern GSList *event_db_list_unacknowledged_alarms (EventDB *edb);

/* This function has a GCompareFunc type signature and can be passed
   to e.g. g_slist_sort.

   Returns -1 if A occurs before B (or, if they start at the same
   time, if A is shorter).  Returns 1 if B occurs before A (or, if
   they start at the same time, if B is shorter).  Otherwise 0.  */
extern gint event_compare_func (gconstpointer a, gconstpointer b);

/* This function has a GCompareFunc type signature and can be passed
   to e.g. g_slist_sort.

   Returns -1 if A's alarm goes off before B's.  Returns 1 if B's
   alarm goes off before A's.  Otherwise 0.  */
extern gint event_alarm_compare_func (gconstpointer a, gconstpointer b);

/* Create a new event in database EDB.  If EVENTID is NULL, one is
   fabricated.  */
extern Event *event_new (EventDB *edb, const char *event_id);

/* Flush event EV to the DB now.  */
extern gboolean event_flush (Event *ev);

/* Remove event EV from the underlying DB and dereference it.  */
extern gboolean event_remove (Event *ev);

/* Acknowledge that EV's alarm went off.  If EV does not have an alarm
   or it would go off in the future, does nothing.  */
extern void event_acknowledge (Event *ev);

/* g_object unref each Event * on the list and destroy the list.  */
extern void event_list_unref (GSList *l);

extern time_t event_get_start (Event *ev) __attribute__ ((pure));

extern unsigned long event_get_duration (Event *ev) __attribute__ ((pure));
extern void event_set_duration (Event *ev, unsigned long duration);

extern unsigned long event_get_alarm (Event *ev) __attribute__ ((pure));
extern void event_set_alarm (Event *ev, unsigned long alarm);

/* Recurrence type.  */
extern enum event_recurrence_type event_get_recurrence_type (Event *ev)
     __attribute__ ((pure));
extern void event_set_recurrence_type (Event *ev,
				       enum event_recurrence_type type);

/* Type of recurrence.  */
#define event_is_recurrence(ev) \
  (event_get_recurrence_type ((ev)) != RECUR_NONE)
extern enum event_recurrence_type event_get_recurrence_type (Event *ev)
     __attribute__ ((pure));
extern void event_set_recurrence_type (Event *ev,
				       enum event_recurrence_type type);

/* Start of the recurrence set.  */
extern time_t event_get_recurrence_start (Event *ev) __attribute__ ((pure));
extern void event_set_recurrence_start (Event *ev, time_t start);

/* End of the recurrence set.  */
extern time_t event_get_recurrence_end (Event *ev) __attribute__ ((pure));
extern void event_set_recurrence_end (Event *ev, time_t end);

/* Number of times the recurrence is expanded.  If 0, then this not
   does constrain the number of recurrences.  */
extern guint32 event_get_recurrence_count (Event *ev)
     __attribute__ ((pure));
extern void event_set_recurrence_count (Event *ev, guint32 count);

/* iCal's interval property: the number of units to skip.  If the
   recurrence type is RECUR_YEARLY then the first recurrence occurs
   INCREMENT years after the initial start.  */
extern guint32 event_get_recurrence_increment (Event *ev)
     __attribute__ ((pure));
extern void event_set_recurrence_increment (Event *ev, guint32 increment);
     
/* The day mask is only meaningful if TYPE is RECUR_WEEKLY: if bit 0
   is set then occurs on Mon, bit 1, Tue, etc.  */
extern guint64 event_get_recurrence_daymask (Event *ev)
     __attribute__ ((pure));
extern void event_set_recurrence_daymask (Event *ev, guint64 daymask);

extern void event_add_recurrence_exception (Event *ev, time_t start);

extern gboolean event_get_untimed (Event *ev) __attribute__ ((pure));
extern void event_set_untimed (Event *ev, gboolean value);

extern unsigned long event_get_uid (Event *ev) __attribute__ ((pure));
extern const char *event_get_eventid (Event *ev) __attribute__ ((pure));


extern const char *event_get_summary (Event *ev) __attribute__ ((pure));
extern void event_set_summary (Event *ev, const char *summary);

extern const char *event_get_description (Event *ev) __attribute__ ((pure));
extern void event_set_description (Event *ev, const char *description);

extern const char *event_get_location (Event *ev) __attribute__ ((pure));
extern void event_set_location (Event *ev, const char *location);

extern const GSList *event_get_categories (Event *ev)
     __attribute__ ((pure));
extern void event_add_category (Event *ev, int category);
/* After calling this function, EV owns CATEGORIES.  If you need to
   continue to use CATEGORIES, pass a copy.  */
extern void event_set_categories (Event *ev, GSList *categories);

#endif
