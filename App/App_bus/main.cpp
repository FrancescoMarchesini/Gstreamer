#include <algorithm>
#include <iostream>
#include <signal.h>
#define LOG_MAIN "[LOG_MAIN] "
#include <gst/gst.h>

//loop princpale del programma direttamente da Glib/Gtk+
static GMainLoop *loop;

//funzione di callback per ritornare indietro i messaggi dal bus
static gboolean my_bus_callback(GstBus *bus, GstMessage *message, gpointer data)
{
    g_print("Ho il %s messaggio\n", GST_MESSAGE_TYPE_NAME(message));
    switch(GST_MESSAGE_TYPE(message)){
       case GST_MESSAGE_ERROR:{
            GError *err;
            gchar *debug;

            //parso il messaggio di errore
            gst_message_parse_error(message, &err, &debug);
            //faccio il print su console del messaggio di errore
            g_print("Errore: %s\n", err->message);
            //libero il puntatore dell'oggetto errore
            g_error_free(err);
            //libero il char del debug
            g_free(debug);

            //esco dal mainloop
            g_main_loop_quit(loop);
            break;
        }
       case GST_MESSAGE_EOS: //End Of Stream = EOS :)
        g_main_loop_quit(loop);
        break;
        default:
        /*qualcosa rigurdo a qualcosa*/
        break;
    }

    //ritornando TRUE sto in ascolto di nuovi messaggi se Fosse FALSE invece si il processo di callback si
    //fermerebbe e niente piu message handler
    return TRUE;
}

//utilizzo di GOption
gint main(gint argc, gchar* argv[])
{
     //dichiaro elemento pipeline
     GstElement *pipeline;
     //dichiaro oggetto bus
     GstBus *bus;
     //varibile che rappresenta id del bus della pipeline
     guint bus_watch_id;

     /*inizializzillo gstream*/
     gst_init(&argc, &argv);

     /*creo la pipeline e l'handler dei messaggi*/
     pipeline = gst_pipeline_new("la mia nuova bellissima pipeline");

     /*prendo dalla pipeline il relativo bus*/
     bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
     //do alla mia funzione di callback il bus in modo che possa guardare i nuovi messaggi
     bus_watch_id = gst_bus_add_watch(bus, my_bus_callback, NULL);
     //libero il bus
     gst_object_unref(bus);

     /*creo il mainloop che iterea ul glib context. il contesto fa il check se ci soono nuovi messaggi
      * nel caso affermativo chiama la funzione handler e stampa il messaggio. Ciò fino a quando non
      * si esce dal loop*/
     loop = g_main_loop_new(NULL, FALSE);
     g_main_loop_run(loop);

     /*pulisco tutto*/
     gst_element_set_state(pipeline, GST_STATE_NULL);
     gst_object_unref(pipeline);
     g_source_remove(bus_watch_id);
     g_main_loop_unref(loop);
     return 0;
}

