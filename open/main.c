/*
This file is licensed under the MIT-License
Copyright (c) 2015 Marius Messerschmidt
For more details view file 'LICENSE'
*/

#include <side/config.h>
#include <side/apps.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <gtk/gtk.h>

//GUI

GtkTreeIter iter;
GtkListStore *list;

void choose_new_app(char *mime)
{
  list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

  side_apps_load();
  AppEntry e;
  puts("Possible Apps:");
  do
  {
    e = (AppEntry) side_apps_get_next_entry();
    if(strstr(e.mime_types, mime) != NULL)
    {
      printf(" :: %s\n", e.app_name);
      gtk_list_store_append(GTK_LIST_STORE(list), &iter);
      gtk_list_store_set(GTK_LIST_STORE(list), &iter, 0, e.app_name, 1, e.exec, -1);
    }
  }while(e.valid == true);
  side_apps_close();

  GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win), "SiDE Open...");
  gtk_window_resize(GTK_WINDOW(win), 500, 400);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_container_add(GTK_CONTAINER(win), box);

  GtkWidget *text = gtk_label_new("Select an app for the current content:");
  gtk_box_pack_start(GTK_BOX(box), text, FALSE, FALSE, 0);

  GtkWidget *tree = gtk_tree_view_new();
  GtkWidget *scroll_win = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_win), 200);
  gtk_container_add(GTK_CONTAINER(scroll_win), tree);

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("App", renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

  gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(list));
  gtk_tree_view_expand_all(GTK_TREE_VIEW(tree));
  gtk_box_pack_start(GTK_BOX(box), scroll_win, TRUE, TRUE, 0);

  gtk_widget_show_all(win);
  gtk_main();
}



// GUI



char *get_mime_type(char *file)
{
  char *ct = g_content_type_guess(file, NULL, 0, NULL);
  char *mime = g_content_type_get_mime_type(ct);
  g_free(ct);
  return mime;
}

#define MIMEFALLBACK "/etc/side/mime.conf"

int main(int argc, char **argv)
{
  gtk_init(&argc, &argv);
  char MIMEDB[2000];
  memset(MIMEDB, 0, 2000);
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;

  strcat(MIMEDB, homedir);
  strcat(MIMEDB, "/.config/side/mime.conf");

  if(access(MIMEDB, F_OK ) == -1 )
  {
    fprintf(stderr, "Can't find local configuration {%s} using system fallback {%s}\n", MIMEDB, MIMEFALLBACK);
    memset(MIMEDB, 0, 2000);
    strcat(MIMEDB, MIMEFALLBACK);
  }

  for (int x = 1; x < argc; x++)
  {
    char *mime_type = get_mime_type(argv[x]);


    char *app = side_lookup_value(MIMEDB, mime_type);
    char *subtype;

    if(!app)
    {
      choose_new_app(mime_type);
      fprintf(stderr, "can't find full match for type %s\n", mime_type);
      subtype = strtok(mime_type, "/");
      app = side_lookup_value(MIMEDB, subtype);
    }

    if(!app)
    {
      fprintf(stderr, "Unable to open MIME type %s or subtype %s\n",mime_type, subtype);
      g_free(mime_type);
      continue;
    }

    char *op = malloc(strlen(app) + strlen(argv[x]) +1);
    memset(op, 0, strlen(app) + strlen(argv[x]) +1);
    strcat(op, app);
    strcat(op, " \"");
    strcat(op, argv[x]);
    strcat(op, "\" &");
    system(op);
    free(op);

    g_free(mime_type);

  }
}