/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libintl.h>
#include <langinfo.h>

#include <gtk/gtk.h>

#include "globals.h"
#include "event-db.h"
#include "gtkdatecombo.h"

#define _(_x) gettext (_x)

struct edit_state
{
  GtkWidget *deletebutton;

  GtkWidget *startdate, *enddate;
  GtkWidget *starttime, *endtime;
  
  GtkWidget *alarmbutton;
  GtkWidget *alarmspin;
  GtkWidget *alarmoption;

  GtkWidget *text;
  
  GtkWidget *radiobuttonforever, *radiobuttonendafter;
  GtkWidget *endspin, *endlabel;
  GtkWidget *radiobuttonnone, *radiobuttondaily, *radiobuttonweekly, 
    *radiobuttonmonthly, *radiobuttonyearly;

  GtkWidget *dailybox, *dailyspin;
  GtkWidget *monthlybox, *monthlyspin;
  GtkWidget *yearlybox, *yearlyspin;
  GtkWidget *weeklybox, *checkbuttonwday[7];

  GtkAdjustment *endspin_adj, *dailyspin_adj, *monthlyspin_adj, 
    *yearlyspin_adj;
  
  event_t ev;
};

static void
gtk_box_pack_start_show(GtkBox *box,
			GtkWidget *child,
			gboolean expand,
			gboolean fill,
			guint padding)
{
  gtk_widget_show (child);
  gtk_box_pack_start (box, child, expand, fill, padding);
}

static void
destroy_user_data (gpointer p)
{
  g_free (p);
}

static void
recalculate_sensitivities(GtkWidget *widget,
			  GtkWidget *d)
{
  struct edit_state *s = gtk_object_get_data (GTK_OBJECT (d), "edit_state");

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->alarmbutton)))
    {
      gtk_widget_set_sensitive (s->alarmoption, 1);
      gtk_widget_set_sensitive (s->alarmspin, 1);
    }
  else
    {
      gtk_widget_set_sensitive (s->alarmoption, 0);
      gtk_widget_set_sensitive (s->alarmspin, 0);
    }
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonnone))) 
    {
      gtk_widget_hide (s->dailybox);
      gtk_widget_hide (s->weeklybox);
      gtk_widget_hide (s->monthlybox);
      gtk_widget_hide (s->yearlybox);
      gtk_widget_set_sensitive (s->endspin, 0);
      gtk_widget_set_sensitive (s->endlabel, 0);
      gtk_widget_set_sensitive (s->radiobuttonforever, 0);
      gtk_widget_set_sensitive (s->radiobuttonendafter, 0);
    }
  else 
    {
      gtk_widget_set_sensitive (s->radiobuttonforever, 1);
      gtk_widget_set_sensitive (s->radiobuttonendafter, 1);
      
      if (gtk_toggle_button_get_active 
	  (GTK_TOGGLE_BUTTON (s->radiobuttonforever))) 
	{
	  gtk_widget_set_sensitive (s->endspin, 0);
	  gtk_widget_set_sensitive (s->endlabel, 0);
	}
      else if (gtk_toggle_button_get_active 
	       (GTK_TOGGLE_BUTTON (s->radiobuttonendafter))) 
	{
	  gtk_widget_set_sensitive (s->endspin, 1);
	  gtk_widget_set_sensitive (s->endlabel, 1);
  	}
  
  	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON 
					  (s->radiobuttondaily))) 
	  gtk_widget_show (s->dailybox);
	else
	  gtk_widget_hide (s->dailybox);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON 
					  (s->radiobuttonweekly))) 
	  gtk_widget_show (s->weeklybox);
	else
	  gtk_widget_hide (s->weeklybox);

  	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON 
					  (s->radiobuttonmonthly))) 
	  gtk_widget_show (s->monthlybox);
	else
	  gtk_widget_hide (s->monthlybox);
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON 
					  (s->radiobuttonyearly))) 
	  gtk_widget_show (s->yearlybox);
	else
	  gtk_widget_hide (s->yearlybox);
    }
}

static void
schedule_alarm(event_t ev)
{
  time_t alarm_t;
  struct tm tm;
  char alrm_date[32], alrm_time[32];
  char uscheduleidcmd [300], uschedulecmd[135];
    
  /* set uschedule cmd */
  sprintf(uscheduleidcmd, "/usr/bin/uschedulecmd --id=%d \"export DISPLAY=:0.0\n/usr/bin/announce \'You have an appointment\'\"\n", ev->uid);
  system(uscheduleidcmd);
  localtime_r (&(ev->start), &tm);
  strftime (alrm_date, sizeof(alrm_date), "%Y-%m-%d", &tm);
  alarm_t=ev->start-60*ev->alarm;
  localtime_r (&alarm_t, &tm);
  strftime (alrm_time, sizeof(alrm_time), "%H:%M:%S", &tm);
  sprintf(uschedulecmd, "/usr/bin/uschedule %d \'%s %s\'\n", ev->uid, alrm_date, alrm_time);
  system(uschedulecmd);    	      
}

static void
unschedule_alarm(event_t ev)
{
  char uschedulerm[30];
  
  if(ev->alarm!=-1) {
    sprintf(uschedulerm,"/usr/bin/uschedulerm %d\n", ev->uid);
    system(uschedulerm);
    sprintf(uschedulerm,"/usr/bin/uschedulerm -c %d\n", ev->uid);
    system(uschedulerm);
  }
}

static void
click_delete(GtkWidget *widget, event_t ev)
{
  GtkWidget *d=gtk_widget_get_toplevel(widget);
  
  event_db_remove (ev);
  unschedule_alarm(ev);
  
  update_current_view ();
  
  gtk_widget_hide(d);
  gtk_widget_destroy(d);
}

static void
click_ok(GtkWidget *widget,
	 GtkWidget *d)
{
  struct edit_state *s = gtk_object_get_data (GTK_OBJECT (d), "edit_state");
  event_t ev = event_db_new ();
  event_details_t ev_d;
  struct tm tm, tm2;
  struct tm tm_start, tm_end;
  char *start = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO 
						      (s->starttime)->entry), 
					0, -1);
  char *end = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO 
						    (s->endtime)->entry),
				      0, -1);

  ev_d = event_db_alloc_details (ev);
  ev_d->description = gtk_editable_get_chars (GTK_EDITABLE (s->text), 
						     0, -1);
  memset(&tm, 0, sizeof(tm));

  tm.tm_year = GTK_DATE_COMBO (s->startdate)->year - 1900;
  tm.tm_mon = GTK_DATE_COMBO (s->startdate)->month;
  tm.tm_mday = GTK_DATE_COMBO (s->startdate)->day;
  tm.tm_sec = 0;

  tm.tm_hour = 0;
  tm.tm_min = 0;
  ev->duration = 24 * 60 * 60;

  if (strptime (start, "%X", &tm_start))
    {
      tm.tm_hour = tm_start.tm_hour;
      tm.tm_min = tm_start.tm_min;
      
      if (strptime (end, "%X", &tm_end))
	{
	  ev->duration = ((tm_end.tm_hour - tm_start.tm_hour) * 60
			  + (tm_end.tm_min - tm_start.tm_min)) * 60;
	}
    }

  g_free (end);
  g_free (start);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->alarmbutton)))
	ev->alarm=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->alarmspin));
  else
        ev->alarm=-1;
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonnone))) 
  	ev->recur.type=RECUR_NONE;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttondaily))) 
  	ev->recur.type=RECUR_DAILY;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonweekly))) 
  	ev->recur.type=RECUR_WEEKLY;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonmonthly))) 
  	ev->recur.type=RECUR_MONTHLY;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonyearly))) 
  	ev->recur.type=RECUR_YEARLY;
  
  ev->start = mktime (&tm);

  localtime_r (&ev->start, &tm2);
  if (tm2.tm_isdst)
    {
      tm.tm_isdst = tm2.tm_isdst;
      ev->start = mktime (&tm);
    }

  if (event_db_add (ev) == FALSE)
    {
      event_db_destroy (ev);
    }

  if (ev->alarm!=-1)  schedule_alarm(ev);
  
  update_current_view ();
      
  gtk_widget_hide(d);
  gtk_widget_destroy(d);
}

static void
click_cancel(GtkWidget *widget,
	   GtkWidget *d)
{
  gtk_widget_hide(d);
  gtk_widget_destroy(d);
}

static GtkWidget *
edit_event_window(event_t ev)
{
  static const nl_item days[] = { ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, 
				  ABDAY_6, ABDAY_7, ABDAY_1 };
  int i;
  
  GSList *vboxrecur_group, *vboxend_group;
  GtkWidget *radiobuttonforever, *radiobuttonendafter;
  GtkWidget *endspin, *endlabel;
  GtkWidget *radiobuttonnone, *radiobuttondaily, *radiobuttonweekly, 
    *radiobuttonmonthly, *radiobuttonyearly;
  GtkWidget *recurborder, *recurendborder;
  GtkWidget *vboxrepeattop;

  GtkWidget *dailylabelevery, *dailyspin, *dailylabels;
  GtkWidget *monthlylabelevery, *monthlyspin, *monthlylabels;
  GtkWidget *yearlylabelevery, *yearlyspin, *yearlylabels;

  GtkAdjustment *endspin_adj, *dailyspin_adj, *monthlyspin_adj, *yearlyspin_adj;
   
  GtkWidget *notebook = gtk_notebook_new ();
  GtkWidget *labeleventpage = gtk_label_new (_("Event"));
  GtkWidget *labelrecurpage = gtk_label_new (_("Recurrence"));
  GtkWidget *labelalarmpage = gtk_label_new (_("Alarm"));

  GtkWidget *vboxevent = gtk_vbox_new (FALSE, 0);
  GtkWidget *vboxrepeat = gtk_vbox_new (FALSE, 0);
  GtkWidget *vboxalarm = gtk_vbox_new (FALSE, 0);
  
  GtkWidget *vboxend = gtk_vbox_new (FALSE, 0);
  GtkWidget *vboxtop = gtk_vbox_new (FALSE, 0);
  
  GtkWidget *vboxrecur = gtk_vbox_new (FALSE, 0);
  GtkWidget *hboxrecurtypes = gtk_hbox_new (FALSE, 0);
  GtkWidget *hboxendafter = gtk_hbox_new (FALSE, 0);
  
  GtkWidget *text = gtk_text_new (NULL, NULL);

  GtkWidget *buttonbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonok = gtk_button_new_with_label (_("Save"));
  GtkWidget *buttoncancel = gtk_button_new_with_label (_("Cancel"));
  GtkWidget *buttondelete = gtk_button_new_with_label (_("Delete"));

  GtkWidget *starttime = gtk_combo_new ();
  GtkWidget *endtime = gtk_combo_new ();

  GtkWidget *startdatebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *enddatebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *startdatelabel = gtk_label_new (_("Start on:"));
  GtkWidget *enddatelabel = gtk_label_new (_("End on:"));
  GtkWidget *starttimelabel = gtk_label_new (_("at:"));
  GtkWidget *endtimelabel = gtk_label_new (_("at:"));

  GtkWidget *summaryhbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *summaryentry = gtk_entry_new ();
  GtkWidget *summarylabel = gtk_label_new (_("Summary:"));

  GtkWidget *descriptionframe = gtk_frame_new (_("Description:"));

  GtkWidget *alldaybutton = 
    gtk_check_button_new_with_label (_("All-day event"));

  GtkWidget *alarmhbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *alarmmenu = gtk_menu_new ();
  GtkWidget *alarmbutton = gtk_check_button_new_with_label (_("Alarm"));
  GtkObject *alarmadj = gtk_adjustment_new (5.0, 1.0, 100.0, 1.0, 5.0, 5.0);
  GtkWidget *alarmspin = gtk_spin_button_new (GTK_ADJUSTMENT (alarmadj), 
					      1.0, 0);
  GtkWidget *alarmoption = gtk_option_menu_new ();
  GtkWidget *alarmbefore = gtk_label_new (_("before event"));

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  struct edit_state *s = g_malloc (sizeof (struct edit_state));

  memset (s, 0, sizeof (*s));

/* Begin event vbox */
  gtk_combo_set_popdown_strings (GTK_COMBO (starttime), times);
  gtk_combo_set_popdown_strings (GTK_COMBO (endtime), times);

  s->startdate = gtk_date_combo_new ();
  s->enddate = gtk_date_combo_new ();
  
  gtk_box_pack_end (GTK_BOX (startdatebox), starttime, TRUE, TRUE, 2);
  gtk_box_pack_end (GTK_BOX (startdatebox), starttimelabel, FALSE, FALSE, 2);
  gtk_box_pack_end (GTK_BOX (startdatebox), s->startdate, TRUE, TRUE, 2);
  gtk_box_pack_end (GTK_BOX (startdatebox), startdatelabel, FALSE, FALSE, 2);

  gtk_box_pack_end (GTK_BOX (enddatebox), endtime, TRUE, TRUE, 2);
  gtk_box_pack_end (GTK_BOX (enddatebox), endtimelabel, FALSE, FALSE, 2);
  gtk_box_pack_end (GTK_BOX (enddatebox), s->enddate, TRUE, TRUE, 2);
  gtk_box_pack_end (GTK_BOX (enddatebox), enddatelabel, FALSE, FALSE, 2);

  gtk_text_set_editable (GTK_TEXT (text), TRUE);
  gtk_text_set_word_wrap (GTK_TEXT (text), TRUE);
  gtk_widget_set_usize (text, -1, 88);

  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
		      GTK_SIGNAL_FUNC (click_ok), window);
  gtk_signal_connect (GTK_OBJECT (buttondelete), "clicked",
		      GTK_SIGNAL_FUNC (click_delete), ev);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
		      GTK_SIGNAL_FUNC (click_cancel), window);

  gtk_container_add (GTK_CONTAINER (descriptionframe), text);
  gtk_container_set_border_width (GTK_CONTAINER (descriptionframe), 2);

  gtk_box_pack_start (GTK_BOX (summaryhbox), summarylabel, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (summaryhbox), summaryentry, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (vboxevent), startdatebox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vboxevent), enddatebox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vboxevent), alldaybutton, FALSE, FALSE, 2);  
  gtk_box_pack_start (GTK_BOX (vboxevent), summaryhbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxevent), descriptionframe, TRUE, TRUE, 2);
  
  gtk_widget_show_all (vboxevent);
  
/* Begin repeat vbox */

  gtk_widget_show (vboxrepeat);
  
  vboxrepeattop = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vboxrepeattop);
  
  recurborder = gtk_frame_new (_("Type:"));
  gtk_box_pack_start_show (GTK_BOX (vboxrepeattop), recurborder, 
			   TRUE, TRUE, 0); 
  gtk_container_add (GTK_CONTAINER (recurborder), vboxrepeat);
   
  vboxrecur = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (vboxrepeat), vboxrecur, FALSE, FALSE, 0);

/* recurrence radio buttons */
  radiobuttonnone = gtk_radio_button_new_with_label (NULL, _("No recurrence"));
  gtk_box_pack_start_show (GTK_BOX (vboxrecur), radiobuttonnone, 
			   FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobuttonnone), TRUE);
  
  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonnone));
  radiobuttondaily = gtk_radio_button_new_with_label (vboxrecur_group, _("Daily"));
  gtk_box_pack_start_show (GTK_BOX (hboxrecurtypes), radiobuttondaily, 
			   FALSE, FALSE, 0);

  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttondaily));
  radiobuttonweekly = gtk_radio_button_new_with_label (vboxrecur_group, _("Weekly"));
  gtk_box_pack_start_show (GTK_BOX (hboxrecurtypes), radiobuttonweekly, 
			   FALSE, FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (vboxrecur), hboxrecurtypes, FALSE, FALSE, 0);

  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonweekly));
  radiobuttonmonthly = gtk_radio_button_new_with_label (vboxrecur_group, _("Monthly"));
  gtk_box_pack_start_show (GTK_BOX (hboxrecurtypes), radiobuttonmonthly, 
			   FALSE, FALSE, 0);

  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonmonthly));
  radiobuttonyearly = gtk_radio_button_new_with_label (vboxrecur_group, _("Yearly"));
  gtk_box_pack_start_show (GTK_BOX (hboxrecurtypes), radiobuttonyearly, 
			   FALSE, FALSE, 0);

  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonyearly));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobuttonnone), TRUE);

  gtk_signal_connect (GTK_OBJECT (radiobuttonnone), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttondaily), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttonweekly), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttonmonthly), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttonyearly), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  
/* daily vbox */
  s->dailybox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (vboxrepeat), s->dailybox, 
			   TRUE, FALSE, 0);

  /* "every" label */
  dailylabelevery = gtk_label_new (_("Every"));
  gtk_box_pack_start_show (GTK_BOX (s->dailybox), dailylabelevery, 
			   FALSE, FALSE, 0);
  
  /* daily spinner */
  dailyspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  dailyspin = gtk_spin_button_new (GTK_ADJUSTMENT (dailyspin_adj), 1, 0);
  gtk_box_pack_start_show (GTK_BOX (s->dailybox), dailyspin, TRUE, TRUE, 0);

  /* days label */
  dailylabels = gtk_label_new (_("day(s)"));
  gtk_box_pack_start_show (GTK_BOX (s->dailybox), dailylabels, 
			   FALSE, FALSE, 0);
  
/* weekly box */
  s->weeklybox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (vboxrepeat), s->weeklybox, TRUE, FALSE, 0);

/* weekly hbox1 */
  {
    GtkWidget *weeklyhbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start_show (GTK_BOX (s->weeklybox), weeklyhbox1, FALSE, FALSE, 0);
    for (i = 0; i < 4; i++)
      {
	GtkWidget *b = gtk_check_button_new_with_label (nl_langinfo(days[i]));
	gtk_box_pack_start_show (GTK_BOX (weeklyhbox1), b, FALSE, FALSE, 0);
	s->checkbuttonwday[i] = b;
      }
  }

/* weekly hbox2 */
  {
    GtkWidget *weeklyhbox2 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start_show (GTK_BOX (s->weeklybox), weeklyhbox2, FALSE, FALSE, 0);
    for (i = 4; i < 7; i++)
      {
	GtkWidget *b = gtk_check_button_new_with_label (nl_langinfo(days[i]));
	gtk_box_pack_start_show (GTK_BOX (weeklyhbox2), b, FALSE, FALSE, 0);
	s->checkbuttonwday[i] = b;
      }
  }    

/* monthly hbox */
  s->monthlybox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (vboxrepeat), s->monthlybox, TRUE, FALSE, 0);

  /* "every" label */
  monthlylabelevery = gtk_label_new (_("Every"));
  gtk_box_pack_start_show (GTK_BOX (s->monthlybox), monthlylabelevery, 
			   FALSE, FALSE, 0);
  
  /* monthly spinner */
  monthlyspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  monthlyspin = gtk_spin_button_new (GTK_ADJUSTMENT (monthlyspin_adj), 1, 0);
  gtk_box_pack_start_show (GTK_BOX (s->monthlybox), monthlyspin, 
			   TRUE, TRUE, 0);

  /* months label */
  monthlylabels = gtk_label_new (_("month(s)"));
  gtk_box_pack_start_show (GTK_BOX (s->monthlybox), monthlylabels, 
			   FALSE, FALSE, 0);
  
/* yearly hbox */
  s->yearlybox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (vboxrepeat), s->yearlybox, TRUE, FALSE, 0);

  /* "every" label */
  yearlylabelevery = gtk_label_new (_("Every"));
  gtk_box_pack_start_show (GTK_BOX (s->yearlybox), yearlylabelevery, 
			   FALSE, FALSE, 0);
  
  /* yearly spinner */
  yearlyspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  yearlyspin = gtk_spin_button_new (GTK_ADJUSTMENT (yearlyspin_adj), 1, 0);
  gtk_box_pack_start_show (GTK_BOX (s->yearlybox), yearlyspin, TRUE, TRUE, 0);

  /* years label */
  yearlylabels = gtk_label_new (_("year(s)"));
  gtk_box_pack_start_show (GTK_BOX (s->yearlybox), yearlylabels, 
			   FALSE, FALSE, 0);
  
  gtk_widget_hide (s->dailybox);
  gtk_widget_hide (s->weeklybox);
  gtk_widget_hide (s->monthlybox);
  gtk_widget_hide (s->yearlybox);

/* Begin end-date vbox */
 
  gtk_widget_show (vboxend);

  recurendborder = gtk_frame_new (_("End Date:"));
  gtk_container_add (GTK_CONTAINER (recurendborder), vboxend);
  gtk_box_pack_end (GTK_BOX (vboxrepeattop), recurendborder, 
		    FALSE, FALSE, 0);
  gtk_widget_show (recurendborder);
  
  /* forever radio button */
  radiobuttonforever = gtk_radio_button_new_with_label (NULL, _("forever"));
  gtk_box_pack_start_show (GTK_BOX (vboxend), radiobuttonforever, 
			   FALSE, FALSE, 0);
  gtk_widget_set_sensitive (radiobuttonforever , FALSE);

  /* endafter hbox */
  hboxendafter = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (vboxend), hboxendafter, FALSE, FALSE, 0);

  /* end after radio button */
  vboxend_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonforever));
  radiobuttonendafter = gtk_radio_button_new_with_label (vboxend_group, 
							 _("end after"));
  gtk_box_pack_start_show (GTK_BOX (hboxendafter), radiobuttonendafter, 
			   FALSE, FALSE, 0);
  gtk_widget_set_sensitive (radiobuttonendafter, FALSE);
  vboxend_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonendafter));
  		  
  /* end after spinner */
  endspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  endspin = gtk_spin_button_new (GTK_ADJUSTMENT (endspin_adj), 1, 0);
  gtk_widget_set_usize (endspin, 50, 20);
  gtk_box_pack_start_show (GTK_BOX (hboxendafter), endspin, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (endspin, FALSE);

  /* events label */
  endlabel= gtk_label_new (_("event(s)"));
  gtk_box_pack_start_show (GTK_BOX (hboxendafter), endlabel, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (endlabel, FALSE);

  gtk_signal_connect (GTK_OBJECT (radiobuttonforever), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttonendafter), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  		  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobuttonforever), TRUE);

  /* begin alarm page */
  gtk_menu_append (GTK_MENU (alarmmenu), 
		   gtk_menu_item_new_with_label (_("minutes")));
  gtk_menu_append (GTK_MENU (alarmmenu), 
		   gtk_menu_item_new_with_label (_("hours")));
  gtk_menu_append (GTK_MENU (alarmmenu), 
		   gtk_menu_item_new_with_label (_("days")));
  gtk_menu_append (GTK_MENU (alarmmenu), 
		   gtk_menu_item_new_with_label (_("weeks")));
  gtk_menu_append (GTK_MENU (alarmmenu), 
		   gtk_menu_item_new_with_label (_("months")));
  gtk_menu_append (GTK_MENU (alarmmenu), 
		   gtk_menu_item_new_with_label (_("years")));
		
  gtk_option_menu_set_menu (GTK_OPTION_MENU (alarmoption), alarmmenu);
  gtk_widget_set_usize (alarmoption, 120, -1);

  gtk_signal_connect (GTK_OBJECT (alarmbutton), "clicked",
		     GTK_SIGNAL_FUNC (recalculate_sensitivities), window);

  gtk_box_pack_start (GTK_BOX (alarmhbox), alarmspin, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (alarmhbox), alarmoption, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (alarmhbox), alarmbefore, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vboxalarm), alarmbutton, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxalarm), alarmhbox, FALSE, FALSE, 0);

  gtk_widget_show_all (vboxalarm);

  gtk_widget_show (labeleventpage);
  gtk_widget_show (labelalarmpage);
  gtk_widget_show (labelrecurpage);
  
  gtk_box_pack_start_show (GTK_BOX (buttonbox), buttondelete, TRUE, FALSE, 4);
  gtk_box_pack_start_show (GTK_BOX (buttonbox), buttoncancel, TRUE, FALSE, 4);
  gtk_box_pack_start_show (GTK_BOX (buttonbox), buttonok, TRUE, FALSE, 4);

  gtk_box_pack_start_show (GTK_BOX (vboxtop), notebook, TRUE, TRUE, 2);
  gtk_box_pack_start_show (GTK_BOX (vboxtop), buttonbox, FALSE, FALSE, 2);

  gtk_widget_show (vboxtop);
  gtk_container_add (GTK_CONTAINER (window), vboxtop);
  
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vboxevent, 
			    labeleventpage);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vboxalarm,
			    labelalarmpage);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vboxrepeattop, 
			    labelrecurpage);

  gtk_widget_grab_focus (text);

  s->deletebutton = buttondelete;
  s->starttime = starttime;
  s->endtime = endtime;
  s->alarmbutton = alarmbutton;
  s->alarmspin = alarmspin;
  s->alarmoption = alarmoption;
  s->text = text;
  s->radiobuttonforever = radiobuttonforever;
  s->radiobuttonendafter = radiobuttonendafter;
  s->endspin = endspin;
  s->endlabel = endlabel;
  s->radiobuttonnone = radiobuttonnone;
  s->radiobuttondaily = radiobuttondaily;
  s->radiobuttonweekly = radiobuttonweekly;
  s->radiobuttonmonthly = radiobuttonmonthly;
  s->radiobuttonyearly = radiobuttonyearly;
  s->dailyspin = dailyspin;
  s->endspin_adj = endspin_adj;
  s->dailyspin_adj = dailyspin_adj;
  s->monthlyspin_adj = monthlyspin_adj;
  s->yearlyspin_adj = yearlyspin_adj;
   
  gtk_object_set_data_full (GTK_OBJECT (window), "edit_state", s, 
			    destroy_user_data);

  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);

  recalculate_sensitivities (NULL, window);
  
  return window;
}

GtkWidget *
new_event(time_t t, guint timesel, event_t ev)
{
  GtkWidget *w = edit_event_window (ev);
  event_details_t evd;
  
  if (w)
    {
      time_t end;
      struct tm tm;
      char buf[32];
      struct edit_state *s = gtk_object_get_data (GTK_OBJECT (w), 
						  "edit_state");
      if (ev==NULL) {
	      
	      gtk_widget_set_sensitive (s->deletebutton, FALSE);

	      localtime_r (&t, &tm);
	      strftime (buf, sizeof(buf), "%X", &tm);
	      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (s->starttime)->entry), buf);
	      tm.tm_hour++;
	      strftime (buf, sizeof(buf), "%X", &tm);
	      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (s->endtime)->entry), buf);

	      gtk_date_combo_set_date (GTK_DATE_COMBO (s->startdate),
	        		       tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
	      gtk_date_combo_set_date (GTK_DATE_COMBO (s->enddate),
	        		       tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
      }
      
      else {
     
	      gtk_widget_set_sensitive (s->deletebutton, TRUE);
	      evd=event_db_get_details (ev);
              gtk_text_insert (GTK_TEXT (s->text), NULL, NULL, NULL, evd->description, -1);
	      
	      localtime_r (&(ev->start), &tm);
	      strftime (buf, sizeof(buf), "%X", &tm);
	      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (s->starttime)->entry), buf);
	      end=ev->start+ev->duration;
	      localtime_r (&end, &tm);
	      strftime (buf, sizeof(buf), "%X", &tm);
	      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (s->endtime)->entry), buf);

	      gtk_date_combo_set_date (GTK_DATE_COMBO (s->startdate),
	        		       tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
	      gtk_date_combo_set_date (GTK_DATE_COMBO (s->enddate),
	        		       tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
      }    

     
    }

  return w;
}
