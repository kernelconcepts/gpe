/*
 * gpe-mail
 *
 * by Erez Doron <erez@savan.com>
 */

#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include "gtkcolombo.h"

#define VERSION "0.0.4"

struct {
   char *path;
   char expanded_path[100];
   char conf[3][10][100];
} config={
   "~/.emailsync/imap",
   "",
   {
      {
         "root","secret","localhost","auto","\2","\1","5000","7",""
      },
      {
         "root@localhost","smtp","auto","\2","","","","","",""
      },
      {
         "","","","","","","","","",""
      }
   }
};

struct {
   gchar *titles[4];
   gchar *opts[3][10];
} options={
   {"Incoming","Outgoing","Misc",NULL},
   {
      {"Username","Password","Server","Port","Use pop3 instead of imap","SSL","Size (bytes)","Since (days), imap only ",NULL,NULL},
      {"Email Adress","Smtp Server","Port","SSL",NULL,NULL,NULL,NULL,NULL,NULL},
      {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}
   }
};

struct {
   GtkWidget *w;
   GtkWidget *b0;
   GtkWidget *hb2;
   GtkWidget *sw2;
   GtkWidget *hb3;
   GtkWidget *vb3;
   GtkWidget *b2;
   GtkWidget *folderoption;
   GtkWidget *foldermenu;
   GtkWidget *cl1;
   GtkWidget *mt;
   GtkWidget *mtsb;
   GtkTooltips *tt;

   GtkWidget *configb;
   GtkWidget *syncb;
   GtkWidget *deleteb;
   GtkWidget *composeb;
   GtkWidget *forwardb;
   GtkWidget *replyb;

   struct {
       GtkWidget *win;
       GtkWidget *vb1;
       GtkWidget *hb;
       GtkWidget *t;
       GtkWidget *text;
       GtkWidget *entries[5];
   }compose;

   struct {
       GtkWidget *win;
       GtkWidget *nb;
       GtkWidget *fr[3];
       GtkWidget *fr_lbl[3];
       GtkWidget *box[4];
       GtkWidget *hb[3][10];
       GtkWidget *lbl[3][10];
       GtkWidget *e[3][10];
       GtkWidget *l[3][10];
   }conf;

   struct {
      FILE *f;
      char part;
      GtkWidget *win;
      GtkWidget *box1;
      GtkWidget *t;
   } sync;

   int cur_msg;
   int cl1_row;
   int sigchild;
   char sortby_last;
   char current_folder[100];
   char heb_font_name[100];
   char fromto[10];
   GdkColor *heb_fg, *heb_bg;
   GdkFont *heb_font;
} self;

struct dirent * direntptr;
DIR *dir;
FILE *f;

void l2vml(char *src0,char *dst0);
void heb_l2v(char *s)
{
	char *s2;
	s2=(char *)malloc(strlen(s)+10);
	l2vml(s,s2);
	strcpy(s,s2);
	free(s2);
}

inline char file_exists(char *s)
{
   struct stat st;
   char c;
   c=(!stat(s,&st));
   return c;
}

inline char starts_with(char *str, char *substr)
{
        return (strstr(str,substr)==str);
}

inline char ends_with(char *str, char *substr)
{
        return (strcmp(str+strlen(str)-strlen(substr),substr)==0);
}


char *b2a_base64(char *s)
{
   const char base64set[64]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   static char *s2=NULL;
   int i,j,val,ofs;

   if (s2!=NULL) free(s2);
   s2=(char *)malloc(strlen(s)*8/6+10);
   for(i=j=val=ofs=0;s[i];i++)
   {
      s2[j++]=base64set[((val<<(6-ofs))|(s[i]>>(ofs+2)))];
      val=((1<<(ofs+2))-1)&s[i];
      ofs=(ofs+8)%6;
      if (!ofs)
      {
         s2[j++]=base64set[val];
         val=0;
      }

   }
   if (ofs)
   {
      s2[j++]=base64set[((val<<(6-ofs))|(s[i]>>(ofs+2)))];
      for(ofs=(6-ofs)/2;ofs;ofs--) s2[j++]='=';
   }
   s2[j++]=0;
   return s2;
}


FILE *search_email_open_file(char *key, char *database_path)
{
	int i=0;
	char *sp;
	char s[1000];
	FILE *f;

	if (!*key) return NULL;
	if (!file_exists("/usr/bin/sqlite"))
	{
		if (!i) printf("sqlite not found\n");
		i=1;
		return NULL;
	}
	//sprintf(s,"'echo \"select * from contacts where tag like '\"'\"'%%EMAIL'\"'\"' and urn = ( select urn from contacts where (tag='\"'\"'NAME'\"'\"' and value like '\"'\"'%s%%'\"'\"') or ( tag like '\"'\"'%%EMAIL'\"'\"' and value like '\"'\"'%s%%'\"'\"'));\" |/usr/bin/sqlite %s|sed '\"'\"'s/|.*//'\"'\"' '",key,key,database_path);
	//printf("key='%s'\n",key);
	sprintf(s,"echo \"select * from contacts where tag like '%%EMAIL' and urn = ( select urn from contacts where (tag='NAME' and value like '%s%%') or ( tag like '%%EMAIL' and value like '%s%%'));\" |/usr/bin/sqlite %s|sed 's/.*|//' ",key,key,database_path);
	//printf("exeuting: %s\n",s);
	f=(FILE *)popen(s,"r");
	//printf("executed\n");
	return f;
}

char *search_email(char *key)
{
	char *sp;
	static FILE *f;
	static char s[200];
	static char *saved_key;

	if (key!=NULL)
	{
		saved_key=key;
		tilde("~/.gpe/contacts",s);
		f=search_email_open_file(saved_key,s);
	}
	sp=NULL;
	if (f!=NULL)
	{
		sp=(char *)fgets(s,sizeof(s)-1,f);
		if (NULL==sp)
		{
			pclose(f);
			f=NULL;
		} else {
			if (*(sp+strlen(sp)-1)=='\n') *(sp+strlen(sp)-1)=0;
		}
	}
	//if (sp) printf(" -> %s\n",sp);
	return sp;
	/*
	while (f!=NULL)
	{
		sp=fgets(s,sizeof(s)-1,f);
		if (sp!=NULL)
			printf(" -> %s",sp);
		else
		{
			pclose(f);
			f=NULL;
		}
	}
	*/

}



void a2b_base64(char *s)
{
   char c;
   int i,j,val,ofs;
   static char initialized=0;
   static signed char conv[256];

   if (!initialized)
   {
      for(i=0;i<256;i++)
      {
      if (i<='Z' && i>='A' ) conv[i]=i-'A';
      else
           {
         if (i<='z' && i>='a' ) conv[i]=i-'a'+26;
         else
         {
            if (i<='9' && i>='0' ) conv[i]=i-'0'+52;
       else if (i=='+') conv[i]=62;
       else if (i=='/') conv[i]=63;
       else conv[i]=-1;
         }
      }
      }
      initialized=1;
   }

   for(i=j=val=ofs=0;(s[j])&&(conv[s[j]]>=0);j++)
   {
      c=conv[s[j]];
      if (!ofs)
      {
      ofs=6;
      val=c;
      } else {
         s[i++]=(val<<(8-ofs))|(c>>(ofs-2));
      ofs=ofs-2;
         val=c&((1<<ofs)-1);
      }
   }
   s[i++]=0;
}


static int get_timezone ( void )           /* compute offset from GMT */
{
#define FDAY (24*60*60)                    /* full day in seconds */
#define HDAY (FDAY/2)                      /* half day in seconds */
struct tm ofst;
long  tloc ;                               /* seconds from 1/1/1970 0.0.0 GMT */
int sec_ofst ;

   time (& tloc) ;                         /* get current time */
   ofst = *gmtime (& tloc) ;               /* find GMT hour */
   sec_ofst = ( ofst.tm_hour * 60 +
                ofst.tm_min ) * 60 +
                ofst.tm_sec ;              /* GMT hour (in seconds) */
   ofst = *localtime (& tloc) ;               /* find GMT hour */
   sec_ofst = ( ( ofst.tm_hour * 60 +
                  ofst.tm_min ) * 60 +
                  ofst.tm_sec - sec_ofst
                + FDAY ) % FDAY ;          /* Local - GMT hours (in seconds, 0-FDAY) */
   if ( sec_ofst > HDAY )
       sec_ofst -= FDAY ;                  /* sec_ofst in range -11:59:59 +12:00:00 */
   return ( sec_ofst ) ;
}

int tilde(char *file,char * exp)
{
  *exp = '\0';
  if (file)
  {
    if (*file == '~')
    {
      char user[128];
      struct passwd *pw = NULL;
      int i = 0;
      
      user[0] = '\0';
      file++;
      while (*file != '/' && i < sizeof(user))
        user[i++] = *file++;
      user[i] = '\0';
      if (i == 0)
      {
        char *login = (char *) getlogin();
        
        if (login == NULL && (pw = getpwuid(getuid())) == NULL)
          return (0);
        if (login != NULL)
          strcpy(user, login);
      }
      if (pw == NULL && (pw = getpwnam(user)) == NULL)
        return (0);
      strcpy(exp, pw->pw_dir);
    }
    strcat(exp, file);
    return (1);
  } return (0);
}

void trunc_newlines(char *s)
{
   while (s[0] && ((s[strlen(s)-1]=='\n')||(s[strlen(s)-1]=='\r'))) s[strlen(s)-1]=0;
}

int hexchar_sub(char c,char w)
{
   if (c>='0' && c<='9') return c-'0';
   if (c>='A' && c<='F') return c-'A'+10;
   if (c>='a' && c<='f') return c-'a'+10;
   if (w) printf("error, char(%d) is not a hexchar\n",(int)c);
   return 16;

}

inline int hexchar(char c){return hexchar_sub(c,1);}
inline char is_hexchar(char c){return hexchar_sub(c,0)!=16;}

void q2b(char *s)
{
   char *sp;

   sp=s;
   while (*sp)
   {
      while (*sp&&*sp!='='&&*sp!='_') sp++;
      if (*sp=='_') *sp=' ';
      else if (*sp=='='&&is_hexchar(*(sp+1))&&is_hexchar(*(sp+2)))
      {
         *(sp++)=16*hexchar(*(sp+1))+hexchar(*(sp+2));
         strcpy(sp,sp+2);
      } else sp++;
      
   }
}

void convert(char *s)
{
   char *sp;
   char *sp2;
   char method;
   char heb;

   //printf("\nconvert: got '%s', ",s);
   
   heb=0;
   // skip spaces
   sp=s;
   while (*sp==' ') sp++;
   if (s!=sp) strcpy(s,sp);

   // look for '=?', if not found do return
   if (NULL==(sp=strstr(s,"=?"))) return;
   sp2=sp;
   if (starts_with(sp,"=?windows-1255?")) heb=1;


   sp+=2; // skip the =?
   while (*sp && *sp!='?')sp++; // skip the encoding
   sp++; // skip the ?
   method=*(sp++); // find if Q or B
   sp++; // skip the ?
   if (method=='B') a2b_base64(sp);
   else if (method=='Q') q2b(sp);
   if (strstr(sp,"?=")!=NULL) *strstr(sp,"?=")=0; // trancate '?=' till end
   strcpy(sp2,sp);
   if (heb) heb_l2v(sp2);

   //printf("convert: returning '%s'\n",s);
   
}

char *get_config(char *section,char *param)
{
   int i,j;

   for(j=0;options.titles[j];j++)
   {
      if (0==strcmp((options.titles[j]),(section)))
      {
         for(i=0;options.opts[j][i];i++)
         {
            if (0==strcmp((options.opts[j][i]),(param)))
            return config.conf[j][i];
      }
      printf("param '%s' not found\n",param);
      return NULL;
      }
   }
   printf("section '%s' not found\n",section);
   return NULL;
}

GtkWidget *mkbutton(char *xpm_name, char *but_name,char *text)
{
   const char xpm_dirs[]="/usr/share/gpe-mail/icons|/home/erez/xpms|./xpms";
   char xpm_path[100];
   int i,j;
   GdkPixmap *gdk_pixmap;
   GtkWidget *gtk_pixmap;
   GdkBitmap *gdk_mask;
   GtkWidget *b;

   if (!*text) text=but_name;
   j=0;
   while (xpm_dirs[j]&&j>=0)
   {
      for (i=j;xpm_dirs[i]&& xpm_dirs[i]!='|';i++) xpm_path[i-j]=xpm_dirs[i];
      xpm_path[(i++)-j]='/';
      xpm_path[i-j]=0;
      j=i;
      strcat(xpm_path,xpm_name);
      gdk_pixmap=gdk_pixmap_create_from_xpm(self.w->window,&gdk_mask,NULL,xpm_path);
      if (gdk_pixmap!=NULL) j=-1;

   }
   if (j>=0) return gtk_button_new_with_label(but_name);
   if (NULL==gdk_pixmap) printf("gdk_pixmap==NULL\n");
   if (NULL==gdk_mask) printf("gdk_mask==NULL\n");
   gtk_pixmap=gtk_pixmap_new(gdk_pixmap,gdk_mask);
   gdk_pixmap_unref(gdk_pixmap);
   gdk_pixmap_unref(gdk_mask);
   //printf("using icon file: %s\n",xpm_path);
   b=gtk_button_new();
   gtk_container_add (GTK_CONTAINER (b), gtk_pixmap);
   gtk_widget_show(gtk_pixmap);
   gtk_tooltips_set_tip(self.tt,b,text,NULL);
   return b;
   
}

void loadconfig()
{
   FILE *f;
   char s[100],s2[100];
   char *sp,*sp2;
   int i,j,k;

   tilde("~/.gpemailrc",s);
   f=(FILE *)fopen(s,"r");
   if (!f)
   {
      perror("fopen:");
      return;
   }
   s[0]=1;
   while (s[0])
   {
      s[0]=0;
      fgets(s,sizeof(s),f);
      if (s[0])
      {
         for(j=0;options.titles[j];j++) for(i=0;options.opts[j][i];i++)
         {
            sprintf(s2,"\"%s.%s\"",options.titles[j],options.opts[j][i]);
            if (NULL!=(sp=strstr(s,s2)))
            {
               sp2=strchr(s,'#');
               if ((sp2==NULL)||(sp2>sp))
               {
                  sp+=strlen(s2);
                  sp=strchr(sp,'=');
                  if (sp!=NULL)
                  {
                     sp++;
                     while (*sp==' '||*sp=='\t') sp++;
                     if (*sp=='"')
                     {
                        sp++;
                        sp2=strrchr(sp,'"');
                        if (sp2!=NULL)
                        {
                           *sp2=0;
                           strcpy(config.conf[j][i],sp);
                        }
                     } else {
                        if (*sp=='0'||*sp=='1')
                        {
                           if (*sp=='0') config.conf[j][i][0]=2;
                           else
                              config.conf[j][i][0]=1;
                           config.conf[j][i][1]=0;
                        }

                     }
                  }
               }
            }

         }
      }
   }
   for(j=0;options.titles[j];j++) for(i=0;options.opts[j][i];i++)
   {
      if (config.conf[j][i][0]&&(config.conf[j][i][0]<3))
         fprintf(f,"\"%s.%s\"=%d\n",options.titles[j],options.opts[j][i],(int)config.conf[j][i][0]%2);
      else
         fprintf(f,"\"%s.%s\"=\"%s\"\n",options.titles[j],options.opts[j][i],config.conf[j][i]);

   }
   fclose(f);
}

void saveconfig()
{
   FILE *f;
   char s[100];
   int i,j;

   tilde("~/.gpemailrc",s);
   f=(FILE *)fopen(s,"w");
   if (!f)
   {
      perror("fopen:");
      return;
   }
   for(j=0;options.titles[j];j++) for(i=0;options.opts[j][i];i++)
   {
      if (config.conf[j][i][0]&&(config.conf[j][i][0]<3))
         fprintf(f,"\"%s.%s\"=%d\n",options.titles[j],options.opts[j][i],(int)config.conf[j][i][0]%2);
      else
         fprintf(f,"\"%s.%s\"=\"%s\"\n",options.titles[j],options.opts[j][i],config.conf[j][i]);

   }
   fclose(f);
}

void configbutton_cancel(GtkButton *button, gpointer user_data)
{
   gtk_widget_hide(self.conf.win);
   gtk_widget_destroy(self.conf.win);
   self.conf.win=NULL;
}

void configbutton_ok(GtkButton *button, gpointer user_data)
{
   int i,j,k;

   for(j=0;options.titles[j];j++) for(i=0;options.opts[j][i];i++)
   {
      if (config.conf[j][i][0]&&(config.conf[j][i][0]<3))
      {
         k=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.conf.l[j][i]));
         if (!k) k=2;
         config.conf[j][i][0]=k;
         config.conf[j][i][1]=0;
      } else {
	 if ((0!=strcmp(options.opts[j][i],"Password"))||(0!=strcmp("***",gtk_entry_get_text(GTK_ENTRY(self.conf.e[j][i])))))
         strcpy(config.conf[j][i],gtk_entry_get_text(GTK_ENTRY(self.conf.e[j][i])));
      }

   }
   gtk_widget_hide(self.conf.win);
   saveconfig();
   gtk_widget_destroy(self.conf.win);
   self.conf.win=NULL;
}

void do_config(GtkButton *button, gpointer user_data)
{
   int i,j;
   GtkWidget *bx,*bt1,*bt2;

   if ((NULL==self.conf.win)||!GTK_IS_WIDGET(self.conf.win))
   {
      self.conf.win=gtk_window_new(GTK_WINDOW_TOPLEVEL);

      self.conf.nb=gtk_notebook_new();
      gtk_notebook_set_tab_pos(GTK_NOTEBOOK(self.conf.nb),GTK_POS_TOP);


      for (i=0;options.titles[i];i++)
      {
         self.conf.fr[i]=gtk_frame_new(options.titles[i]);
         gtk_container_set_border_width(GTK_CONTAINER(self.conf.fr[i]),10);
         gtk_frame_set_shadow_type(GTK_FRAME(self.conf.fr[i]),GTK_SHADOW_ETCHED_OUT);

         self.conf.fr_lbl[i]=gtk_label_new(options.titles[i]);
         gtk_notebook_append_page(GTK_NOTEBOOK(self.conf.nb),self.conf.fr[i],self.conf.fr_lbl[i]);
      }

      for(i=0;i<4;i++)
      {
         self.conf.box[i]=gtk_vbox_new(FALSE,0);
         if (i<3) gtk_container_add(GTK_CONTAINER(self.conf.fr[i]),self.conf.box[i]);
      }
        gtk_box_pack_start (GTK_BOX (self.conf.box[3]), self.conf.nb, TRUE, TRUE, 0);
      gtk_container_add(GTK_CONTAINER(self.conf.win),self.conf.box[3]);

      for(j=0;options.titles[j];j++) for(i=0;options.opts[j][i];i++)
      {
         self.conf.hb[j][i]=gtk_hbox_new(FALSE,0);
           gtk_box_pack_start (GTK_BOX (self.conf.box[j]), self.conf.hb[j][i], TRUE, TRUE, 0);
         if (config.conf[j][i][0]&&(config.conf[j][i][0]<3))
         {
            self.conf.l[j][i]=gtk_check_button_new_with_label(options.opts[j][i]);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.conf.l[j][i]),config.conf[j][i][0]&1);
         }
         else
         {
         
            self.conf.l[j][i]=gtk_label_new(options.opts[j][i]);
            self.conf.e[j][i]=gtk_entry_new();
              gtk_box_pack_end (GTK_BOX (self.conf.hb[j][i]), self.conf.e[j][i], TRUE, TRUE, 0);
            if (j==0&&(0==strcmp(options.opts[j][i],"Password")))
               gtk_entry_set_text(GTK_ENTRY(self.conf.e[j][i]),"***");
            else
               gtk_entry_set_text(GTK_ENTRY(self.conf.e[j][i]),config.conf[j][i]);
         }
           gtk_box_pack_start (GTK_BOX (self.conf.hb[j][i]), self.conf.l[j][i], TRUE, TRUE, 0);
      }
      bx=gtk_hbox_new(FALSE,0);
      gtk_box_pack_start (GTK_BOX (self.conf.box[3]), bx, TRUE, TRUE, 0);
      bt1=gtk_button_new_with_label("Cancel");
      bt2=gtk_button_new_with_label("OK");
      gtk_box_pack_start (GTK_BOX (bx), bt1, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (bx), bt2, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT(bt1), "clicked", 
         (GtkSignalFunc) configbutton_cancel, NULL);
      gtk_signal_connect (GTK_OBJECT(bt2), "clicked", 
         (GtkSignalFunc) configbutton_ok, NULL);


   }
   gtk_widget_show_all(self.conf.win);
}

int hide_sync_win()
{
   usleep(100000);
   if (self.sync.part++<22) return 1;
   gtk_widget_hide(self.sync.win);
   return 0;
}

int do_sync2();
int open_mailfolder();

int update_sync_text()
{
   char s[100];
   int i;
   
   //printf("before usleep\n");
   usleep(100000);
   i=fread(s,1,sizeof(s)-1,self.sync.f);
   if (i<1) {
      if (!self.sigchild) return 1;
      i=pclose(self.sync.f);
      if (!i&&self.sync.part==1)
      {
         gtk_text_insert((GtkText *)self.sync.t,self.heb_font,self.heb_fg,self.heb_bg,"\n\n\nReceive:\n\n",-1);
         gtk_idle_add(do_sync2,NULL);
      }
      else
      {
         if (!i) gtk_text_insert((GtkText *)self.sync.t,self.heb_font,self.heb_fg,self.heb_bg,"\n\nDone\n",-1);
      else gtk_text_insert((GtkText *)self.sync.t,self.heb_font,self.heb_fg,self.heb_bg,"\n\nError occured !\n",-1);
         gtk_idle_add(hide_sync_win,NULL);
         gtk_idle_add(open_mailfolder,NULL);
      }
      self.sync.part=2;
      return 0;
   }
   s[i]=0;
   gtk_text_insert((GtkText *)self.sync.t,self.heb_font,self.heb_fg,self.heb_bg,s,-1);
   return 1;
}

void onsigchild(int i)
{
   //printf("sigchild ...\n");
   self.sigchild=1;
}

int do_sync2()
{
   char s[200];
   char s2[5],s3[5];
   int dy,hh,mm,ss,yy;
   char *sp;
   time_t t;
   int i;

   sprintf(s,"/usr/bin/emailsync server=%s port=%s ssl=%d user=%s pass=%s proto=%s verbose=2 prefix=%s",get_config("Incoming","Server"),get_config("Incoming","Port"),(int)(1-((*get_config("Incoming","SSL"))-1)),get_config("Incoming","Username"),get_config("Incoming","Password"),(1-((*get_config("Incoming","Use pop3 instead of imap"))-1)) ? "pop":"imap",config.expanded_path);
   if (*get_config("Incoming","Size (bytes)"))
   {
      strcat(s," size=");
      strcat(s,get_config("Incoming","Size (bytes)"));
   }
   if (*get_config("Incoming","Since (days), imap only "))
   {
      i=0;
      sscanf(get_config("Incoming","Since (days), imap only "),"%d",&i);
      if (-1==time(&t)) i=0 ;
      if (i)
      {
         t-=24*3600*i;
         // convert "Mon Jul 29 14:47:20 2002" -> "29-Jul-2002"
         sscanf(ctime(&t),"%s %s %d %d:%d:%d %d",s2,s3,&dy,&hh,&mm,&ss,&yy);
         sprintf(s+strlen(s)," since=%02d-%s-%04d",dy,s3,yy);
      }
   }
   //printf("cmdline: %s \n",s);
   self.sigchild=0;
   self.sync.f=(FILE *)popen(s,"r");
   if (self.sync.f==NULL) {printf("error\n");gtk_widget_hide(self.sync.win);return;}
   fcntl(fileno(self.sync.f),F_SETFL,O_NONBLOCK);
   self.sync.part=2;
   //printf("before idle add #2\n");
   gtk_idle_add(update_sync_text,NULL);
   return 0;
}

void do_sync(GtkButton *button, gpointer user_data)
{
   char s[200];
   char do_send;

   do_send=1;

   sprintf(s,"%s/OUTBOX",config.expanded_path);
   if (!file_exists(s)) do_send=0;
   sprintf(s,"/usr/bin/emailsync server=%s port=%s ssl=%d proto=smtp verbose=2 prefix=%s",get_config("Outgoing","Smtp Server"),get_config("Outgoing","Port"),(int)(1-((*get_config("Outgoing","SSL"))-1)),config.expanded_path);
   //printf("cmdline(smtp): %s \n",s);
   if ((NULL==self.sync.win)||!GTK_IS_WIDGET(self.sync.win))
   {
      self.sync.win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
      self.sync.box1=gtk_vbox_new(FALSE,0);
      gtk_container_add(GTK_CONTAINER(self.sync.win),self.sync.box1);
      self.sync.t=gtk_text_new(NULL,NULL);
      gtk_box_pack_start (GTK_BOX (self.sync.box1), self.sync.t, TRUE, TRUE, 0);
      gtk_text_insert((GtkText *)self.sync.t,self.heb_font,self.heb_fg,self.heb_bg,"Sending mail:\n\nemailsync output:\n\nSend:\n",-1);

   }
   gtk_text_forward_delete((GtkText *)self.sync.t,gtk_text_get_length((GtkText *)self.sync.t));
   gtk_text_backward_delete((GtkText *)self.sync.t,gtk_text_get_length((GtkText *)self.sync.t));
   gtk_widget_show_all(self.sync.win);
   self.sigchild=0;
   signal(SIGCHLD,onsigchild);
   if (do_send)
   {
      self.sync.f=(FILE *)popen(s,"r");
      if (self.sync.f==NULL) {printf("error\n");gtk_widget_hide(self.sync.win);return;}
      fcntl(fileno(self.sync.f),F_SETFL,O_NONBLOCK);
      self.sync.part=1;
      //printf("before idle add\n");
      gtk_idle_add(update_sync_text,NULL);
      //sprintf(s,"/usr/bin/emailsync server=%s user=%s pass=%s port=%s size=%s proto=%s ssl=%s since=%s verbose=2
   } else do_sync2();
}

void do_delete(GtkButton *button, gpointer user_data)
{
   char s[200];

   if (!self.cur_msg) return;
   //printf("delete: %d\n",self.cur_msg);
   sprintf(s,"/bin/rm -f %s/%s/%d.*",config.expanded_path,self.current_folder,self.cur_msg);
   if (0!=strcmp(self.current_folder,"OUTBOX"))
      sprintf(s+strlen(s),";echo 'delete %d \"%s\"' >> %s/.onsync",self.cur_msg,self.current_folder,config.expanded_path);
   //printf("system('%s')\n",s);
   system(s);
   self.cur_msg=0;
   gtk_text_freeze((GtkText *)self.mt);
   gtk_text_forward_delete((GtkText *)self.mt,gtk_text_get_length((GtkText *)self.mt));
   gtk_text_backward_delete((GtkText *)self.mt,gtk_text_get_length((GtkText *)self.mt));
   gtk_text_thaw((GtkText *)self.mt);

   gtk_clist_freeze(GTK_CLIST(self.cl1));
   gtk_clist_remove(GTK_CLIST(self.cl1),self.cl1_row);
   if (gtk_clist_row_is_visible(GTK_CLIST(self.cl1),self.cl1_row))
      gtk_clist_select_row(GTK_CLIST(self.cl1),self.cl1_row,1);
   else
   if (gtk_clist_row_is_visible(GTK_CLIST(self.cl1),self.cl1_row-1))
   {
      self.cl1_row--;
      gtk_clist_select_row(GTK_CLIST(self.cl1),self.cl1_row,1);
      
   } else self.cl1_row=-1;
   gtk_clist_thaw(GTK_CLIST(self.cl1));
}


void compose_window_destroy(GtkButton *button, gpointer user_data)
{
   gtk_widget_hide(self.compose.win);
   gtk_widget_destroy(self.compose.win);
   self.compose.win=NULL;
}

void compose_window_send(GtkButton *button, gpointer user_data)
{
   time_t t;
   char s[1000];
   char s2[100];
   char s3[10];
   char *sp;
   int i;
   int h,m,sec,d,y;

   strcpy(s,gtk_entry_get_text(GTK_ENTRY(self.compose.entries[1])));
   if (!*s || 0==strcmp(s,"empty"))
   {
      gtk_entry_set_text(GTK_ENTRY(self.compose.entries[1]),"empty");
      printf("recepient missing\n");
      gtk_entry_select_region(GTK_ENTRY(self.compose.entries[1]),0,-1);
      return;
   }
   strcpy(s,gtk_entry_get_text(GTK_ENTRY(self.compose.entries[0])));
   if (0==strcmp(s,"root@localhost"))
   {
      printf("illeagle sender adress\n");
      gtk_entry_select_region(GTK_ENTRY(self.compose.entries[0]),0,-1);
      return;
   }
   sprintf(s,"From: %s\nTo: %s\nCc: %s\nBcc: %s\nDate: ",gtk_entry_get_text(GTK_ENTRY(self.compose.entries[0])),gtk_entry_get_text(GTK_ENTRY(self.compose.entries[1])),gtk_entry_get_text(GTK_ENTRY(self.compose.entries[2])),gtk_entry_get_text(GTK_ENTRY(self.compose.entries[3])));
   // convert 'Sun Aug 4 18:17:03 2002' -> 'Sun, 04 Aug 2002 18:17:03 '
   time(&t);
   sscanf(ctime(&t),"%s %s %d %d:%d:%d %d",s2,s3,&d,&h,&m,&sec,&y);
   sprintf(s+strlen(s),"%s, %02d %s %04d %02d:%02d:%02d ",s2,d,s3,y,h,m,sec);
   i=get_timezone();
   if (i<0) strcat(s,"-"); else strcat(s,"+");
   sprintf(s+strlen(s),"%02d%02d\nSubject: %s\nX-Mailer-Frontend: gpe-mail (%s)\n",i/3600,(i/60)%60,gtk_entry_get_text(GTK_ENTRY(self.compose.entries[4])),VERSION);
   sprintf(s2,"%s/OUTBOX/",config.expanded_path);
   if (!file_exists(s2))
   {
      sprintf(s2,"mkdir -p %s/OUTBOX",config.expanded_path);
      printf("%s\n",s2);
      system(s2);
      do_folders();
   }
   i=0;
   do
   {
      i++;
      sprintf(s2,"%s/OUTBOX/%d.head",config.expanded_path,i);
   } while (file_exists(s2));
   f=(FILE *)fopen(s2,"w");
   fwrite(s,strlen(s),1,f);
   fclose(f);
   strcpy(s2+strlen(s2)-4,"body");
   f=(FILE *)fopen(s2,"w");
   sp=gtk_editable_get_chars(GTK_EDITABLE(self.compose.text),0,-1);
   fwrite(sp,strlen(sp),1,f);
   fclose(f);
   if (0==strcmp(self.current_folder,"OUTBOX")) gtk_idle_add(open_mailfolder,NULL);
   compose_window_destroy(button, user_data);


}

void do_compose(GtkButton *button, gpointer user_data)
{
   char *q[]={"From","To","Cc","Bcc","Subj",NULL};
   int a;
   GtkWidget *l;
   GtkWidget *but1,*but2;
   char s[100];
   char s2[100];
   char **sp;
   char *sp2,*sp3;

   if (button!=(GtkButton *)self.composeb && (0==strcmp(self.current_folder,"OUTBOX"))) return;
   if (button!=(GtkButton *)self.composeb && self.cl1_row==-1) return;
   if ((NULL==self.compose.win)||!GTK_IS_WIDGET(self.compose.win))
   {
      self.compose.win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
      self.compose.vb1=gtk_vbox_new(FALSE,0);
      gtk_container_add(GTK_CONTAINER(self.compose.win),self.compose.vb1);
      self.compose.t=gtk_table_new(4,100,FALSE);
      gtk_box_pack_start (GTK_BOX (self.compose.vb1), self.compose.t, TRUE, TRUE, 0);
      for(a=0;q[a]!=NULL;a++)
      {
         sprintf(s,"%s:",q[a]);
         l=gtk_label_new(s);
         gtk_table_attach_defaults(GTK_TABLE(self.compose.t),l,1,2,a+1,a+2);
	 if (a>0&&a<4)
	 {
		 self.compose.entries[a]=gtk_colombo_new();
		 gtk_colombo_set_func(GTK_COLOMBO(self.compose.entries[a]),search_email);
	 }
	 else self.compose.entries[a]=gtk_entry_new();
         gtk_table_attach_defaults(GTK_TABLE(self.compose.t),self.compose.entries[a],2,3,a+1,a+2);
         if (0==strcmp(q[a],"From"))
            gtk_entry_set_text(GTK_ENTRY(self.compose.entries[a]),get_config("Outgoing","Email Adress"));

      }
      self.compose.text=gtk_text_new(NULL,NULL);
      gtk_box_pack_start (GTK_BOX (self.compose.vb1), self.compose.text, TRUE, TRUE, 0);
      gtk_text_set_editable(GTK_TEXT(self.compose.text),1);
      self.compose.hb=gtk_hbox_new(FALSE,0);
      gtk_box_pack_start (GTK_BOX (self.compose.vb1), self.compose.hb, TRUE, TRUE, 0);
      but1=gtk_button_new_with_label("Cancel");
      gtk_signal_connect (GTK_OBJECT(but1), "clicked", 
         (GtkSignalFunc) compose_window_destroy, NULL);
      but2=gtk_button_new_with_label("Send");
      gtk_signal_connect (GTK_OBJECT(but2), "clicked", 
         (GtkSignalFunc) compose_window_send, NULL);
      gtk_box_pack_start (GTK_BOX (self.compose.hb), but1, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (self.compose.hb), but2, TRUE, TRUE, 0);
   }
   if (button==GTK_BUTTON(self.forwardb))
   {
      sp=(char **)&s;
      if (0==gtk_clist_get_text((GtkCList *)self.cl1,self.cl1_row,0,sp)) {printf("can not find file name in clist\n");return;}
      while (**sp==' ') (*sp)++;
      if (starts_with(*sp,"Fwd:")) *sp+=4;
      while (**sp==' ') (*sp)++;
      sprintf(s2,"Fwd: %s",*sp);
      gtk_entry_set_text(GTK_ENTRY(self.compose.entries[4]),s2);
      s2[0]=0;
      gtk_entry_set_text(GTK_ENTRY(self.compose.entries[1]),s2);
   }
   if (button==GTK_BUTTON(self.replyb))
   {
      sp=(char **)&s;
      if (0==gtk_clist_get_text((GtkCList *)self.cl1,self.cl1_row,0,sp)) {printf("can not find file name in clist\n");return;}
      while (**sp==' ') (*sp)++;
      if (starts_with(*sp,"Re:")) *sp+=3;
      while (**sp==' ') (*sp)++;
      sprintf(s2,"Re: %s",*sp);
      gtk_entry_set_text(GTK_ENTRY(self.compose.entries[4]),s2);
      sp=(char **)&s;
      if (0==gtk_clist_get_text((GtkCList *)self.cl1,self.cl1_row,1,sp)) {printf("can not find file name in clist\n");return;}
      gtk_entry_set_text(GTK_ENTRY(self.compose.entries[1]),*sp);
   }
   gtk_text_freeze(GTK_TEXT(self.compose.text));
   gtk_text_forward_delete((GtkText *)self.compose.text,gtk_text_get_length((GtkText *)self.compose.text));
   gtk_text_backward_delete((GtkText *)self.compose.text,gtk_text_get_length((GtkText *)self.compose.text));
   if (button==GTK_BUTTON(self.composeb))
   {
      s2[0]=0;
      gtk_entry_set_text(GTK_ENTRY(self.compose.entries[1]),s2);
      gtk_entry_set_text(GTK_ENTRY(self.compose.entries[4]),s2);
   } else {
      gtk_text_insert((GtkText *)self.compose.text,self.heb_font,self.heb_fg,self.heb_bg,"\n\nOriginal Message:\n",-1);
      sp2=gtk_editable_get_chars(GTK_EDITABLE(self.mt),0,-1);
      while (sp2!=NULL)
      {
         sp3=strchr(sp2,'\n');
         if (sp3!=NULL)
         {
            gtk_text_insert((GtkText *)self.compose.text,self.heb_font,self.heb_fg,self.heb_bg,"> ",-1);
            gtk_text_insert((GtkText *)self.compose.text,self.heb_font,self.heb_fg,self.heb_bg,sp2,sp3-sp2+1);
            sp2=sp3+1;
         } else sp2=NULL;
      }

   }
   gtk_text_thaw((GtkText *)self.compose.text);
   gtk_widget_show_all(self.compose.win);
}

void on_col_click(GtkCList *clist, gint column, gpointer user_data)
{
   if (column==2) gtk_clist_set_sort_column(clist,4);
   else gtk_clist_set_sort_column(clist,column);
   if (column==self.sortby_last)
      gtk_clist_set_sort_type(clist,1-clist->sort_type);
   self.sortby_last=column;
   gtk_clist_sort(clist);
}

void on_subj_sel (GtkCList *clist, gint row, gint column,
      GdkEventButton *event, gpointer user_data)
{
   char s[100];
   char **sp;
   char s2[100000];
   char s3[500];
   char boundary[500];
   char heb;
   char *sp2;
   char type;
   int i;
   sp=(char **)&s;
   gtk_text_freeze((GtkText *)self.mt);
   gtk_text_forward_delete((GtkText *)self.mt,gtk_text_get_length((GtkText *)self.mt));
   gtk_text_backward_delete((GtkText *)self.mt,gtk_text_get_length((GtkText *)self.mt));
   //gtk_text_set_point((GtkText *)self.mt,0);
   //gtk_text_forward_delete((GtkText *)self.mt,1e9);
   //printf("row=%d\n",row);
   if (0==gtk_clist_get_text((GtkCList *)self.cl1,row,3,sp)) {printf("can not find file name in clist\n");gtk_text_thaw((GtkText *)self.mt);return;}
   //printf("s=%s\n",*sp);
   sscanf(*sp,"%d.",&self.cur_msg);
   //printf("num=%d\n",self.cur_msg);
   self.cl1_row=row;
   sprintf(s2,"%s/%s/%d.head",config.expanded_path,self.current_folder,self.cur_msg);
   //printf("openning: '%s'\n",s2);
   f=(FILE *)fopen(s2,"r");
   boundary[0]=0;
   heb=0;
   if (f)
   {
      do
      {
         s2[0]=0;
         fgets(s2,sizeof(s2),f);
         if (!s2[0]) break;
	 //printf("s2=%s\n",s2);
         convert(s2);
         if (NULL!=(sp2=strstr(s2,"boundary=\"")))
         {
            sp2+=strlen("boundary=\"");
            if (NULL!=strchr(sp2,'"'))
            {
               *strchr(sp2,'"')=0;
               strcpy(boundary,sp2);
	       //printf("boundary=%s\n",boundary);
            }
         }
	 if (NULL!=(sp2=strstr(s2,"charset=\"windows-1255\""))) heb=1;
	 //if (NULL!=(sp2=strstr(s2,"charset="))) printf("%s\n",sp2);
      } while (s2[0]);
      fclose(f);
   }


   sprintf(s2,"%s/%s/%d.body",config.expanded_path,self.current_folder,self.cur_msg);
   strcpy(s3,self.fromto);
   if (0==gtk_clist_get_text((GtkCList *)self.cl1,row,1,sp)) {printf("can not find file name in clist\n");gtk_text_thaw((GtkText *)self.mt);return;}
   strcat(s3,*sp);
   strcat(s3,"\nSubject: ");
   if (0==gtk_clist_get_text((GtkCList *)self.cl1,row,0,sp)) {printf("can not find file name in clist\n");gtk_text_thaw((GtkText *)self.mt);return;}
   strcat(s3,*sp);
   strcat(s3,"\nDate: ");
   if (0==gtk_clist_get_text((GtkCList *)self.cl1,row,2,sp)) {printf("can not find file name in clist\n");gtk_text_thaw((GtkText *)self.mt);return;}
   strcat(s3,*sp);
   strcat(s3,"\n\n");
   gtk_text_insert((GtkText *)self.mt,self.heb_font,self.heb_fg,self.heb_bg,s3,-1);

   //printf("opening: '%s'\n",s2);
   f=(FILE *)fopen(s2,"r");
   if (f) 
   {
      s2[0]=0;
      if (boundary[0])
      {
         do
         {
            s2[0]=0;
	    fgets(s2,sizeof(s2),f);
	    //if (s2[0]) printf("removing '%s'\n",s2);
         } while (s2[0]&&!strstr(s2,boundary));
	 type=0;
	 while (s2[0]&&s2[0]!='\n')
	 {
            s2[0]=0;
	    fgets(s2,sizeof(s2),f);
	    if ((sp2=strstr(s2,"Content-Transfer-Encoding: base64"))!=NULL) type='B';
	    if ((sp2=strstr(s2,"Content-Transfer-Encoding: quoted-printable"))!=NULL) type='Q';
	    if (NULL!=(sp2=strstr(s2,"charset=\"windows-1255\""))) heb=1;
	    //if (NULL!=(sp2=strstr(s2,"charset="))) printf("%s\n",sp2);
	 }
      }
      s2[0]=0;
      do
      {
         sp2=s2+strlen(s2);
         sp2[0]=0;
	 fgets(sp2,sizeof(s2)-strlen(s2),f);
	 if (!*sp2) break;
	 //printf(":%s",sp2);
	 if (boundary[0])
	 	if (strstr(sp2,boundary)) *sp2=0;
      } while (*sp2);
      if (type=='B') a2b_base64(s2);
      else if (type=='Q') q2b(s2);
      if (heb)
      {
         //printf("%s -> \n",s2);
         heb_l2v(s2);
         //printf(" -> %s\n",s2);
      }
      gtk_text_insert((GtkText *)self.mt,self.heb_font,self.heb_fg,self.heb_bg,s2,-1);
      //printf("body:\n%s\n",s2);
      fclose(f);
   } else gtk_text_insert((GtkText *)self.mt,self.heb_font,self.heb_fg,self.heb_bg,"-----------------------------\n     body has not been\n  downloaded with message\n-----------------------------\n",-1);
   gtk_text_thaw((GtkText *)self.mt);
}

void on_mi_act (GtkMenuItem *menuitem, gpointer user_data)
{
   //printf("%s\n",(char *)user_data);
   strcpy(self.current_folder,(char *)user_data);
   open_mailfolder();
   gtk_text_freeze((GtkText *)self.mt);
   gtk_text_forward_delete((GtkText *)self.mt,gtk_text_get_length((GtkText *)self.mt));
   gtk_text_backward_delete((GtkText *)self.mt,gtk_text_get_length((GtkText *)self.mt));
   gtk_text_thaw((GtkText *)self.mt);
}


void add_mailfolder(char *name)
{
   GtkWidget *mi;
   mi=gtk_menu_item_new_with_label(name);
   gtk_widget_show(mi);
   gtk_menu_prepend(GTK_MENU(self.foldermenu),mi);
   gtk_signal_connect (GTK_OBJECT(mi), "activate", 
      (GtkSignalFunc) on_mi_act, name);
}

int open_mailfolder()
{
   char path[100];
   char s[100];
   char subj[100];
   char from[100];
   char date[100];
   char fname[100];
   char *a[6]={subj,from,date,fname,s,NULL};

   int i;

   self.cur_msg=0;
   self.cl1_row=-1;
   usleep(10000);
   gtk_clist_clear(GTK_CLIST(self.cl1));
   strcpy(path,config.expanded_path);
   strcat(path,"/");
   strcat(path,self.current_folder);
   if (strcmp(self.current_folder,"OUTBOX"))
      strcpy(self.fromto,"From:");
   else
      strcpy(self.fromto,"To:");
   gtk_clist_set_column_title(GTK_CLIST(self.cl1),1,self.fromto);
   if (!file_exists(path))
   {
	   sprintf(s,"/bin/mkdir -p %s",path);
	   system(s);
   }
   dir=opendir(path);
   if (!dir) {perror(path);exit(0);} 
   direntptr=(void *)1;
   do
   {
      direntptr=readdir(dir);
      if (direntptr)
      {
         if (ends_with(direntptr->d_name,".head"))
         {
            sscanf(direntptr->d_name,"%d.head",&i);
            strcpy(s,path);
            strcat(s,"/");
            strcat(s,direntptr->d_name);
            //printf("opening: %s \n",s);
            f=(FILE *)fopen(s,"r");
            subj[0]=0;
            from[0]=0;
            date[0]=0;
            do
            {
               s[0]=0;
               fgets(s,sizeof(s),f);
               convert(s);
               if (starts_with(s,"Subject:")) strcpy(subj,s+9);
               if (starts_with(s,self.fromto)) strcpy(from,s+strlen(self.fromto)+1);
               if (starts_with(s,"Date:")) strcpy(date,s+6);
            } while (s[0]);
            trunc_newlines(subj);
            trunc_newlines(from);
            trunc_newlines(date);
            fclose(f);
            sprintf(s,"%08d",i);
            strcpy(fname,direntptr->d_name);
            gtk_clist_append(GTK_CLIST(self.cl1),a);
         }
      }
   } while (direntptr);

   self.sortby_last=2;
   gtk_clist_set_sort_column(GTK_CLIST(self.cl1),4);
   gtk_clist_sort(GTK_CLIST(self.cl1));
   gtk_clist_thaw(GTK_CLIST(self.cl1));
   return FALSE;
}

int do_folders()
{
   char s[200];
   char inbox_exists=0;

   usleep(10000);
   if (!file_exists(config.expanded_path))
   {
	   sprintf(s,"/bin/mkdir -p %s",config.expanded_path);
	   system(s);
   }
   dir=opendir(config.expanded_path);
   if (!dir) {perror(config.expanded_path);exit(0);} 
   direntptr=(void *)1;
   do
   {
      direntptr=readdir(dir);
      if (direntptr)
      {
         if ((strcmp(direntptr->d_name,"..")!=0) &&
             (strcmp(direntptr->d_name,"." )!=0))
            add_mailfolder(direntptr->d_name);
	 if (0==strcmp(direntptr->d_name,"inbox")) inbox_exists=1;
      }
   } while (direntptr);
   if (!inbox_exists) add_mailfolder("inbox");
   
   strcpy(self.current_folder,"inbox");
   gtk_idle_add(open_mailfolder,NULL);
   return FALSE;
}

int main(int argc, char **argv)
{

   gchar *titles[]={"Subject","From","Sent","filename","uid"};
   GtkStyle *gtk_style;

   tilde(config.path,config.expanded_path);
   loadconfig();

   // set locale
   gtk_set_locale();
   
   // init gtk
   gtk_init(&argc, &argv);

   // main window
   self.w=gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_signal_connect (GTK_OBJECT(self.w), "delete-event", 
      (GtkSignalFunc) gtk_main_quit, NULL);
      //(GtkSignalFunc) gtk_true, NULL);
   gtk_signal_connect (GTK_OBJECT(self.w), "destroy", 
      (GtkSignalFunc) gtk_main_quit, NULL);


   self.b0=gtk_vbox_new(FALSE,0);
   gtk_container_add (GTK_CONTAINER (self.w), self.b0);
   gtk_widget_show(self.b0);

   self.hb2=gtk_hbox_new(FALSE,0);
     gtk_box_pack_start (GTK_BOX (self.b0), self.hb2, FALSE, FALSE, 0);
   gtk_widget_show(self.hb2);

   // subject/msg pane
   self.b2=gtk_vpaned_new();
   gtk_widget_show(self.b2);

   //subject sw
   self.sw2=gtk_scrolled_window_new(NULL,NULL);
   gtk_widget_set_usize(self.sw2,-1,80);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self.sw2),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
   gtk_widget_show(self.sw2);

   //msg body
   self.hb3=gtk_hbox_new(FALSE,0);
     gtk_paned_add2 ((GtkPaned *)self.b2, self.hb3 );
   self.vb3=gtk_vbox_new(FALSE,0);
   gtk_widget_show(self.vb3);

   self.folderoption=gtk_option_menu_new();
   gtk_widget_show(self.folderoption);
   self.foldermenu=gtk_menu_new();
   gtk_widget_show(self.foldermenu);
   gtk_option_menu_set_menu((GtkOptionMenu *)self.folderoption,self.foldermenu);
     gtk_box_pack_start (GTK_BOX (self.vb3), self.folderoption, FALSE, FALSE, 0);
     gtk_paned_add1 ((GtkPaned *)self.b2, self.vb3);
     gtk_box_pack_start (GTK_BOX (self.vb3), self.sw2, TRUE, TRUE, 0);
   gtk_widget_show(self.hb3);


   // buttons
   self.tt=gtk_tooltips_new();
   self.configb=mkbutton("configure_16_mail.xpm","Config","Configure Setting");
   self.conf.win=NULL;

   self.syncb=mkbutton("send-receive16.xpm","Sync","Send-Receive");
   self.sync.win=NULL;

     gtk_box_pack_start (GTK_BOX (self.b0), self.b2, TRUE, TRUE, 0);
     gtk_box_pack_start (GTK_BOX (self.hb2), self.configb, TRUE, TRUE, 0);
     gtk_box_pack_start (GTK_BOX (self.hb2), self.syncb, TRUE, TRUE, 0);


   gtk_signal_connect (GTK_OBJECT(self.configb), "clicked", 
      (GtkSignalFunc) do_config, NULL);
   gtk_signal_connect (GTK_OBJECT(self.syncb), "clicked", 
      (GtkSignalFunc) do_sync, NULL);

   self.deleteb=mkbutton("mini-cross.xpm","Delete","");
     gtk_box_pack_start (GTK_BOX (self.hb2), self.deleteb, TRUE, TRUE, 0);
   gtk_signal_connect (GTK_OBJECT(self.deleteb), "clicked", 
      (GtkSignalFunc) do_delete, NULL);

   self.composeb=mkbutton("edit.xpm","Compose","");
     gtk_box_pack_start (GTK_BOX (self.hb2), self.composeb, TRUE, TRUE, 0);
   gtk_signal_connect (GTK_OBJECT(self.composeb), "clicked", 
      (GtkSignalFunc) do_compose, NULL);

   self.forwardb=mkbutton("forward.xpm","Fwd","Forward");
     gtk_box_pack_start (GTK_BOX (self.hb2), self.forwardb, TRUE, TRUE, 0);
   gtk_signal_connect (GTK_OBJECT(self.forwardb), "clicked", 
      (GtkSignalFunc) do_compose, NULL);

   self.replyb=mkbutton("reply.xpm","Reply","");
     gtk_box_pack_start (GTK_BOX (self.hb2), self.replyb, TRUE, TRUE, 0);
   gtk_signal_connect (GTK_OBJECT(self.replyb), "clicked", 
      (GtkSignalFunc) do_compose, NULL);

   
   gtk_tooltips_enable(self.tt);
   // subject clist
   self.cl1=gtk_clist_new_with_titles(5,titles);
   gtk_clist_set_column_width(GTK_CLIST(self.cl1),0,100);
   gtk_clist_set_column_width(GTK_CLIST(self.cl1),1,50);
   gtk_clist_set_column_width(GTK_CLIST(self.cl1),2,30);
   gtk_clist_set_column_width(GTK_CLIST(self.cl1),3,0);
   gtk_clist_set_column_width(GTK_CLIST(self.cl1),4,0);
   gtk_clist_set_column_visibility(GTK_CLIST(self.cl1),3,0);
   gtk_clist_set_column_visibility(GTK_CLIST(self.cl1),4,0);
   gtk_container_add (GTK_CONTAINER (self.sw2), self.cl1);
   self.sortby_last=-1;
   gtk_signal_connect (GTK_OBJECT(self.cl1), "select_row", 
      (GtkSignalFunc) on_subj_sel, NULL);
   gtk_signal_connect (GTK_OBJECT(self.cl1), "click_column", 
      (GtkSignalFunc) on_col_click, NULL);

   // mst text
   self.mt=gtk_text_new(NULL,NULL);
   gtk_text_set_editable(GTK_TEXT(self.mt),FALSE);
   gtk_text_set_word_wrap(GTK_TEXT(self.mt),TRUE);
   gtk_text_set_line_wrap(GTK_TEXT(self.mt),TRUE);
     gtk_box_pack_start (GTK_BOX (self.hb3), self.mt, TRUE, TRUE, 0);
   self.mtsb=gtk_vscrollbar_new(GTK_TEXT(self.mt)->vadj);
     gtk_box_pack_start (GTK_BOX (self.hb3), self.mtsb, FALSE, FALSE, 0);
   gtk_widget_queue_resize(self.mtsb);
   strcpy(self.heb_font_name,"-*-*-*-*-*--7-*-*-*-*-*-iso8859-8");
   self.heb_font=gdk_font_load(self.heb_font_name);
   gtk_style=gtk_widget_get_style(self.mt);
   self.heb_fg=&(gtk_style->fg[0]);
   self.heb_bg=&(gtk_style->bg[0]);
   if (!self.heb_font)
      printf("using default encoding\n");
   else
   {
      printf("supporting %s\n",self.heb_font_name);
      gtk_style->font=self.heb_font;
   }





   gtk_widget_show_all(self.w);
   gtk_idle_add(do_folders,NULL);

   gtk_main();
   gtk_exit(0);
}

