/*
 * Copyright (C) 2002 Colin Marquardt <ipaq@marquardt-home.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h> /* for access() */

#include <X11/Xlib.h>

//#include <gobject.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gpe/errorbox.h>
#include <gpe/init.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/stylus.h>

#include "rootpixmap.h"

// #include "main.h"

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif


#define CURRENT_DATAFILE_VER 2

/* WARNING: don't mess with this! */
#define GPE_OWNERINFO_DATA   "/etc/gpe/gpe-ownerinfo.data"
#define INFO_MATCH           "[gpe-ownerinfo data version "

#define UPGRADE_ERROR      -1
#define UPGRADE_NOT_NEEDED  0

GtkWidget *GPE_Ownerinfo;
GtkWidget *name;
GtkWidget *phone;
GtkWidget *email;
GtkWidget *address;
GdkPixbuf *photopixbuf;
GtkWidget *smallphotobutton;
GtkWidget *bigphotobutton;
gchar *photofile = PREFIX "/share/gpe/pixmaps/default/tux-48.png";


/* redraw the pixbuf */
gboolean
on_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  gint width  = 10;
  gint height = 10;
  guint maxwidth = 32, maxheight = 32;
  guint resultwidth, resultheight;
  gfloat scale, scale_width = 2.72, scale_height = 3.14;
  GdkPixbuf *scaledpixbuf;

  maxwidth  = widget->allocation.width;
  maxheight = widget->allocation.height;
  g_message ("allocation for drawing area: %d x %d", maxwidth, maxheight);

  width  = gdk_pixbuf_get_width (photopixbuf);
  height = gdk_pixbuf_get_height (photopixbuf);
  g_message ("pixbuf: %d x %d", width, height);

  if (width > maxwidth)
    scale_width = (gfloat) maxwidth / width;
  else
    scale_width = 1.0;

  if (height > maxheight)
    scale_height = (gfloat) maxheight / height;
  else
    scale_height = 1.0;

  scale = scale_width < scale_height ? scale_width : scale_height;
  g_message ("scale_width: %f, scale_height: %f, selected scale: %f",
	     scale_width, scale_height, scale);

  resultwidth  = (gint) (width  * scale);
  resultheight = (gint) (height * scale);
  g_message ("resulting size: %d x %d", resultwidth, resultheight);

  scaledpixbuf = gdk_pixbuf_scale_simple (photopixbuf, resultwidth, resultheight, GDK_INTERP_BILINEAR);

  gdk_pixbuf_render_to_drawable_alpha (scaledpixbuf, widget->window,
				       0, 0,                      /* src_x, src_y */
				       (gint) ((maxwidth - resultwidth)/2), 0.5, /* dest_x, dest_y */
				       -1, -1,                    /* use the pixbuf size */
				       GDK_PIXBUF_ALPHA_BILEVEL,  /* ignored */
				       128,                       /* ignored */
				       GDK_RGB_DITHER_NORMAL,     /* dither mode */
				       0, 0);                     /* x_dither, y_dither */
  
  g_message ("======================================================");
  return TRUE;
}

void
on_smallphotobutton_clicked            (GtkButton       *button,
                                        GtkWidget       *notebook)
{
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 1);
}

void
on_bigphotobutton_clicked              (GtkButton       *button,
                                        GtkWidget       *notebook)
{
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 0);
}


/*
 *  Return value: upgrade status
 *
 *  The data file format is described in the file HACKING.
 */

gint
upgrade_to_v2 (guint new_version)
{
  gchar *firstline, *oldcontent;
  FILE *fp;
  
  /* firstline = g_strdup ("Initial firstline"); */
#warning FIXME: Why doesnt this work?
  /* sprintf (firstline, "%s %d]", INFO_MATCH, new_version); */
  /* sprintf (firstline, INFO_MATCH "%d]", INFO_MATCH, new_version); maybe??? */
  firstline =  g_strdup ("[gpe-ownerinfo data version 2]");
  oldcontent = g_strdup ("Initial oldcontent.");
  
  fp = fopen (GPE_OWNERINFO_DATA, "r");
  if (fp)
    {
      char buf[2048];
      guint numchar;
      
      if ((numchar=fread (buf, 1, sizeof (buf), fp)))
      	{
      	  oldcontent = g_strdup (buf);
      	  oldcontent[numchar]='\0';
      	}

      printf("oldcontent:\n%s\n", oldcontent);
      
      fclose (fp);
    }
  else /* fp == NULL */
    { 
      perror (GPE_OWNERINFO_DATA);
    }
  
  fp = fopen (GPE_OWNERINFO_DATA, "w");
  if (fp)
    {
      fputs (firstline, fp);
      fputs ("\n", fp);

      fputs (PREFIX "/share/gpe/pixmaps/default/tux-48.png", fp);
      fputs ("\n", fp);

      fputs (oldcontent, fp);
      
      printf ("gpe-ownerinfo: Migrated data file '%s' to version %d.\n",
	      GPE_OWNERINFO_DATA, new_version);

      fclose (fp);
    }
  else /* fp == NULL */
    { 
      perror (GPE_OWNERINFO_DATA);
      return (UPGRADE_ERROR);
    }

  return (new_version);
}


gint
maybe_upgrade_datafile ()
{
  gchar *firstline;
  guint version = 0, idx = 0;
  gint upgrade_result = UPGRADE_ERROR;
  FILE *fp;

  fp = fopen (GPE_OWNERINFO_DATA, "r");
  if (fp)
    {
      char buf[2048];
      if (fgets (buf, sizeof (buf), fp))
	{
	  firstline = g_strdup (buf);
	  firstline[strlen (firstline)-1]='\0';
	  
	  /*
	   * looking for a string like
	   *   [gpe-ownerinfo data version 42]
	   */
	  
	  if (strstr (firstline, INFO_MATCH)) { /* found magic string */
	    version = strtol (firstline + strlen (INFO_MATCH), (char **)NULL, 10);
	    
	    if (version == 0) {
	      fprintf (stderr, "gpe-ownerinfo: file '%s' is version %d, which should not happen.\n",
		       GPE_OWNERINFO_DATA, version);
	      fprintf (stderr, "   Please file a bug. I am continuing anyway.\n");
	    }
	    if (version > CURRENT_DATAFILE_VER) {
	      fprintf (stderr, "gpe-ownerinfo: file '%s' is version %d.\n   I only know how to handle version %d. Exiting.\n",
		       GPE_OWNERINFO_DATA, version, CURRENT_DATAFILE_VER);
	      fclose (fp);
	      exit (1);
	    }
	  }
	  else { /* must be version 1 which didn't have a version indicator */
	    version = 1;
	  }
	}
      fclose (fp);
      
      if (version != CURRENT_DATAFILE_VER) {
	for (idx = version+1; idx <= CURRENT_DATAFILE_VER; idx++) {
	  printf ("Need to upgrade. idx is %d\n", idx);
	  switch (idx) {
	  case 0:
	    break;
	  case 1:
	    break;
	  case 2: /* from here, it's cumulative upgrades */
	    upgrade_result = upgrade_to_v2 (idx);
	  case 3:
	    /* upgrade_result = upgrade_to_v3 (fp, idx); */ /* ...and so on */
	    break;
	  }
	}
      } else {
	upgrade_result = UPGRADE_NOT_NEEDED;
      }
    }
  else /* fp == NULL */
    { 
      perror (GPE_OWNERINFO_DATA);
    }

  if (upgrade_result == UPGRADE_ERROR) {
    printf ("Had found version %d, but cannot upgrade for some reason.\n", version);
    return (UPGRADE_ERROR);
  }
  else {
    printf ("Had found version %d.\n", version);
    return UPGRADE_NOT_NEEDED;
  }
}

/* http://www.gtk.org/tutorial/sec-thedrawingareawidget.html */

// /* Backing pixmap for drawing area */
// static GdkPixmap *pixmap = NULL;
// 
// /* Create a new backing pixmap of the appropriate size */
// static gint
// configure_event (GtkWidget *widget, GdkEventConfigure *event)
// {
//   if (pixmap)
//     gdk_pixmap_unref(pixmap);
// 
//   pixmap = gdk_pixmap_new(widget->window,
// 			  widget->allocation.width,
// 			  widget->allocation.height,
// 			  -1);
//   gdk_draw_rectangle (pixmap,
// 		      widget->style->white_gc,
// 		      TRUE,
// 		      0, 0,
// 		      widget->allocation.width,
// 		      widget->allocation.height);
// 
//   return TRUE;
// }
// 


int
main (int argc, char *argv[])
{
  GtkWidget *GPE_Ownerinfo;
  GtkWidget *notebook;
  GtkWidget *mainvbox;
  GtkWidget *catlabel;
  GtkWidget *cathbox;
  GtkWidget *indentlabel;
  GtkWidget *indentedhbox;
  GtkWidget *leftcolvbox;
  GtkWidget *namelabel;
  GtkWidget *phonelabel;
  GtkWidget *emaillabel;
  GtkWidget *addresslabel;
  GtkWidget *address_button_vbox;
  GtkWidget *smallphotodrawingarea;
  GtkWidget *bigphotodrawingarea;
  GtkWidget *rightcolvbox;
  GtkWidget *scrolledwindow;
  GtkWidget *viewport;
  
  gchar *gpe_catindent = gpe_get_catindent ();
  //guint gpe_catspacing = gpe_get_catspacing ();
  guint gpe_boxspacing = gpe_get_boxspacing ();
  //guint gpe_border     = gpe_get_border ();

  gchar *ownername, *owneremail, *ownerphone, *owneraddress;
  gchar *ownerphotofile;
  FILE *fp;
  gchar * geometry = NULL;
  gboolean flag_transparent = FALSE;
  gboolean flag_keep_on_top = FALSE;
  gint x = -1, y = -1, h = 0, w = 0;
  gint val;
  gint opt;
  gint upgrade_result = UPGRADE_ERROR;
  
#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif

  while ((opt = getopt (argc,argv,"hktg:")) != EOF)
    {
      switch (opt) {
      case 'g':
	geometry = optarg;
	break;
      case 't':
	flag_transparent = TRUE;
	break;
      case 'k':
	flag_keep_on_top = TRUE;
	break;
      case 'h':
	printf ("GPE Owner Info $Revision$\n");
	printf ("\n");
	printf ("Valid options:\n");
	printf ("   -g GEOMETRY  window geometry (default: 240x120+0+200)\n");
	printf ("   -t           make window transparent\n");
	printf ("   -k           always keep window on top (override redirect)\n");
	printf ("   -h           this help text\n");
	exit (1);
      case '?':
	if (isprint (optopt))
	  ;
	/* fprintf (stderr, "gpe-ownerinfo: Unknown option -%c'.\n", optopt); */
	else
	  fprintf (stderr,
		   "gpe-ownerinfo: Unknown option character \\x%x'.\n",
		   optopt);
	break;
      default:
	fprintf (stderr,
		 "gpe-ownerinfo: Unknown error while parsing command line. Command was %c\n",
		 opt);
	exit (1);
      }
    }
    
  ownername    = g_strdup ("GPE User");
  owneremail   = g_strdup ("nobody@localhost.localdomain");
  ownerphone   = g_strdup ("+99 (9999) 999-9999");
  owneraddress = g_strdup ("The user 'root' can use\n\"GPE Configuration\" ('Owner')\nto change this data.");

  fp = fopen (GPE_OWNERINFO_DATA, "r");
  if (fp)
    {
      char buf[2048];
      guint numchar;
      gchar *firstline;

      upgrade_result = maybe_upgrade_datafile ();

      printf ("upgrade_result: %d\n", upgrade_result);
      
      if (upgrade_result == UPGRADE_NOT_NEEDED) {	
	/*  we have at least version 2, so we need to skip the 1st line
	 *  and read the photo file name:
	 */
	if (fgets (buf, sizeof (buf), fp))
	  {
	    firstline = g_strdup (buf);
	    firstline[strlen (firstline)-1]='\0';
	  }
	if (fgets (buf, sizeof (buf), fp))
	  {
	    ownerphotofile = g_strdup (buf);
	    ownerphotofile[strlen (ownerphotofile)-1]='\0';
	    if (access (ownerphotofile, R_OK) == 0)
	      photofile = g_strdup (ownerphotofile);
	    else
	      fprintf (stderr, "gpe-ownerinfo: file '%s' could not be found,\n   using default file '%s'.\n",
		       ownerphotofile, photofile);
	  }
      }
      else {
	/* upgrade went wrong, need to handle old format */
	/* Note: handle all possible old formats here */
      }
      /* now get the rest of the data: */
      if (fgets (buf, sizeof (buf), fp))
        {
          ownername = g_strdup (buf);
          ownername[strlen (ownername)-1]='\0';
        }
      if (fgets (buf, sizeof (buf), fp))
        {
          owneremail = g_strdup (buf);
          owneremail[strlen (owneremail)-1]='\0';
        }
      if (fgets (buf, sizeof (buf), fp))
        {
	  ownerphone = g_strdup (buf);
	  ownerphone[strlen (ownerphone)-1]='\0';
        }
      /* the rest is taken as address: */
      if ((numchar=fread (buf, 1, sizeof (buf), fp)))
      	{
	  owneraddress = g_malloc (numchar + 1);
	  memcpy (owneraddress, buf, numchar);
	  owneraddress[numchar] = '\0';
      	}

      fclose (fp);
    }
  else /* fp == NULL */
    {
      /* show default info: */
      /*
      gpe_perror_box (GPE_OWNERINFO_DATA);
      exit (1);
      */
    }

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  /* FIXME: check error */
  photopixbuf = gdk_pixbuf_new_from_file (photofile, NULL);

  /* draw the GUI */
  GPE_Ownerinfo = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (GPE_Ownerinfo, "GPE_Ownerinfo");
  gtk_object_set_data (GTK_OBJECT (GPE_Ownerinfo), "GPE_Ownerinfo", GPE_Ownerinfo);
  //gtk_widget_set_usize (GPE_Ownerinfo, 240, 120);
  //gtk_container_set_border_width (GTK_CONTAINER (GPE_Ownerinfo), gpe_border);
  gtk_window_set_title (GTK_WINDOW (GPE_Ownerinfo), _("GPE Owner Info"));

  /* notebook with two pages;
   * page 1 holds the small photo and the info text,
   * page 2 just the big photo:
   */
  notebook = gtk_notebook_new ();
  gtk_widget_show (notebook);
  GTK_WIDGET_UNSET_FLAGS (notebook, GTK_CAN_FOCUS);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_container_add (GTK_CONTAINER (GPE_Ownerinfo), notebook);

  /*
   * The first notebook page
   */
  mainvbox = gtk_vbox_new (FALSE, 0);
  //mainvbox = gtk_vbox_new (FALSE, gpe_catspacing);
  gtk_widget_show (mainvbox);
  gtk_container_add (GTK_CONTAINER (notebook), mainvbox);

  catlabel = gtk_label_new (_("Owner Information"));
  /* FIXME: what about gettext here? */
  gtk_label_set_markup (GTK_LABEL (catlabel), "<span weight='bold'>Owner Information</span>");
  /* <span weight="bold">Category Header</span>
     Set the label property Use Markup to Yes, and X Align to 0.0 to left align the label.
  */
  gtk_widget_show (catlabel);
  gtk_box_pack_start (GTK_BOX (mainvbox), catlabel, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (catlabel), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (catlabel), 0, 0.5);

  cathbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (cathbox);
  gtk_box_pack_start (GTK_BOX (mainvbox), cathbox, TRUE, TRUE, 0);

  indentlabel = gtk_label_new (gpe_catindent);
  gtk_widget_show (indentlabel);
  gtk_box_pack_start (GTK_BOX (cathbox), indentlabel, FALSE, FALSE, 0);

  indentedhbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (indentedhbox);
  gtk_box_pack_start (GTK_BOX (cathbox), indentedhbox, TRUE, TRUE, 0);

  leftcolvbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (leftcolvbox);
  gtk_box_pack_start (GTK_BOX (indentedhbox), leftcolvbox, FALSE, FALSE, gpe_boxspacing);

  namelabel = gtk_label_new (_("Name:"));
  gtk_widget_show (namelabel);
  gtk_box_pack_start (GTK_BOX (leftcolvbox), namelabel, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (namelabel), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (namelabel), 0, 0.5);

  emaillabel = gtk_label_new (_("E-Mail:"));
  gtk_widget_show (emaillabel);
  gtk_box_pack_start (GTK_BOX (leftcolvbox), emaillabel, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (emaillabel), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (emaillabel), 0, 0.5);

  phonelabel = gtk_label_new (_("Phone:"));
  gtk_widget_show (phonelabel);
  gtk_box_pack_start (GTK_BOX (leftcolvbox), phonelabel, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (phonelabel), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (phonelabel), 0, 0.5);

  address_button_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (address_button_vbox);
  gtk_box_pack_start (GTK_BOX (leftcolvbox), address_button_vbox, TRUE, TRUE, 0);

  addresslabel = gtk_label_new (_("Address:"));
  gtk_widget_show (addresslabel);
  gtk_box_pack_start (GTK_BOX (address_button_vbox), addresslabel, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (addresslabel), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (addresslabel), 0, 0);

  smallphotobutton = gtk_button_new ();
  gtk_widget_show (smallphotobutton);
  //  GTK_WIDGET_UNSET_FLAGS (smallphotobutton, GTK_CAN_FOCUS);
  //  gtk_button_set_relief (GTK_BUTTON (smallphotobutton), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (address_button_vbox), smallphotobutton, TRUE, TRUE, 0);

  smallphotodrawingarea = gtk_drawing_area_new ();
  gtk_widget_add_events (GTK_WIDGET (smallphotodrawingarea),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (G_OBJECT (smallphotodrawingarea), "expose_event",  
                    G_CALLBACK (on_expose_event), NULL);
  gtk_widget_show (smallphotodrawingarea);
  gtk_container_add (GTK_CONTAINER (smallphotobutton), smallphotodrawingarea);
  
  rightcolvbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (rightcolvbox);
  gtk_box_pack_start (GTK_BOX (indentedhbox), rightcolvbox, TRUE, TRUE, 0);

  name = gtk_label_new (_("Foo Bar"));
  gtk_widget_show (name);
  gtk_box_pack_start (GTK_BOX (rightcolvbox), name, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (name), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (name), 0, 0.5);

  email = gtk_label_new (_("foo@localhost"));
  gtk_widget_show (email);
  gtk_box_pack_start (GTK_BOX (rightcolvbox), email, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (email), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (email), 0, 0.5);

  phone = gtk_label_new (_("999 9999"));
  gtk_widget_show (phone);
  gtk_box_pack_start (GTK_BOX (rightcolvbox), phone, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (phone), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (phone), 0, 0.5);

  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow);
  gtk_box_pack_start (GTK_BOX (rightcolvbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_widget_show (viewport);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), viewport);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);

  address = gtk_label_new (_("Address 1\nAddress 2\nAddress 3\nAddress 4"));
  gtk_widget_show (address);
  gtk_container_add (GTK_CONTAINER (viewport), address);
  gtk_label_set_justify (GTK_LABEL (address), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (address), 0, 0);

  /*
   * The second notebook page
   */
  bigphotobutton = gtk_button_new ();
  gtk_widget_show (bigphotobutton);
  GTK_WIDGET_UNSET_FLAGS (bigphotobutton, GTK_CAN_FOCUS);
  gtk_button_set_relief (GTK_BUTTON (bigphotobutton), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (notebook), bigphotobutton);

  bigphotodrawingarea = gtk_drawing_area_new ();
  gtk_widget_add_events (GTK_WIDGET (bigphotodrawingarea),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (G_OBJECT (bigphotodrawingarea), "expose_event",  
                    G_CALLBACK (on_expose_event), NULL);
  gtk_widget_show (bigphotodrawingarea);
  gtk_container_add (GTK_CONTAINER (bigphotobutton), bigphotodrawingarea);
  
  /* end of drawing the GUI */

  
  if (geometry)
    {
      val = XParseGeometry (geometry, &x, &y, &w, &h);
      if ((val & (HeightValue | WidthValue)) == (HeightValue | WidthValue))
	gtk_widget_set_usize (GPE_Ownerinfo, w, h);
      if (val & (XValue | YValue))
	gtk_widget_set_uposition (GPE_Ownerinfo, x, y);
    }
  else
    {
      gtk_widget_set_usize (GPE_Ownerinfo, 240, 120);
      gtk_widget_set_uposition (GPE_Ownerinfo, 0, 200);
    }

  /* make the labels grey: */
//  widget = lookup_widget (GPE_Ownerinfo, "owner_name_label");
//  gtk_rc_parse_string ("widget '*owner_name_label' style 'gpe_ownerinfo_labels'");
//  gtk_widget_set_name (widget, "owner_name_label");
//  widget = lookup_widget (GPE_Ownerinfo, "owner_email_label");
//  gtk_rc_parse_string ("widget '*owner_email_label' style 'gpe_ownerinfo_labels'");
//  gtk_widget_set_name (widget, "owner_email_label");
//  widget = lookup_widget (GPE_Ownerinfo, "owner_phone_label");
//  gtk_rc_parse_string ("widget '*owner_phone_label' style 'gpe_ownerinfo_labels'");
//  gtk_widget_set_name (widget, "owner_phone_label");
//  widget = lookup_widget (GPE_Ownerinfo, "owner_address_label");
//  gtk_rc_parse_string ("widget '*owner_address_label' style 'gpe_ownerinfo_labels'");
//  gtk_widget_set_name (widget, "owner_address_label");
 
  gtk_label_set_text (GTK_LABEL (name), ownername);
  gtk_label_set_text (GTK_LABEL (email), owneremail);
  gtk_label_set_text (GTK_LABEL (phone), ownerphone);
  gtk_label_set_text (GTK_LABEL (address), owneraddress);

  /*
    this widget is shown in the on_smallphotobutton_size_allocate() callback
  widget = lookup_widget (GPE_Ownerinfo, "smallphoto");
  gtk_widget_show (widget);
  */

//  /* make window transparent if option -t is given: */
//  if (flag_transparent) {
//    gtk_signal_connect (GTK_OBJECT (GPE_Ownerinfo), "map-event",
//			GTK_SIGNAL_FUNC (mapped), NULL);
//  }

  gtk_signal_connect (GTK_OBJECT (GPE_Ownerinfo), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (smallphotobutton), "clicked",
		      GTK_SIGNAL_FUNC (on_smallphotobutton_clicked),
		      notebook);
  gtk_signal_connect (GTK_OBJECT (bigphotobutton), "clicked",
		      GTK_SIGNAL_FUNC (on_bigphotobutton_clicked),
		      notebook);

  gtk_widget_show (GPE_Ownerinfo);

  if (flag_keep_on_top) {
    gdk_window_set_override_redirect (GPE_Ownerinfo->window, TRUE);
  }

  gtk_main ();
  return 0;
}

