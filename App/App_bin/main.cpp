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
    //------------------------------------------------------------------
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
     //----------------------------------------------------------------------

     //dichiaro gli elementi
     GstElement *bin, *pipeline, *source, *sink;

     //init di gst
     gst_init(&argc, &argv);

     //creo gli elementi
     pipeline = gst_pipeline_new("nuova pipeline");
     bin = gst_bin_new("my_bin");
     source = gst_element_factory_make("fakesrc", "source");
     sink = gst_element_factory_make("fakesrc", "sink");

     if( !pipeline | !bin | !source | !sink )
     {
            g_print("Fallito a creare almeno uno degli elementi\n");
            return -1;
     }

     //aggiungo gli elemento creati al bin
     gst_bin_add_many(GST_BIN(bin), source, sink, NULL);

     //aggiungo il bin alla pipeline
     gst_bin_add(GST_BIN(pipeline), bin);

     //link gli elementi
     gst_element_link(source, sink);


     return 0;
}

