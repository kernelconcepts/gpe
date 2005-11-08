/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *               2003-2005 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xcms.h>
#include <X11/Xlib.h>
#include <libintl.h>
#include <time.h>
#include "applets.h"
#include "theme.h"
#include "widgets/sp-color-slider.h"
#include "widgets/color.h"

#include <gpe/errorbox.h>
#include <gpe/question.h>
#include <gpe/spacing.h>
#include <xsettings-client.h>

#define KEY_MATCHBOX "MATCHBOX/"
#define KEY_THEME "Net/"
#define KEY_GTK "Gtk/"
#define CMD_XST PREFIX "/bin/xst"
#define CMD_GCONF PREFIX "/bin/gconftool-2"
#define PARAM_RXVT_FONT "Rxvt*font:"

static XSettingsClient *client;
static gboolean use_xst, use_gconf;

static struct
{
  GtkWidget *cbTheme;
  GtkWidget *eImage;
  GtkWidget *rbImgStr,*rbImgTiled,*rbImgCent;
  gchar *themename;
  GtkWidget *bOpen;
  GtkWidget *demolabel2, *demolabel3;
  GtkWidget *spFSApp, *bFontApp;
  GtkWidget *cDefault;
  GtkWidget *rbToolIcons;
  GtkWidget *rbToolText;
  GtkWidget *rbToolBoth;
  GtkWidget *rbToolBothH;
  GtkWidget *spFSTerminal, *bFontTerminal;
#ifndef APPMGR_INTERFACE	
  GtkWidget *rbSolid, *rbGrad, *rbImage;	
  GtkWidget *rbH,*rbV;
  GtkWidget *bColor1,*bColor2;
  GtkWidget *demolabel;
  GtkWidget *spFS, *bFont, *bColorFont;
  GtkWidget *slIconSize;
  GtkWidget *slToolbarSize;
  GtkWidget *cPerformance;
#endif  
}
self;

typedef struct
{
  SPColorSlider *s[3];
  GtkAdjustment *a[3];
  GtkWidget *previewbutton;
}
tcsel;

tcsel *csel;


static char *matchboxpath = PREFIX "/share/themes";

static GList *themeitems;
static GtkWidget *colorbox;

int updating = FALSE;

typedef struct _mbdesktop_bg {
  int type;

  union {
    char *filename;
    int cols[3];
    int gcols[6];
  } data;

} MBDesktopBG;

MBDesktopBG mb_back;

static void select_font_popup (GtkWidget *parent_button);
static void on_font_size_change(GtkSpinButton *spinbutton,GtkScrollType arg1);
static GtkWidget *popup_menu_button_new (const gchar *stock_id);

/* some init stuff related to external tools and libs */
void
init_tools(void)
{
	use_gconf = FALSE;
	use_xst = FALSE;
	
	/* prefer xst */
	if (!access(CMD_XST,X_OK))
		use_xst = TRUE;
	/* to be extended in future */
}

int 
get_terminal_fontsize(void)
{
	gchar *xdefaults;
	FILE *fxdefaults;
	int size = 9;
	
	xdefaults = g_strdup_printf("%s/%s", g_get_home_dir(), ".Xdefaults");
	if (access(xdefaults, R_OK))
	{
		g_free(xdefaults);
		xdefaults = g_strdup("/etc/X11/Xdefaults");
		if (access(xdefaults, R_OK))
		{
			g_free(xdefaults);
			return size;
		}
	}
	
	fxdefaults = fopen(xdefaults, "r");
	if (fxdefaults)
	{
		while (!feof(fxdefaults))
		{
			char buf[128];
			char *p1;
			
			fgets(buf, 128, fxdefaults);
			p1 = strstr(buf, PARAM_RXVT_FONT " xft:");
			if (p1)
			{
				p1 += strlen(PARAM_RXVT_FONT " xft:");
				p1 = strstr(p1, ":");
				p1 = strstr(p1, "=");
				if (p1)
				{
					if (sscanf(p1, "=%i", &size))
						break;
				}
			}
		}
		fclose(fxdefaults);
	}
	return size;
}

char *
get_terminal_fontname (void)
{
	gchar *xdefaults;
	FILE *fxdefaults;
	gchar *name = g_malloc0(128);
	
	xdefaults = g_strdup_printf("%s/%s", g_get_home_dir(), ".Xdefaults");
	if (access(xdefaults, R_OK))
	{
		g_free(xdefaults);
		xdefaults = g_strdup("/etc/X11/Xdefaults");
		if (access(xdefaults, R_OK))
		{
			g_free(xdefaults);
			sprintf(name, "Mono");
			return name;
		}
	}
	
	fxdefaults = fopen(xdefaults, "r");
	if (fxdefaults)
	{
		while (!feof(fxdefaults))
		{
			char buf[128];
			char *p1;
			int i = 0;
			fgets(buf, 128, fxdefaults);
			p1 = strstr(buf, PARAM_RXVT_FONT " xft:");
			if (p1)
			{
				p1 += strlen(PARAM_RXVT_FONT " xft:");
				if (strstr(p1, ":"))
					while (p1[i] != ':')
					{
						name[i] = p1[i];
						i++;
					}
			}
		}
		fclose(fxdefaults);
	}
	if (!name[0]) 
		sprintf(name, "Mono");
	return name;
}

void
set_terminal_font(int size, const gchar *name)
{
	gchar *line, *xdefaults;
	
	xdefaults = g_strdup_printf("%s/%s", g_get_home_dir(), ".Xdefaults");
	line = g_strdup_printf("%s xft:%s:pixelsize=%i",PARAM_RXVT_FONT, name, size);
	file_set_line(xdefaults, PARAM_RXVT_FONT, line);
	g_free(line);
	g_free(xdefaults);
}

void
task_change_background_image(void)
{
	GtkWidget *filesel, *feedbackdlg;
  
	Theme_Build_Objects();
	filesel = gtk_file_selection_new(_("Choose backgound image"));
	gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(filesel), FALSE);
  
	if (gtk_dialog_run(GTK_DIALOG(filesel)) == GTK_RESPONSE_OK)
	{
		int ec = 0;
		const gchar *file = 
			gtk_file_selection_get_filename (GTK_FILE_SELECTION(filesel));
		
		gtk_widget_hide(filesel); 
		ec = access(file, R_OK);
		if (ec)
			feedbackdlg = gtk_message_dialog_new(GTK_WINDOW(filesel),
			  GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
			  _("You are not allowed to read this file, choose another."));
		else
		{
			gchar *confstr = NULL;
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImgCent)))
					confstr = g_strdup_printf("img-centered:%s", file);
				else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImgStr)))
					confstr = g_strdup_printf("img-stretched:%s", file);
				else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImgTiled)))
					confstr = g_strdup_printf("img-tiled:%s", file);
			if (confstr != NULL)
			{
				gchar *p = g_strdup_printf (CMD_XST " write %s%s str '%s'", 
				                            KEY_MATCHBOX, "Background", confstr);
				system(p);
				g_free(p);
				g_free(confstr);
			}
	  
			feedbackdlg = gtk_message_dialog_new(GTK_WINDOW(filesel),
			  GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
			  "%s\n%s",_("Succesfully changed background image."), 
			  _("Use \"Look and Feel\" setup tool to change details."));
		}
		gtk_dialog_run(GTK_DIALOG(feedbackdlg));
		gtk_widget_destroy(feedbackdlg);
	}
	gtk_widget_destroy(filesel);	
}

gboolean
mbbg_parse_spec(MBDesktopBG *mbbg, char *spec)
{
  XColor tmpxcol;
  int i, mapping_cnt, spec_offset = 0, type = 0;
  char *bg_def = NULL, *p = NULL;
  Display *dpy;
  Screen *scr;
  Colormap colormap;
  	
  struct conf_mapping_t {
    char *name;
    int   id;
  } conf_mapping[] = {
    { "img-stretched:",           BG_STRETCHED_PXM  },
    { "img-tiled:",               BG_TILED_PXM      },
    { "img-centered:",            BG_CENTERED_PXM   },
    { "col-solid:",               BG_SOLID          },
    { "col-gradient-vertical:",   BG_GRADIENT_VERT  },
    { "col-gradient-horizontal:", BG_GRADIENT_HORIZ },
  };

  dpy = GDK_DISPLAY();
  scr =  gdk_x11_screen_get_xscreen(gdk_screen_get_default());
  colormap = gdk_x11_colormap_get_xcolormap(gdk_screen_get_default_colormap(gdk_screen_get_default()));
  
  mapping_cnt = (sizeof(conf_mapping)/sizeof(struct conf_mapping_t));

  memset(mbbg, 0, sizeof(mbbg));

  for (i=0; i<mapping_cnt; i++)
	{
	  spec_offset = strlen(conf_mapping[i].name);
	  if (spec_offset < strlen(spec)
	      && !strncmp(conf_mapping[i].name, spec, spec_offset))
	    {
	      type = conf_mapping[i].id;
	      break;
	    }
	}

    if (!type)
	{
	  /* Assume we've just been passed an image filename */
	  mbbg->type = BG_STRETCHED_PXM;
	  mbbg->data.filename = strdup(spec);
	  return True;
	} else bg_def = spec + spec_offset;
    

  mbbg->type = type;
  switch(type)
    {
    case BG_SOLID:
      XParseColor(dpy, colormap, 
		  bg_def, &tmpxcol);
      mbbg->data.cols[0] = tmpxcol.red   >> 8;
      mbbg->data.cols[1] = tmpxcol.green >> 8;
      mbbg->data.cols[2] = tmpxcol.blue  >> 8;
      break;
    case BG_TILED_PXM:
    case BG_CENTERED_PXM:
    case BG_STRETCHED_PXM:
      mbbg->data.filename = strdup(bg_def);
      break;
    case BG_GRADIENT_HORIZ:
    case BG_GRADIENT_VERT:
      p = bg_def;
      while(*p != ',' && *p != '\0') p++;
      if (*p == '\0')
	{
	  return False; 	/* XXX need to reset on fail */
	}
      *p = '\0';
      XParseColor(dpy, colormap, 
		  bg_def, &tmpxcol);
      mbbg->data.gcols[0] = (tmpxcol.red   >> 8);
      mbbg->data.gcols[2] = (tmpxcol.green >> 8);
      mbbg->data.gcols[4] = (tmpxcol.blue  >> 8);
      p++;
      XParseColor(dpy, colormap, 
		  p, &tmpxcol);
      mbbg->data.gcols[1] = (tmpxcol.red   >> 8);
      mbbg->data.gcols[3] = (tmpxcol.green >> 8);
      mbbg->data.gcols[5] = (tmpxcol.blue  >> 8);
      break;
    }    

  return True;
}


void
widget_set_color_rgb8 (GtkWidget * widget,float r, float g, float b)
{
  GtkStyle *astyle;
  GtkRcStyle *rc_style;
  GdkColor gcolor;
  GtkBin bbin;
  float gray;

  if (!GTK_IS_BUTTON(widget))
  {
	 fprintf(stderr,"no button... exiting\n");
	 return;
  }
  
  bbin =  (GtkBin)(GTK_BUTTON(widget)->bin);

  gray = 0.3*r + 0.59*g + 0.11*b;

	
  if (gray > 0.5)
  {
	  gtk_label_set_markup(GTK_LABEL(bbin.child),
	  	g_strdup_printf("<span foreground=\"#000000\"> %s </span>",gtk_label_get_text(GTK_LABEL(bbin.child))));
  }
  else
  {
	  gtk_label_set_markup(GTK_LABEL(bbin.child),
	    g_strdup_printf("<span foreground=\"#ffffff\"> %s </span>",gtk_label_get_text(GTK_LABEL(bbin.child))));
  }
  gcolor.pixel = 0;
  gcolor.red = (int)(r*65534.0);
  gcolor.green = (int)(g*65534.0);
  gcolor.blue = (int)(b*65534.0);
	
  astyle = gtk_widget_get_style (widget);
  rc_style = gtk_rc_style_new ();
  if (astyle)
    {
      rc_style->base[GTK_STATE_NORMAL] = gcolor;
      rc_style->fg[GTK_STATE_NORMAL] = gcolor;
      rc_style->bg[GTK_STATE_NORMAL] = gcolor;
      rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_FG;
      rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_BASE;
      rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_BG;
		
      gtk_widget_modify_style (widget, rc_style);
    }
  gtk_widget_ensure_style (widget);
}

//--------------------------
static void
sp_color_selector_update_sliders (SPColorSlider * csel1, guint channels)
{
  float r,g,b;
  if (updating)
    return;
  updating = TRUE;
  if ((channels != 3) && (channels != 2))
    {
      /* Update red */
      sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[0]),
				  SP_RGBA32_F_COMPOSE (0.0, csel->a[1]->value,
						       csel->a[2]->value,
						       1.0),
				  SP_RGBA32_F_COMPOSE (1.0, csel->a[1]->value,
						       csel->a[2]->value,
						       1.0));
    }
  if ((channels != 3) && (channels != 1))
    {
      /* Update green */
      sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[1]),
				  SP_RGBA32_F_COMPOSE (csel->a[0]->value, 0.0,
						       csel->a[2]->value,
						       1.0),
				  SP_RGBA32_F_COMPOSE (csel->a[0]->value, 1.0,
						       csel->a[2]->value,
						       1.0));
    }
  if ((channels != 1) && (channels != 2))
    {
      /* Update blue */
      sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[2]),
				  SP_RGBA32_F_COMPOSE (csel->a[0]->value,
						       csel->a[1]->value, 0.0,
						       1.0),
				  SP_RGBA32_F_COMPOSE (csel->a[0]->value,
						       csel->a[1]->value, 1.0,
						       1.0));
    }
	
  r = csel->a[0]->value;
  g = csel->a[1]->value;
  b = csel->a[2]->value;
	
  if (GTK_WIDGET_DRAWABLE(csel->previewbutton)) widget_set_color_rgb8(csel->previewbutton,r,g,b);
  updating = FALSE;
}


//#--------------------------

/*******************/
/*   init stuff    */
/*******************/

void update_enabled_widgets()
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.cDefault)))
	{
#ifndef APPMGR_INTERFACE	
		gtk_widget_set_sensitive(self.bColor1,FALSE);
		gtk_widget_set_sensitive(self.bColor2,FALSE);
#endif
		gtk_widget_set_sensitive(self.eImage,FALSE);
		gtk_widget_set_sensitive(self.bOpen,FALSE);
#ifndef APPMGR_INTERFACE	
		gtk_widget_set_sensitive(self.rbH,FALSE);
		gtk_widget_set_sensitive(self.rbV,FALSE);
#endif		
		gtk_widget_set_sensitive(self.rbImgCent,FALSE);
		gtk_widget_set_sensitive(self.rbImgStr,FALSE);
		gtk_widget_set_sensitive(self.rbImgTiled,FALSE);
#ifndef APPMGR_INTERFACE	
		gtk_widget_set_sensitive(self.rbSolid,FALSE);
		gtk_widget_set_sensitive(self.rbGrad,FALSE);
		gtk_widget_set_sensitive(self.rbImage,FALSE);
#endif		
	}
	else
	{
#ifndef APPMGR_INTERFACE	
		gtk_widget_set_sensitive(self.rbSolid,TRUE);
		gtk_widget_set_sensitive(self.rbGrad,TRUE);
		gtk_widget_set_sensitive(self.rbImage,TRUE);
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbSolid)))
		{
			gtk_widget_set_sensitive(self.bColor1,TRUE);
			gtk_widget_set_sensitive(self.bColor2,FALSE);
			gtk_widget_set_sensitive(self.eImage,FALSE);
			gtk_widget_set_sensitive(self.bOpen,FALSE);
			gtk_widget_set_sensitive(self.rbH,FALSE);
			gtk_widget_set_sensitive(self.rbV,FALSE);
			gtk_widget_set_sensitive(self.rbImgCent,FALSE);
			gtk_widget_set_sensitive(self.rbImgStr,FALSE);
			gtk_widget_set_sensitive(self.rbImgTiled,FALSE);
		}
		else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbGrad)))
		{
			gtk_widget_set_sensitive(self.bColor1,TRUE);
			gtk_widget_set_sensitive(self.bColor2,TRUE);
			gtk_widget_set_sensitive(self.eImage,FALSE);
			gtk_widget_set_sensitive(self.bOpen,FALSE);
			gtk_widget_set_sensitive(self.rbH,TRUE);
			gtk_widget_set_sensitive(self.rbV,TRUE);
			gtk_widget_set_sensitive(self.rbImgCent,FALSE);
			gtk_widget_set_sensitive(self.rbImgStr,FALSE);
			gtk_widget_set_sensitive(self.rbImgTiled,FALSE);
		}
		else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImage)))
#endif			
		{
#ifndef APPMGR_INTERFACE	
			gtk_widget_set_sensitive(self.bColor1,FALSE);
			gtk_widget_set_sensitive(self.bColor2,FALSE);
			gtk_widget_set_sensitive(self.rbH,FALSE);
			gtk_widget_set_sensitive(self.rbV,FALSE);
#endif			
			gtk_widget_set_sensitive(self.eImage,TRUE);
			gtk_widget_set_sensitive(self.bOpen,TRUE);
			gtk_widget_set_sensitive(self.rbImgCent,TRUE);
			gtk_widget_set_sensitive(self.rbImgStr,TRUE);
			gtk_widget_set_sensitive(self.rbImgTiled,TRUE);
		}
	}
}

GtkWidget *
build_colorbox ()
{
  guint gpe_border = gpe_get_border ();
  guint gpe_boxspacing = gpe_get_boxspacing ();
  gchar *tstr;
  int i;
  GtkWidget *label, *button;
  GtkAttachOptions table_attach_left_col_x;
  GtkAttachOptions table_attach_left_col_y;
  GtkAttachOptions table_attach_right_col_x;
  GtkAttachOptions table_attach_right_col_y;
  GtkJustification table_justify_left_col;
  GtkJustification table_justify_right_col;

  table_attach_left_col_x = GTK_FILL;
  table_attach_left_col_y = 0;
  table_attach_right_col_x = GTK_SHRINK | GTK_EXPAND | GTK_FILL;
  table_attach_right_col_y = GTK_FILL;

  table_justify_left_col = GTK_JUSTIFY_LEFT;
  table_justify_right_col = GTK_JUSTIFY_RIGHT;

  colorbox = gtk_table_new (3, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (colorbox), gpe_border);
  gtk_table_set_row_spacings (GTK_TABLE (colorbox), 0);
  gtk_table_set_col_spacings (GTK_TABLE (colorbox), gpe_boxspacing);
  gtk_widget_show (colorbox);

	button = gtk_button_new_with_label(" ");
	csel->previewbutton = button;
    gtk_widget_set_size_request(button,30,30+6*gpe_boxspacing);
    gtk_table_attach (GTK_TABLE (colorbox), button, 2, 3, 0, 3,
			(GtkAttachOptions) (table_attach_left_col_x),
			(GtkAttachOptions) (table_attach_left_col_y), 0, 0);
    gtk_widget_show(button);

  for (i = 0; i < 3; i++)
    {
      label = gtk_label_new (NULL);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_widget_show (label);
      switch (i)
	{
	case 0:
	  tstr = g_strdup_printf ("<b>%s</b>", _("Red"));
	  gtk_label_set_markup (GTK_LABEL (label), tstr);
	  g_free (tstr);
	  break;
	case 1:
	  tstr = g_strdup_printf ("<b>%s</b>", _("Green"));
	  gtk_label_set_markup (GTK_LABEL (label), tstr);
	  g_free (tstr);
	  break;
	case 2:
	  tstr = g_strdup_printf ("<b>%s</b>", _("Blue"));
	  gtk_label_set_markup (GTK_LABEL (label), tstr);
	  g_free (tstr);
	  break;
	}
      gtk_table_attach (GTK_TABLE (colorbox), label, 0, 1, i, i + 1,
			(GtkAttachOptions) (table_attach_left_col_x),
			(GtkAttachOptions) (table_attach_left_col_y), 0, 0);
      csel->a[i] = GTK_ADJUSTMENT(gtk_adjustment_new (0.0, 0.0, 255.0, 1.0, 1.0, 10.0));
      label = sp_color_slider_new (csel->a[i]);
      gtk_widget_show (label);
      csel->s[i] = (SPColorSlider*)label;
      g_signal_connect (G_OBJECT (csel->s[i]), "changed",
			  G_CALLBACK (sp_color_selector_update_sliders),
			  NULL);

      sp_color_slider_set_map ((SPColorSlider*)label, NULL);
      sp_color_slider_set_colors ((SPColorSlider*)label, 1.0, 65534.0);
      gtk_table_attach (GTK_TABLE (colorbox), label, 1, 2, i, i + 1,
			(GtkAttachOptions) (table_attach_left_col_x),
			(GtkAttachOptions) (table_attach_left_col_y), 0, 0);	    
    }
	
  return colorbox;
}

static void
notify_func (const char *name,
	     XSettingsAction action,
	     XSettingsSetting * setting, void *cb_data)
{
  GtkRcStyle *astyle;
  GtkWidget *label;
	
   if (strncmp (name, KEY_GTK, strlen (KEY_GTK)) == 0)
    {
      char *p = (char *) name + strlen (KEY_GTK);

 	  if (!strcmp (p, "FontName"))
	  {
	  if (setting->type == XSETTINGS_TYPE_STRING)
	    {
           astyle = gtk_rc_style_new ();
           astyle->font_desc = pango_font_description_from_string (setting->data.v_string);
           gtk_widget_modify_style (self.demolabel2, astyle);
		   gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.spFSApp),(float)pango_font_description_get_size(astyle->font_desc)/PANGO_SCALE);
		   label = g_object_get_data (G_OBJECT (self.bFontApp), "label");
		   gtk_label_set_text(GTK_LABEL(label),pango_font_description_get_family(astyle->font_desc));	
		}
	  }
    }
   if (strncmp (name, KEY_GTK, strlen (KEY_GTK)) == 0)
    {
      char *p = (char *) name + strlen (KEY_GTK);
#ifndef APPMGR_INTERFACE	
 	  if (!strcmp (p, "ToolbarIconSize"))
	  {
	  if (setting->type == XSETTINGS_TYPE_INT)
	    {
			gtk_range_set_value(GTK_RANGE(self.slToolbarSize),(float)setting->data.v_int);
		}
	  }
#endif	  
 	  if (!strcmp (p, "ToolbarStyle"))
	  {
	  if (setting->type == XSETTINGS_TYPE_INT)
	    {
			switch (setting->data.v_int)
			{
				case GTK_TOOLBAR_TEXT:
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbToolText),TRUE);
				break;
				case GTK_TOOLBAR_BOTH:
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbToolBoth),TRUE);
				break;
				case GTK_TOOLBAR_BOTH_HORIZ:
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbToolBothH),TRUE);
				break;
				default:
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbToolIcons),TRUE);
				break;
			}
		}
	  }
    }
   if (strncmp (name, KEY_THEME, strlen (KEY_THEME)) == 0)
    {
      char *p = (char *) name + strlen (KEY_THEME);

      if (!strcmp (p, "ThemeName"))
	{
	  if (setting->type == XSETTINGS_TYPE_STRING)
	    {
	      if (self.themename == NULL) self.themename = g_strdup (setting->data.v_string);
	      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (self.cbTheme)->entry),
				  setting->data.v_string);
	    }
	}
    }
  if (strncmp (name, KEY_MATCHBOX, strlen (KEY_MATCHBOX)) == 0)
    {
      char *p = (char *) name + strlen (KEY_MATCHBOX);

      if (!strcmp (p, "Background"))
	{
	  if (setting->type == XSETTINGS_TYPE_STRING)
	    {
			if (strlen(setting->data.v_string) > 1) /* There is something defined */
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.cDefault),FALSE);
				
			mbbg_parse_spec(&mb_back,g_strdup(setting->data.v_string));
			  switch(mb_back.type)
				{
#ifndef APPMGR_INTERFACE	
				case BG_SOLID:
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbSolid),TRUE);
				  widget_set_color_rgb8(self.bColor1,(float)mb_back.data.cols[0]/255.0,(float)mb_back.data.cols[1]/255.0,(float)mb_back.data.cols[2]/255.0);
				  break;
#endif				
				case BG_TILED_PXM:
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbImgTiled),TRUE);
#ifndef APPMGR_INTERFACE	
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbImage),TRUE);
#endif				
				  gtk_entry_set_text(GTK_ENTRY(self.eImage),mb_back.data.filename);
				  break;
   				case BG_CENTERED_PXM:
 				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbImgCent),TRUE);
#ifndef APPMGR_INTERFACE	
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbImage),TRUE);
#endif				
				  gtk_entry_set_text(GTK_ENTRY(self.eImage),mb_back.data.filename);
				  break;
				case BG_STRETCHED_PXM:
 				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbImgStr),TRUE);
#ifndef APPMGR_INTERFACE	
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbImage),TRUE);
#endif				
				  gtk_entry_set_text(GTK_ENTRY(self.eImage),mb_back.data.filename);
				  break;
#ifndef APPMGR_INTERFACE	
				case BG_GRADIENT_HORIZ:
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbH),TRUE);
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbGrad),TRUE);
				  widget_set_color_rgb8(self.bColor1,(float)mb_back.data.cols[0]/255.0,(float)mb_back.data.cols[2]/255.0,(float)mb_back.data.cols[4]/255.0);
				  widget_set_color_rgb8(self.bColor2,(float)mb_back.data.cols[1]/255.0,(float)mb_back.data.cols[3]/255.0,(float)mb_back.data.cols[5]/255.0);
				  break;
				case BG_GRADIENT_VERT:
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbV),TRUE);
				  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbGrad),TRUE);
				  widget_set_color_rgb8(self.bColor1,(float)mb_back.data.cols[0]/255.0,(float)mb_back.data.cols[2]/255.0,(float)mb_back.data.cols[4]/255.0);
				  widget_set_color_rgb8(self.bColor2,(float)mb_back.data.cols[1]/255.0,(float)mb_back.data.cols[3]/255.0,(float)mb_back.data.cols[5]/255.0);
				  break;
#endif				
				}    
			}
 	    update_enabled_widgets();			
	}
      
	
#ifndef APPMGR_INTERFACE	
	if (!strcmp (p, "Desktop/IconSize"))
	{
	  if (setting->type == XSETTINGS_TYPE_INT)
	    {
			gtk_range_set_value(GTK_RANGE(self.slIconSize),(float)setting->data.v_int);
		}
	}
	
	if (!strcmp (p, "Desktop/Font"))
	{
	  if (setting->type == XSETTINGS_TYPE_STRING)
	    {
		   char* spos = NULL;
			
           astyle = gtk_rc_style_new ();
		   spos = strrchr(setting->data.v_string,'-'); // this should be the size divider
		   if ((spos != NULL) && strlen(spos) && isdigit(spos[1]))
			  spos[0] = ' ';		   
           astyle->font_desc = pango_font_description_from_string (setting->data.v_string);
           gtk_widget_modify_style (self.demolabel, astyle);
		   gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.spFS),(float)pango_font_description_get_size(astyle->font_desc)/PANGO_SCALE);
		   label = g_object_get_data (G_OBJECT (self.bFont), "label");
		   gtk_label_set_text(GTK_LABEL(label),pango_font_description_get_family(astyle->font_desc));	
		}
	}
	
	if (!strcmp (p, "Desktop/FontColor"))
	{
	  if (setting->type == XSETTINGS_TYPE_STRING)
	    {
           Display *dpy;
           Colormap colormap;
           XColor tmpxcol;
			
           dpy = GDK_DISPLAY();
           colormap = gdk_x11_colormap_get_xcolormap(gdk_screen_get_default_colormap(gdk_screen_get_default()));
		   XParseColor(dpy, colormap,setting->data.v_string, &tmpxcol);
           widget_set_color_rgb8(self.bColorFont,(float)(tmpxcol.red >> 8)/255.0,(float)(tmpxcol.green >> 8)/255.0,(float)(tmpxcol.blue >> 8)/255.0);
		}
	}
	
	if (!strcmp (p, "COMPOSITE"))
	{
	  if (setting->type == XSETTINGS_TYPE_STRING)
	    {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.cPerformance),
				!strcmp(setting->data.v_string,"on"));
		}
	}
#endif	
   }
 }

static GdkFilterReturn
xsettings_event_filter (GdkXEvent * xevp, GdkEvent * ev, gpointer p)
{
  if (xsettings_client_process_event (client, (XEvent *) xevp))
    return GDK_FILTER_REMOVE;

  return GDK_FILTER_CONTINUE;
}


static void
watch_func (Window window, Bool is_start, long mask, void *cb_data)
{
  GdkWindow *gdkwin;

  gdkwin = gdk_window_lookup (window);

  if (is_start)
    {
      if (!gdkwin)
	gdkwin = gdk_window_foreign_new (window);
      else
	g_object_ref (gdkwin);

      gdk_window_add_filter (gdkwin, xsettings_event_filter, NULL);
    }
  else
    {
      g_assert (gdkwin);
      g_object_unref (gdkwin);
      gdk_window_remove_filter (gdkwin, xsettings_event_filter, NULL);
    }
}

gboolean
mb_start_xsettings (void)
{
  Display *dpy = GDK_DISPLAY ();

  client =
    xsettings_client_new (dpy, DefaultScreen (dpy), notify_func, watch_func,
			  NULL);
  if (client == NULL)
    {
      fprintf (stderr, "Cannot create XSettings client.\n");
      return FALSE;
    }

  return TRUE;
}


/*******************/
/*  Changing stuff */
/*******************/

void
on_matchbox_entry_changed (GtkWidget * menu, gpointer user_data)
{
  gchar *tn = NULL;
  tn =
    gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (self.cbTheme)->entry), 0, 
	                        -1);
  if ((tn) && (strlen (tn)))
    {
	  if (use_xst)
		system_printf (CMD_XST " write %s%s str %s", KEY_THEME, "ThemeName", tn);
    }
  if (tn)
    g_free (tn);
}

void
dialog_color_response (GtkDialog * dialog, gint response, gpointer caller)
{
  float r,g,b;
  if (response == GTK_RESPONSE_ACCEPT)
    {
      // grab color here
  	  r = csel->a[0]->value;
      g = csel->a[1]->value;
      b = csel->a[2]->value;
      widget_set_color_rgb8(caller,r,g,b);
    }

  gtk_container_remove (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), colorbox);
  gtk_widget_destroy (GTK_WIDGET (dialog));
}


void
on_color_select (GtkWidget * widget, GdkEvent * event)
{
  GtkWidget *w;
  GtkStyle *astyle = gtk_widget_get_style(widget);
	
  w = gtk_dialog_new_with_buttons (_("Select colour"), GTK_WINDOW (mainw),
				   GTK_DIALOG_MODAL |
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
				   GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				   NULL);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (w)->vbox), build_colorbox ());
	
  // pass calling widget to handler
  g_signal_connect (G_OBJECT (w), "response",
		    (void *) dialog_color_response, (gpointer) widget);

  gtk_adjustment_set_value(csel->a[0],(float)astyle->base[GTK_STATE_NORMAL].red/65534.0);
  gtk_adjustment_set_value(csel->a[1],(float)astyle->base[GTK_STATE_NORMAL].green/65534.0);
  gtk_adjustment_set_value(csel->a[2],(float)astyle->base[GTK_STATE_NORMAL].blue/65534.0);
  g_signal_emit_by_name (G_OBJECT (csel->s[0]), "changed");

  gtk_dialog_run (GTK_DIALOG (w));
}


static void
File_Selected (char *file, gpointer data)
{
  if (access(file,R_OK))
	gpe_error_box(_("You don't have read access to selected background image!"));
  else
	gtk_entry_set_text (GTK_ENTRY (self.eImage), file);
}

void
choose_file (GtkWidget * button, gpointer user_data)
{
  ask_user_a_file (g_get_home_dir(), NULL, File_Selected, NULL, NULL);
}

char* get_color_from_widget(GtkWidget* w)
{
  GdkColor acolor;
  GtkStyle* astyle;
  int r,g,b;
  gchar *result;
	
  astyle = gtk_widget_get_style (w);
  acolor = astyle->base[GTK_STATE_NORMAL];
  r = acolor.red >> 8;
  g = acolor.green >> 8;
  b = acolor.blue >> 8;
  result = g_strdup_printf("#%02x%02x%02x",r,g,b);
  return result;
}


void
Theme_Save (void)
{
	char *confstr = NULL;
	const char* clabel;
	char *par1,*par2;
	int fs;
	GtkWidget* label;
	
    /* background settings */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.cDefault)))
	{
		confstr = NULL;
	}
	else
	{	
#ifndef APPMGR_INTERFACE	
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbSolid)))
		{
			par1 = get_color_from_widget(self.bColor1);
			confstr = g_strdup_printf("col-solid:%s",par1);
			free(par1);
		}
		else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbGrad)))
		{
			par1 = get_color_from_widget(self.bColor1);
			par2 = get_color_from_widget(self.bColor2);
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbV)))
				confstr = g_strdup_printf("col-gradient-vertical:%s,%s",par1,par2);
			else
				confstr = g_strdup_printf("col-gradient-horizontal:%s,%s",par1,par2);
			free(par1);
			free(par2);		
		}
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImage)))
#endif			
		{
			par1 = gtk_editable_get_chars(GTK_EDITABLE(self.eImage),0,-1);
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImgCent)))
				confstr = g_strdup_printf("img-centered:%s",par1);
			else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImgStr)))
				confstr = g_strdup_printf("img-stretched:%s",par1);
			else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbImgTiled)))
				confstr = g_strdup_printf("img-tiled:%s",par1);
			if (access(par1,R_OK))
			{
				gpe_error_box(_("You don't have read access to selected background image!"));
				free(confstr);
				confstr = NULL;
			}
			free(par1);
		}
	
	}
	
	/* background style*/  	
	if (confstr != NULL)
	{
		gchar *p = g_strdup_printf (CMD_XST " write %s%s str '%s'", KEY_MATCHBOX, "Background", confstr);
		system(p);
		g_free(p);
		free(confstr);
	}
	else /* theme default - erase settings */
	{
		system_printf (CMD_XST " delete %s%s", KEY_MATCHBOX, "Background");
	}
	
#ifndef APPMGR_INTERFACE	
	/* composite/performance setting */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.cPerformance)))
		system_printf (CMD_XST " write %s%s str %s", KEY_MATCHBOX, "COMPOSITE", "on");
	else
		system_printf (CMD_XST " write %s%s str %s", KEY_MATCHBOX, "COMPOSITE", "off");
	
	/* desktop font type and size */
	label = g_object_get_data (G_OBJECT (self.bFont), "label");
	clabel = gtk_label_get_text(GTK_LABEL(label));
	fs = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self.spFS));
	confstr = g_strdup_printf(CMD_XST " write %s%s str \"%s-%i\"", KEY_MATCHBOX, "Desktop/Font", clabel,fs);
	system(confstr);
	free(confstr);
	
	/* desktop font colour */
	confstr = get_color_from_widget(self.bColorFont);
	system_printf (CMD_XST " write %s%s str %s", KEY_MATCHBOX, "Desktop/FontColor", confstr);
	free(confstr);
#endif	
	/* application font type and size */
	label = g_object_get_data (G_OBJECT (self.bFontApp), "label");
	clabel = gtk_label_get_text(GTK_LABEL(label));
	fs = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self.spFSApp));
	confstr = g_strdup_printf(CMD_XST " write %s%s str \"%s %i\"", KEY_GTK, "FontName", clabel,fs);
	system(confstr);
	free(confstr);	

#ifndef APPMGR_INTERFACE	
	/* desktop icon size */
	fs = (int)gtk_range_get_value(GTK_RANGE(self.slIconSize));
	system_printf (CMD_XST " write %s%s int %d", KEY_MATCHBOX, "Desktop/IconSize", fs);
	
	/* toolbar icon size */
	fs = (int)gtk_range_get_value(GTK_RANGE(self.slToolbarSize));
	system_printf (CMD_XST " write %s%s int %d", KEY_GTK, "ToolbarIconSize", fs);
#endif	
	/* toolbar style */
	fs = GTK_TOOLBAR_ICONS;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbToolText)))
		fs = GTK_TOOLBAR_TEXT;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbToolBoth)))
		fs = GTK_TOOLBAR_BOTH;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbToolBothH)))
		fs = GTK_TOOLBAR_BOTH_HORIZ;
	system_printf (CMD_XST " write %s%s int %d", KEY_GTK, "ToolbarStyle", fs);
	
	/* terminal */
	label = g_object_get_data (G_OBJECT (self.bFontTerminal), "label");
	clabel = gtk_label_get_text(GTK_LABEL(label));
	set_terminal_font(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self.spFSTerminal)), clabel);
}

void
Theme_Restore ()
{
	system_printf (CMD_XST " write %s%s str %s", KEY_THEME, "ThemeName", self.themename);
}

/****************/
/*  interface   */
/****************/

GtkWidget *
Theme_Build_Objects ()
{
#ifndef APPMGR_INTERFACE	
  GtkWidget *rg_background, *rg_hv;
#endif	
  GtkWidget *table, *rg_img;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *notebook;
  GtkAttachOptions table_attach_left_col_x;
  GtkAttachOptions table_attach_left_col_y;
  GtkAttachOptions table_attach_right_col_x;
  GtkAttachOptions table_attach_right_col_y;
  GtkJustification table_justify_left_col;
  GtkJustification table_justify_right_col;
  guint gpe_border = gpe_get_border ();
  guint gpe_boxspacing = gpe_get_boxspacing ();
  gchar *tstr, *desc; 
  GtkRcStyle* astyle;
  int isize, minsize;

  table_attach_left_col_x = GTK_FILL;
  table_attach_left_col_y = 0;
  table_attach_right_col_x = GTK_SHRINK | GTK_EXPAND | GTK_FILL;
  table_attach_right_col_y = GTK_FILL;

  table_justify_left_col = GTK_JUSTIFY_LEFT;
  table_justify_right_col = GTK_JUSTIFY_RIGHT;

  csel = malloc (sizeof (tcsel));
  self.themename = NULL;
  
  init_tools();
  
/* ------------------------------------------------------------------------ */

  notebook  = gtk_notebook_new();
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
  
#ifndef APPMGR_INTERFACE	
  label = gtk_label_new (_("Theme"));
#else
  label = gtk_label_new (_("Look and Feel"));
#endif  
  table = gtk_table_new (3, 2, FALSE);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);
  gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
  gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
  gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);

  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  tstr = g_strdup_printf ("<b>%s</b>", _("Desktop Theme"));
  gtk_label_set_markup (GTK_LABEL (label), tstr);
  g_free (tstr);
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  
  hbox = gtk_hbox_new (FALSE, gpe_boxspacing);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  
  label = gtk_label_new (_("Name"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);

  self.cbTheme = gtk_combo_new ();
  themeitems = make_items_from_dir (matchboxpath,"matchbox");
  gtk_combo_set_popdown_strings (GTK_COMBO (self.cbTheme), themeitems);
  gtk_combo_set_value_in_list (GTK_COMBO (self.cbTheme), FALSE, FALSE);
  gtk_editable_set_editable (GTK_EDITABLE(GTK_COMBO(self.cbTheme)->entry), FALSE);
  gtk_widget_set_size_request (self.cbTheme, 120, -1);
  gtk_box_pack_start (GTK_BOX (hbox), self.cbTheme, FALSE, TRUE, 0);
			  
#ifndef APPMGR_INTERFACE	
  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.1);
  tstr = g_strdup_printf ("<b>%s</b>", _("Icon Size"));
  gtk_label_set_markup (GTK_LABEL (label), tstr);
  g_free (tstr);
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 2, 3,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  
  self.slIconSize = gtk_hscale_new_with_range(16.0,64.0,2.0);
  gtk_table_attach (GTK_TABLE (table), self.slIconSize, 0, 4, 3, 4,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_range_set_increments(GTK_RANGE(self.slIconSize),2.0,4.0);
  
  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.1);
  tstr = g_strdup_printf ("<b>%s</b>", _("Visual Effects"));
  gtk_label_set_markup (GTK_LABEL (label), tstr);
  g_free (tstr);
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 4, 5,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  self.cPerformance = gtk_check_button_new_with_label(_("Enable Visual Effects"));
  gtk_table_attach (GTK_TABLE (table), self.cPerformance, 0, 2, 5, 6,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

 /* Background tab */

  label = gtk_label_new (_("Background"));
  table = gtk_table_new (3, 2, FALSE);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,label);

  gtk_widget_set_name (table, "table");
  gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
  gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
  gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);
#else
#endif

  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
#ifndef APPMGR_INTERFACE	
  tstr = g_strdup_printf ("<b>%s</b>", _("Background Settings"));
#else
  tstr = g_strdup_printf ("\n<b>%s</b>", _("Background Settings"));
#endif
  gtk_label_set_markup (GTK_LABEL (label), tstr);
  g_free (tstr);
  
#ifndef APPMGR_INTERFACE	
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  self.cDefault = gtk_check_button_new_with_label(_("Theme Default"));
  gtk_table_attach (GTK_TABLE (table), self.cDefault, 0, 2, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  g_signal_connect (G_OBJECT (self.cDefault), "toggled",
		       G_CALLBACK(update_enabled_widgets), NULL);
#else
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 2, 3,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  self.cDefault = gtk_check_button_new_with_label(_("Theme Default"));
  gtk_table_attach (GTK_TABLE (table), self.cDefault, 0, 2, 3, 4,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  g_signal_connect (G_OBJECT (self.cDefault), "toggled",
		       G_CALLBACK(update_enabled_widgets), NULL);
#endif
#ifndef APPMGR_INTERFACE
  label = gtk_radio_button_new_with_label (NULL, _("Solid colour"));
  rg_background = label;
  self.rbSolid = label;
  g_signal_connect (G_OBJECT (label), "toggled",
		       G_CALLBACK(update_enabled_widgets), NULL);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
			
  // box for radio buttons
  hbox = gtk_hbox_new(FALSE,gpe_boxspacing);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 3, 6, 7,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  label = gtk_radio_button_new_with_label (NULL, _("Horizontal"));
  rg_hv = label;
  self.rbH = label;
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
  label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(rg_hv), _("Vertical"));
  self.rbV = label;
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
  
  self.bColor1 = gtk_button_new();
  gtk_button_set_label(GTK_BUTTON(self.bColor1),_("Colour"));
  g_signal_connect (G_OBJECT (self.bColor1), "clicked",
		    G_CALLBACK (on_color_select), NULL);
  gtk_table_attach (GTK_TABLE (table), self.bColor1, 1, 2, 2, 3,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  label =
    gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
						 (rg_background),
						 _("Colour gradient"));
  g_signal_connect (G_OBJECT (label), "toggled",
		       G_CALLBACK(update_enabled_widgets), NULL);
  self.rbGrad = label;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  self.bColor2 = gtk_button_new();
  gtk_button_set_label(GTK_BUTTON(self.bColor2),_("Colour"));
  g_signal_connect (G_OBJECT (self.bColor2), "clicked",
		    G_CALLBACK (on_color_select), NULL);
			
  gtk_table_attach (GTK_TABLE (table), self.bColor2, 1, 2, 3, 4,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  label =
    gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
						 (rg_background), _("Image"));
  g_signal_connect (G_OBJECT (label), "toggled",
		       (update_enabled_widgets), NULL);
  self.rbImage = label;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 7, 8,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
#endif
  self.eImage = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), self.eImage, 0, 1, 8, 9,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_widget_set_size_request (self.eImage, 150, -1);

  self.bOpen = gtk_button_new_from_stock (GTK_STOCK_OPEN);
  g_signal_connect (G_OBJECT (self.bOpen), "pressed",
		      G_CALLBACK (choose_file), NULL);
  gtk_table_attach (GTK_TABLE (table), self.bOpen, 1, 2, 8, 9,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  hbox = gtk_hbox_new(False,gpe_boxspacing);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 3, 9, 10,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
			
  label = gtk_radio_button_new_with_label (NULL, _("Centered"));
  rg_img = label;
  self.rbImgCent = label;
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
  label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(rg_img), _("Tiled"));
  self.rbImgTiled = label;
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
  label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(rg_img), _("Stretched"));
  self.rbImgStr = label;
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
			
  /* toolbar tab */

#ifndef APPMGR_INTERFACE
  label = gtk_label_new (_("Toolbars"));
  table = gtk_table_new (3, 2, FALSE);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,label);

  gtk_widget_set_name (table, "table");
  gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
  gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
  gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);
#endif
  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
#ifndef APPMGR_INTERFACE
  tstr = g_strdup_printf ("<b>%s</b>", _("Toolbar Style"));
#else
  tstr = g_strdup_printf ("\n<b>%s</b>", _("Toolbar Style"));
#endif
  gtk_label_set_markup (GTK_LABEL (label), tstr);
  g_free (tstr);
#ifndef APPMGR_INTERFACE
  gtk_table_attach (GTK_TABLE (table), label, 0, 3, 0, 1,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  self.rbToolIcons =  gtk_radio_button_new_with_label (NULL, _("Icons"));
  gtk_table_attach (GTK_TABLE (table), self.rbToolIcons, 0, 1, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  self.rbToolText =  gtk_radio_button_new_with_label_from_widget 
  						(GTK_RADIO_BUTTON(self.rbToolIcons), _("Text"));
  gtk_table_attach (GTK_TABLE (table), self.rbToolText, 1, 2, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  self.rbToolBoth =  gtk_radio_button_new_with_label_from_widget 
  						(GTK_RADIO_BUTTON(self.rbToolIcons), _("Both"));
  gtk_table_attach (GTK_TABLE (table), self.rbToolBoth, 2, 3, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  self.rbToolBothH =  gtk_radio_button_new_with_label_from_widget 
  						(GTK_RADIO_BUTTON(self.rbToolIcons), _("Both H"));
  gtk_table_attach (GTK_TABLE (table), self.rbToolBothH, 3, 4, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
#else
  gtk_table_attach (GTK_TABLE (table), label, 0, 3, 10, 11,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  hbox = gtk_hbox_new(FALSE, gpe_get_boxspacing());
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 4, 11, 12,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  
  self.rbToolIcons =  gtk_radio_button_new_with_label (NULL, _("Icons"));
  gtk_box_pack_start(GTK_BOX(hbox), self.rbToolIcons, FALSE, TRUE, 0);
  
  self.rbToolText =  gtk_radio_button_new_with_label_from_widget 
  						(GTK_RADIO_BUTTON(self.rbToolIcons), _("Text"));
  gtk_box_pack_start(GTK_BOX(hbox), self.rbToolText, FALSE, TRUE, 0);
  self.rbToolBoth =  gtk_radio_button_new_with_label_from_widget 
  						(GTK_RADIO_BUTTON(self.rbToolIcons), _("Both (icon above text)"));
  gtk_box_pack_start(GTK_BOX(hbox), self.rbToolBoth, FALSE, TRUE, 0);
  self.rbToolBothH =  gtk_radio_button_new_with_label_from_widget 
  						(GTK_RADIO_BUTTON(self.rbToolIcons), _("Both (icon left of text)"));
  gtk_box_pack_start(GTK_BOX(hbox), self.rbToolBothH, FALSE, TRUE, 0);
#endif

#ifndef APPMGR_INTERFACE	
  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.1);
  tstr = g_strdup_printf ("<b>%s</b>", _("Toolbar Icon Size"));
  gtk_label_set_markup (GTK_LABEL (label), tstr);
  g_free (tstr);
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 2, 3,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  
  self.slToolbarSize = gtk_hscale_new_with_range(1.0,3.0,1.0);
  gtk_table_attach (GTK_TABLE (table), self.slToolbarSize, 0, 3, 3, 4,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_range_set_value(GTK_RANGE(self.slToolbarSize),2.0);  
  gtk_range_set_increments(GTK_RANGE(self.slToolbarSize),1.0,1.0);
#endif			
 /* fonts tab */
#ifndef APPMGR_INTERFACE	
  label = gtk_label_new (_("Fonts"));
  table = gtk_table_new (2, 2, FALSE);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,label);
  gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
  gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
  gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);

  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  tstr = g_strdup_printf ("<b>%s</b>", _("Desktop Font"));
  gtk_label_set_markup (GTK_LABEL (label), tstr);
  g_free (tstr);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  label = gtk_label_new(_("Family"));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);

  self.bFont = GTK_WIDGET(popup_menu_button_new (GTK_STOCK_SELECT_FONT));
  g_object_set_data (G_OBJECT (self.bFont), "active", FALSE);
  g_signal_connect (G_OBJECT (self.bFont), "pressed",
		      G_CALLBACK (select_font_popup), NULL);
  gtk_table_attach (GTK_TABLE (table), self.bFont, 0, 1, 2, 3,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  self.demolabel = gtk_label_new(_("Desktop Font"));  
  gtk_table_attach (GTK_TABLE (table), self.demolabel, 0, 4, 3, 4,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  gtk_widget_set_size_request(self.demolabel,-1,30);
  gtk_misc_set_alignment(GTK_MISC(self.demolabel),0.5,0.2);
  
  label = gtk_label_new(_("Size"));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  self.spFS = gtk_spin_button_new_with_range(3,20,1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.spFS),7.0);
  gtk_table_attach (GTK_TABLE (table), self.spFS, 1, 2, 2, 3,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  g_signal_connect(G_OBJECT(self.spFS),"value-changed",G_CALLBACK(on_font_size_change),NULL);	
  
  label = gtk_label_new(_("Colour"));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  self.bColorFont = gtk_button_new();
  gtk_button_set_label(GTK_BUTTON(self.bColorFont),_("Colour"));
  g_signal_connect (G_OBJECT (self.bColorFont), "clicked",
		    G_CALLBACK (on_color_select), NULL);
  gtk_table_attach (GTK_TABLE (table), self.bColorFont, 3, 4, 2, 3,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
#endif
/*---------------------------------------------*/
  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
#ifndef APPMGR_INTERFACE	
  tstr = g_strdup_printf ("<b>%s</b>", _("Application Font"));
#else  
  tstr = g_strdup_printf ("\n<b>%s</b>", _("Application Font"));
#endif
  gtk_label_set_markup (GTK_LABEL (label), tstr);
  g_free (tstr);
#ifndef APPMGR_INTERFACE	
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  label = gtk_label_new(_("Family"));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);

  self.bFontApp = GTK_WIDGET(popup_menu_button_new (GTK_STOCK_SELECT_FONT));
  g_object_set_data (G_OBJECT (self.bFontApp), "active", FALSE);
  g_signal_connect (G_OBJECT (self.bFontApp), "pressed",
		      G_CALLBACK (select_font_popup), NULL);
  gtk_table_attach (GTK_TABLE (table), self.bFontApp, 0, 1, 6, 7,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  self.demolabel2 = gtk_label_new(_("Application Font"));  
  gtk_table_attach (GTK_TABLE (table), self.demolabel2, 0, 4, 7, 8,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  gtk_widget_set_size_request(self.demolabel2,-1,30);
  gtk_misc_set_alignment(GTK_MISC(self.demolabel2),0.5,0.2);

  label = gtk_label_new(_("Size"));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 5, 6,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  self.spFSApp = gtk_spin_button_new_with_range(3,20,1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.spFSApp),8.0);
  gtk_table_attach (GTK_TABLE (table), self.spFSApp, 1, 2, 6, 7,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
#else
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 12, 13,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  label = gtk_label_new(_("Family"));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 13, 14,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);

  self.bFontApp = GTK_WIDGET(popup_menu_button_new (GTK_STOCK_SELECT_FONT));
  g_object_set_data (G_OBJECT (self.bFontApp), "active", FALSE);
  g_signal_connect (G_OBJECT (self.bFontApp), "pressed",
		      G_CALLBACK (select_font_popup), NULL);
  gtk_table_attach (GTK_TABLE (table), self.bFontApp, 0, 1, 14, 15,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  self.demolabel2 = gtk_label_new(_("Application Font"));  
  gtk_table_attach (GTK_TABLE (table), self.demolabel2, 0, 4, 15, 16,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  gtk_widget_set_size_request(self.demolabel2,-1,30);
  gtk_misc_set_alignment(GTK_MISC(self.demolabel2),0.5,0.2);

  label = gtk_label_new(_("Size"));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 13, 14,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  self.spFSApp = gtk_spin_button_new_with_range(3,16,1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.spFSApp),8.0);
  gtk_table_attach (GTK_TABLE (table), self.spFSApp, 1, 2, 14, 15,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
#endif
  g_signal_connect(G_OBJECT(self.spFSApp),"value-changed",G_CALLBACK(on_font_size_change),NULL);	

/* terminal tab */
  label = gtk_label_new (_("Terminal"));
  table = gtk_table_new (2, 2, FALSE);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);
  gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
  gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
  gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);

  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  tstr = g_strdup_printf ("<b>%s</b>", _("Font"));
  gtk_label_set_markup (GTK_LABEL (label), tstr);
  g_free (tstr);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);

  label = gtk_label_new(_("Family"));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);

  self.bFontTerminal = GTK_WIDGET(popup_menu_button_new (GTK_STOCK_SELECT_FONT));
  g_object_set_data (G_OBJECT (self.bFontTerminal), "active", FALSE);
  g_signal_connect (G_OBJECT (self.bFontTerminal), "pressed",
		      G_CALLBACK (select_font_popup), NULL);
  gtk_table_attach (GTK_TABLE (table), self.bFontTerminal, 0, 3, 2, 3,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  self.demolabel3 = gtk_label_new(_("Terminal Font"));  
  gtk_table_attach (GTK_TABLE (table), self.demolabel3, 0, 4, 4, 5,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  gtk_widget_set_size_request(self.demolabel3, -1, 30);
  gtk_misc_set_alignment(GTK_MISC(self.demolabel3), 0.5, 0.2);

  label = gtk_label_new(_("Size"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  self.spFSTerminal = gtk_spin_button_new_with_range(3, 16, 1);
  gtk_table_attach (GTK_TABLE (table), self.spFSTerminal, 1, 2, 3, 4,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), gpe_boxspacing, 0);
  g_signal_connect(G_OBJECT(self.spFSTerminal), "value-changed",
                   G_CALLBACK(on_font_size_change), NULL);	
			
/* insert some defaults */
  astyle = gtk_rc_style_new ();
  astyle->font_desc = pango_font_description_from_string ("Sans 9");
  gtk_widget_modify_style (self.demolabel2, astyle);
#ifndef APPMGR_INTERFACE	
  gtk_widget_modify_style (self.demolabel, astyle);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.spFS),(float)9.0);
  label = g_object_get_data (G_OBJECT (self.bFont), "label");
  gtk_label_set_text(GTK_LABEL(label),"Sans");	
#endif
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.spFSApp),(float)8.0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.cDefault),TRUE);
  label = g_object_get_data (G_OBJECT (self.bFontApp), "label");
  gtk_label_set_text(GTK_LABEL(label),"Sans");	
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.cDefault),TRUE);
   
  /* icon sizes */
  minsize = (gdk_screen_height() < gdk_screen_width()) 
            ? gdk_screen_height() : gdk_screen_width();
  if (minsize < 200) 
	 isize = 24;
  else if (minsize < 300)
	 isize = 32;
  else 
	 isize = 48;
#ifndef APPMGR_INTERFACE	
  gtk_range_set_value(GTK_RANGE(self.slIconSize),(float)isize);
#endif 
  /* toolbar layout and font size */
  if (minsize > 400)
  {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbToolBoth),TRUE);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.spFSApp),9.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.spFS),8.0);
  }
  else
  {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbToolIcons),TRUE);
  }
  
  /* terminal font type and size */
  isize = get_terminal_fontsize();
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.spFSTerminal), isize);
  label = g_object_get_data (G_OBJECT (self.bFontTerminal), "label");
  tstr = get_terminal_fontname();
  gtk_label_set_text(GTK_LABEL(label), tstr);
  
  desc = g_strdup_printf("%s %d", tstr, isize);
  astyle->font_desc = pango_font_description_from_string (desc);
  gtk_widget_modify_style (self.demolabel3, astyle);
  g_free(tstr);
  g_free(desc);
  
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.cPerformance), FALSE);
  mb_start_xsettings ();
  
  g_signal_connect (G_OBJECT (GTK_COMBO (self.cbTheme)->entry), "changed",
		      G_CALLBACK (on_matchbox_entry_changed), NULL);

  return notebook;
}


//------font stuff------------from gpe-word
static GtkWidget *
popup_menu_button_new (const gchar *stock_id)
{
  GtkWidget *button, *arrow, *hbox, *image, *label;
  GtkRequisition requisition;
  gint width = 0, height;

  button = gtk_button_new ();
  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
  hbox = gtk_hbox_new (FALSE, 0);
  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_SMALL_TOOLBAR);
  label = gtk_label_new(NULL);

  gtk_box_set_homogeneous (GTK_BOX (hbox), FALSE);

  g_object_set_data (G_OBJECT (button), "active", FALSE);
  g_object_set_data (G_OBJECT (button), "hbox", hbox);
  g_object_set_data (G_OBJECT (button), "image", image);
  g_object_set_data (G_OBJECT (button), "arrow", arrow);
  g_object_set_data (G_OBJECT (button), "label", label);

  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), arrow, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  gtk_widget_size_request (image, &requisition);
  width = width + requisition.width;
  height = requisition.height;

  gtk_widget_size_request (arrow, &requisition);
  width = (width + requisition.width) - 5;

  gtk_widget_set_size_request (hbox, width, height);
  gtk_container_add (GTK_CONTAINER (button), hbox);
  gtk_widget_show_all (button);

  return button;
}


#ifndef APPMGR_INTERFACE	
static void
on_font_select_desk(GtkWidget * widget, gpointer style)
{
	GtkWidget * label;
	gtk_widget_modify_style(self.demolabel,GTK_RC_STYLE(style));
	pango_font_description_set_size(GTK_RC_STYLE(style)->font_desc,gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self.spFS))*PANGO_SCALE);
	label = g_object_get_data(G_OBJECT(self.bFont),"label");
	gtk_label_set_text(GTK_LABEL(label),pango_font_description_get_family(GTK_RC_STYLE(style)->font_desc));	
	select_font_popup(self.bFont);
	on_font_size_change(GTK_SPIN_BUTTON(self.spFS),GTK_SCROLL_NONE);
}
#endif	

static void
on_font_select_app(GtkWidget * widget, gpointer style)
{
	GtkWidget * label;
	gtk_widget_modify_style(self.demolabel2,GTK_RC_STYLE(style));
	pango_font_description_set_size(GTK_RC_STYLE(style)->font_desc,gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self.spFSApp))*PANGO_SCALE);
	label = g_object_get_data(G_OBJECT(self.bFontApp),"label");
	gtk_label_set_text(GTK_LABEL(label),pango_font_description_get_family(GTK_RC_STYLE(style)->font_desc));	
	select_font_popup(self.bFontApp);
	on_font_size_change(GTK_SPIN_BUTTON(self.spFSApp),GTK_SCROLL_NONE);
}

static void
on_font_select_terminal(GtkWidget * widget, gpointer style)
{
	GtkWidget * label;
	gtk_widget_modify_style(self.demolabel3,GTK_RC_STYLE(style));
	pango_font_description_set_size(GTK_RC_STYLE(style)->font_desc,gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self.spFSTerminal))*PANGO_SCALE);
	label = g_object_get_data(G_OBJECT(self.bFontTerminal),"label");
	gtk_label_set_text(GTK_LABEL(label),pango_font_description_get_family(GTK_RC_STYLE(style)->font_desc));	
	select_font_popup(self.bFontTerminal);
	on_font_size_change(GTK_SPIN_BUTTON(self.spFSTerminal),GTK_SCROLL_NONE);
}

static void
on_font_size_change(GtkSpinButton *spinbutton,GtkScrollType arg1)
{
	GtkRcStyle *astyle = gtk_rc_style_new();
	
#ifndef APPMGR_INTERFACE	
	if (GTK_WIDGET(spinbutton) == self.spFS)
	{
		astyle->font_desc = gtk_widget_get_style(self.demolabel)->font_desc;
		pango_font_description_set_size(astyle->font_desc,gtk_spin_button_get_value_as_int(spinbutton)*PANGO_SCALE);
	    gtk_widget_modify_style(self.demolabel,GTK_RC_STYLE(astyle));
	}
	else 
#endif		
	if (GTK_WIDGET(spinbutton) == self.spFSApp) 	
	{
		astyle->font_desc = gtk_widget_get_style(self.demolabel2)->font_desc;
		pango_font_description_set_size(astyle->font_desc,gtk_spin_button_get_value_as_int(spinbutton)*PANGO_SCALE);
	    gtk_widget_modify_style(self.demolabel2,GTK_RC_STYLE(astyle));
	}
	if (GTK_WIDGET(spinbutton) == self.spFSTerminal) 	
	{
		astyle->font_desc = gtk_widget_get_style(self.demolabel3)->font_desc;
		pango_font_description_set_size(astyle->font_desc,gtk_spin_button_get_value_as_int(spinbutton)*PANGO_SCALE);
	    gtk_widget_modify_style(self.demolabel3,GTK_RC_STYLE(astyle));
	}

	g_free(astyle);
}


static int
select_font_popup_cmp_families (const void *a, const void *b)
{
  const char *a_name = pango_font_family_get_name (*(PangoFontFamily **)a);
  const char *b_name = pango_font_family_get_name (*(PangoFontFamily **)b);
  
  return g_utf8_collate (a_name, b_name);
}

static void
select_font_popup (GtkWidget *parent_button)
{
  GtkWidget *popup_window, *frame, *vbox, *button, *alignment, *scrolled_window, *button_label;
  GtkWidget *parent_arrow;
  GtkRequisition frame_requisition, parent_button_requisition;
  GtkRcStyle *button_label_rc_style;
  PangoFontFamily **families;
  gint n_families, i;
  gint x, y;
  gint screen_width;
  gint screen_height;

  parent_arrow = g_object_get_data (G_OBJECT (parent_button), "arrow");
  if ((gboolean)g_object_get_data (G_OBJECT (parent_button), "active") == TRUE)
  {
    gtk_widget_destroy (g_object_get_data (G_OBJECT (parent_button), "window"));
    g_object_set_data (G_OBJECT (parent_button), "active", FALSE);
    gtk_arrow_set (GTK_ARROW (parent_arrow), GTK_ARROW_DOWN, GTK_SHADOW_NONE);
  }
  else
  {
    g_object_set_data (G_OBJECT (parent_button), "active", (gpointer)TRUE);
    gtk_arrow_set (GTK_ARROW (parent_arrow), GTK_ARROW_UP, GTK_SHADOW_NONE);

    popup_window = gtk_window_new (GTK_WINDOW_POPUP);
    vbox = gtk_vbox_new (FALSE, 0);
    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    pango_context_list_families (gtk_widget_get_pango_context (GTK_WIDGET (parent_button)), &families, &n_families);
    qsort (families, n_families, sizeof (PangoFontFamily *), select_font_popup_cmp_families);

    for (i=0; i<n_families; i++)
    {
      const gchar *font_name = pango_font_family_get_name (families[i]);

      button_label = gtk_label_new (font_name);
      button_label_rc_style = gtk_rc_style_new ();
      button_label_rc_style->font_desc = pango_font_description_from_string (g_strdup_printf ("%s 9", font_name));
      gtk_widget_modify_style (button_label, button_label_rc_style);

      alignment = gtk_alignment_new (0, 0, 0, 0);
      button = gtk_button_new ();
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_container_add (GTK_CONTAINER (alignment), button_label);
      gtk_container_add (GTK_CONTAINER (button), alignment);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
#ifndef APPMGR_INTERFACE	
	  if (parent_button == self.bFont) g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_font_select_desk),button_label_rc_style);
	  	else	  
#endif		  
	  if (parent_button == self.bFontApp) g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_font_select_app),button_label_rc_style);
		else
          if (parent_button == self.bFontTerminal) g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_font_select_terminal),button_label_rc_style);
    }

    g_free (families);

    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), vbox);
    gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
    gtk_container_add (GTK_CONTAINER (popup_window), frame);
    gtk_widget_show_all (frame);

    gtk_widget_size_request (parent_button, &parent_button_requisition);
    gtk_widget_size_request (frame, &frame_requisition);

    gdk_window_get_position (GDK_WINDOW (mainw->window), &x, &y);

    screen_width = gdk_screen_width ();
    screen_height = gdk_screen_height ();
      
    x = CLAMP (x + parent_button->allocation.x, 0, MAX (0, screen_width - frame_requisition.width));
    y += parent_button->allocation.y;
    y += parent_button_requisition.height;

    gtk_widget_set_size_request (scrolled_window, -1, (screen_height - y) - 10);

    gtk_widget_set_uposition (popup_window, x, y);
      
    g_object_set_data (G_OBJECT (parent_button), "window", popup_window);

    gtk_widget_show (popup_window);
  }
}
