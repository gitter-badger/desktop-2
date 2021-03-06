/*
This file is licensed under the MIT-License
Copyright (c) 2015 Marius Messerschmidt
For more details view file 'LICENSE'
*/
#define _XOPEN_SOURCE 500
#include <glib.h>
#include <gtk/gtk.h>
#include "../shared/info.h"
#include "../shared/strdup.h"
#include "../shared/config.h"
#include <string.h>
#include <stdlib.h>
#include <side/apps.h>
#include <unistd.h>

#ifdef X_WINDOW_SYSTEM
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif //X_WINDOW_SYSTEM


/*
This file contains the startup tool
to run the desktop enviroment you just have to type:

side-session

*/

int SESSION = 0; //will contain the return value -> errors

gboolean panel     = TRUE;
gboolean wallpaper = TRUE;
gboolean wm        = TRUE;
gboolean autostart = TRUE;

#define PANEL "dbus-launch side-panel &"
#define WALLPAPER "dbus-launch side-wallpaper-service &"

void XDG_autostart(void)
{
  //Variables
  int count = 0;
  AppEntry ent;

  side_apps_load_dir("/etc/xdg/autostart/");

  ent.valid = TRUE;

  while(ent.valid == TRUE)
  {
    char *buff;
    ent = side_apps_get_next_entry();
    if(ent.show_in_side == false || ent.hidden == true)
      continue;
    buff = g_strdup_printf("%s &", ent.exec);
    system(buff);
    free(buff);
    count++;
  }
  side_apps_close();

  char *d = g_strdup_printf("%s/.config/autostart/", g_get_home_dir());
  if(!g_file_test(d, G_FILE_TEST_EXISTS))
  {
    g_print("Loaded %d Autostart apps.\n (no autostart from .config)", count);
    return;
  }

  side_apps_load_dir(d);
  ent.valid = TRUE;
  while(ent.valid == TRUE)
  {
    char *buff;
    ent = side_apps_get_next_entry();
    if(ent.show_in_side == false || ent.hidden == true)
      continue;
    buff = g_strdup_printf("%s &", ent.exec);
    system(buff);
    free(buff);
    count++;
  }
  side_apps_close();

  g_print("Loaded %d Autostart apps.\n", count);

}


void autorun(void)
{
  GSettings *s = g_settings_new("org.jetspace.desktop.session");
  char *str    = strdup(g_variant_get_string(g_settings_get_value(s, "autostart"), NULL));

  char *p = strtok(str, ";");

  while(p != NULL)
    {
      char *exec = malloc(strlen(p) + 3);
      strcpy(exec, p);
      strcat(exec, " &");
      g_debug("AUTOSTART: launchings %s\n", exec);
      system(exec);
      free(exec);
      p = strtok(NULL, ";");
    }
  free(p);
  free(str);

}

#ifdef X_WINDOW_SYSTEM
void grab_key(Display *dpy, Window root, int key, unsigned int mod)
{
  unsigned int ig1 = Mod2Mask;
  unsigned int ig2 = LockMask;
  unsigned int ig3 = LockMask | Mod2Mask;
  XGrabKey(dpy, key, mod, root, true, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, key, mod | ig1, root, true, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, key, mod | ig2, root, true, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, key, mod | ig3, root, true, GrabModeAsync, GrabModeAsync);

}
#endif



int main(int argc, char **argv)
{
  GSettings *session = g_settings_new("org.jetspace.desktop.session");

  if(argc > 1)
  {
    if(strcmp(argv[1], "--logout") == 0)
      {
        system("killall side-session");
        return 0;
      }
    if(strcmp(argv[1], "--reboot") == 0)
      {
        system(g_variant_get_string(g_settings_get_value(session, "reboot"), NULL));
        return 0;
      }
    if(strcmp(argv[1], "--shutdown") == 0)
      {
        system(g_variant_get_string(g_settings_get_value(session, "shutdown"), NULL));
        return 0;
      }
    if(strcmp(argv[1], "--restart") == 0)
      {
        system("killall side-panel");
        system("side-panel &");
        system("killall side-wallpaper-service");
        system("side-wallpaper-service &");
        puts("restarted services!");
        exit(0);
      }
  }

  g_print("SIDE-session Version %s loading...\n", VERSION);

  if(wm)
    system(g_variant_get_string(g_settings_get_value(session, "wm"), NULL));

  if(panel)
    system(PANEL);

  if(wallpaper)
    system(WALLPAPER);

  if(autostart)
    autorun();
  if(g_variant_get_boolean(g_settings_get_value(session, "xdg-autostart")))
    XDG_autostart();

  //now go to endless mode, so the session won't fail
  g_print("switching to endless loop...\n");


  #ifdef X_WINDOW_SYSTEM
  Display *dpy = XOpenDisplay(0);
  Window root = DefaultRootWindow(dpy);
  XEvent event;

  grab_key(dpy, root, XKeysymToKeycode(dpy,XK_R) , Mod4Mask);
  grab_key(dpy, root, XKeysymToKeycode(dpy,XK_F2) , Mod1Mask);
  grab_key(dpy, root, XKeysymToKeycode(dpy,XK_F3) , Mod1Mask);

  XSelectInput(dpy, root, KeyPressMask );


  #endif

  while(1)
  {
    #ifdef X_WINDOW_SYSTEM
    XNextEvent(dpy, &event);
    if(event.type == KeyPress)
    {
      switch(event.xbutton.button)
      {
        case 27: // R
        case 68: // F2
          system("side-run &");
        break;

        case 69: // F3
          system("side-search &");
        break;
      }
    }



    #endif



    usleep(100);
  }



  return SESSION;
}
