/*
This file is licensed under the MIT-License
Copyright (c) 2015 Marius Messerschmidt
For more details view file 'LICENSE'
*/

#include <gmodule.h>
#include <gtk/gtk.h>

typedef void (*CallPlugin) (GtkWidget *root);

void load_plugins(char *path, GtkWidget *root)
{
  //MODULE VARS
  GModule *module = NULL;
  CallPlugin call = NULL;
  gchar module_path[2000];

  //SCAN VARS
  DIR *d;
  struct dirent *de;

  d = opendir(path);
  if(d == NULL)
    {
      g_error("MODULE PATH NOT FOUND");
      return;
    }


  while((de = readdir(d)) != NULL)
    {
      strcpy(module_path, path);
      strcat(module_path, de->d_name);

      if(g_module_supported() == FALSE)
        {
          g_error("Modules are not available");
        }

      if(strlen(de->d_name) < 4)
        continue; //skip files without .so

      g_print("-----\nModule path: %s\n-----\n", module_path);

      module = g_module_open(module_path, G_MODULE_BIND_LAZY);
      g_module_make_resident(module); //users are able to create callbacks, because the module would not be unloaded...

      if(module == NULL)
        {
          g_warning("Loading Module failed: %s", g_module_error());
        }

      if(g_module_symbol(module, "plugin_call", (gpointer *) &call) == FALSE)
        {
          g_warning("Can't load symbol 'plugin_call': %s", g_module_error());
          continue;
        }


      call(root);


    }

}
