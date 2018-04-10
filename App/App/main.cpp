#include <algorithm>
//#include "glDisplay.h"
//#include "glTexture.h"
#include <iostream>
#include <signal.h>


#define LOG_MAIN "[LOG_MAIN] "

#include <gst/gst.h>

//utilizzo di GOption
int main(int argc, char* argv[])
{
    gboolean silent = FALSE;
    gchar *savefile =  NULL;
    GOptionContext *ctx; //server per parsare da linea di comando
    GError *err = NULL;
    GOptionEntry enteries[] = {
        {"silent", 's', 0, G_OPTION_ARG_NONE, &silent,
        "non printo un cazzo perchè sono silent"},
        {"output", 'o', 0, G_OPTION_ARG_STRING, &savefile,
        "salvo un xml che rappresenta la pipeline ed esco", "FILE"},
        {NULL}
    };

    ctx = g_option_context_new("myGstGlibFuckinApp");
    g_option_context_add_main_entries(ctx, enteries, NULL);
    g_option_context_add_group(ctx, gst_init_get_option_group());
    if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
       g_print ("Fallito ad inizializzare il todos: %s\n", err->message);
       g_clear_error (&err);
       g_option_context_free (ctx);
       return 1;
     }
     g_option_context_free (ctx);

     printf ("Eseguimi con --help per vedere le opzioni dell'app.\n");

     return 0;
}

