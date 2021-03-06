/*
This file is licensed under the MIT-License
Copyright (c) 2015 Marius Messerschmidt
For more details view file 'LICENSE'
*/

#include <jetspace/configkit.h>

#include <side/apps.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>

char MIMEDB[2000];
#define MIMEFALLBACK "/etc/side/mime.conf"

bool found_new_app = false;

//GUI

GtkTreeIter iter;
GtkListStore *list;
GtkWidget *tree;

gboolean use_app (GtkWidget *w, GdkEvent *e, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));

  char *exec;

  if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), &model, &iter))
  {
    gtk_tree_model_get(model, &iter, 1, &exec, -1);
    jet_set_value(MIMEDB, (char *)data, exec, true);
    gtk_widget_destroy(GTK_WIDGET(gtk_widget_get_parent_window(w)));
    gtk_main_quit();
    found_new_app = true;
  }
  return FALSE;
}

gboolean destroy(GtkWidget *w, GdkEvent *e, gpointer data)
{
  gtk_main_quit();
  return FALSE;
}


void choose_new_app(char *mime)
{
  list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  found_new_app = false;
  side_apps_load();
  AppEntry e;
  printf(_("Possible Apps for type '%s':\n"), mime);
  do
  {
      e = side_apps_get_next_entry();
      if(strstr(e.mime_types, mime) == 0)
        continue;
      printf(" :: %s\n", e.app_name);
      gtk_list_store_append(GTK_LIST_STORE(list), &iter);
      gtk_list_store_set(GTK_LIST_STORE(list), &iter, 0, e.app_name, 1, e.exec, -1);

  }while(e.valid == true);
  side_apps_close();

  GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win), _("SiDE Open..."));
  gtk_window_resize(GTK_WINDOW(win), 500, 400);
  g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(destroy), NULL);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_container_set_border_width(GTK_CONTAINER(box), 12);
  gtk_container_add(GTK_CONTAINER(win), box);

  GtkWidget *text = gtk_label_new(_("Select an app for the current Content-Type.\nIf you close this window, SiDE will try to find a fallback Application for this file."));
  GtkWidget *tp = gtk_label_new("");
  char *tp_txt = g_strdup_printf(_("<b>Content-Type: %s</b>"), mime);
  gtk_label_set_markup(GTK_LABEL(tp), tp_txt);
  g_free(tp_txt);
  gtk_box_pack_start(GTK_BOX(box), tp, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(box), text, FALSE, FALSE, 2);

  tree = gtk_tree_view_new();
  GtkWidget *scroll_win = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_win), 200);
  gtk_container_add(GTK_CONTAINER(scroll_win), tree);

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(_("App"), renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

  gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(list));
  gtk_tree_view_expand_all(GTK_TREE_VIEW(tree));
  gtk_box_pack_start(GTK_BOX(box), scroll_win, TRUE, TRUE, 0);

  GtkWidget *button = gtk_button_new_with_label(_("Select"));
  gtk_box_pack_end(GTK_BOX(box), button, FALSE, FALSE, 0);

  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(use_app), g_strdup(mime));

  gtk_widget_show_all(win);
  gtk_main();
}



// GUI



char *get_mime_type(char *file)
{
  GFile *gf = g_file_new_for_path(file);
  GFileInfo *info;
  info = g_file_query_info(gf, "standard::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
  char *content_type = g_strdup(g_file_info_get_content_type(info));

  return content_type;
}



int main(int argc, char **argv)
{
  textdomain("side");
  gtk_init(&argc, &argv);
  memset(MIMEDB, 0, 2000);
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;

  strcat(MIMEDB, homedir);
  strcat(MIMEDB, "/.config/side/mime.conf");

  if(access(MIMEDB, F_OK ) == -1 )
  {
    fprintf(stderr, _("Can't find local configuration {%s} using system fallback {%s}\n"), MIMEDB, MIMEFALLBACK);
    memset(MIMEDB, 0, 2000);
    strcat(MIMEDB, MIMEFALLBACK);
  }
  bool select = false;
  for (int x = 1; x < argc; x++)
  {
    if(strcmp(argv[x], "--select") == 0)
      {
        select = true;
        continue;
      }

    if(strcmp(argv[x], "--choose") == 0)
    {
      if(argc <= x)
        continue;
      gboolean select_old = select;
      select = true;
      choose_new_app(argv[x+1]);
      select = select_old;
      x++;
      continue;
    }

    char *mime_type;
    if(strncmp(argv[x], "http://", 7) == 0)
    {
      mime_type = g_strdup("x-scheme-handler/http");
    }
    else if (strncmp(argv[x], "https://", 8) == 0)
    {
      mime_type = g_strdup("x-scheme-handler/https");
    }
    else
      mime_type = get_mime_type(argv[x]);

    if(select)
    {
      choose_new_app(mime_type);
    }

    char *app = jet_lookup_value(MIMEDB, mime_type);
    char *subtype;

    if(!app)
    {
      if(!select)
        choose_new_app(mime_type);
      if(found_new_app && !select)
        {
          x--;
          continue;
        }

      fprintf(stderr, _("can't find full match for type %s\n"), mime_type);
      subtype = strtok(mime_type, "/");
      app = jet_lookup_value(MIMEDB, subtype);
    }

    if(!app)
    {
      fprintf(stderr, _("Unable to open MIME type %s or subtype %s\n"),mime_type, subtype);
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

    g_free(mime_type);
    select = false;
  }
}
