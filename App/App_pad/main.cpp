#include <algorithm>
#include <iostream>
#include <signal.h>
#define LOG_MAIN "[LOG_MAIN] "
#include <gst/gst.h>

//creazione di un pad
static void cb_new_pad(GstElement * element, GstPad *pad, gpointer data)
{
    //nome del pad
    gchar *name;
    name = gst_pad_get_name(pad);
    g_print("il nome del pad creato è : %s\n", name);
    g_free(name);

    //qui c'è il setup del nuovo apd e il link per l'elemento creato
}

//creazione un request pad
static void cb_new_pad_request(GstElement *tee)
{
    GstPad* pad;
    gchar* name;

    //faccio una all'elemento tee per avere il suo pad
    pad = gst_element_get_request_pad(tee, "src%d");
    name = gst_pad_get_name(pad);
    g_print("Un nuovo pad %s è stato creato\n", pad);
    g_free(name);

    //rialscio l'oggetto dopo avrelo usato
    gst_object_unref(GST_OBJECT(pad));
}

//richesta di un pad compatibile all'elent alquanle si vuole linkare
static void link_to_multiplayer(GstPad *tolink_pad, GstElement *mux )
{
    //inistazione oggetto pad
    GstPad *pad;
    //char per avere i nomi dei pad source(destra) and sink(sinistra)
    gchar *srcname, *sinkname;

    //get name of sorgenete pad
    srcname = gst_pad_get_name(tolink_pad);

    //chiedo a gst di darmi un pad copatibileall'elemento mux
    pad = gst_element_get_compatible_pad(mux, tolink_pad, NULL);

    //creo il link tra pad ed elemetno
    gst_pad_link(tolink_pad, pad);

    //get the name of pad sink
    sinkname = gst_pad_get_name(pad);

    gst_object_unref(GST_OBJECT(pad));

    g_print("Un nuovo pad è stato creato %s e linkato a %s\n",  sinkname, srcname);
    g_free(sinkname);
    g_free(srcname);
}

//estrazione delle informazioni width and height
static void read_video_props (GstCaps *caps)
{
  gint width, height;
  //mi da la struttura del camps ovvero una struttura dati , simil array, che mi da
  //le vaire inforamzioni dell'elelemneto in questione
  const GstStructure *str;

  g_return_if_fail (gst_caps_is_fixed (caps));

  str = gst_caps_get_structure (caps, 0);
  if (!gst_structure_get_int (str, "width", &width) ||
      !gst_structure_get_int (str, "height", &height)) {
    g_print ("No width/height available\n");
    return;
  }

  g_print ("video size  %dx%d\n", width, height);
}

//attraverso questo cap facciamo in modo i dati fluiscana tra l'elemento 1 e il 2 forzandolo a cambiare
//dimensione framerate e formato
static gboolean link_elements_with_filter (GstElement *element1, GstElement *element2)
{
  gboolean link_ok;
  GstCaps *caps;

  //determino come i dati deveo essere trasmutati per passare dall'elemnto 1 all'elemento due
  /*caps = gst_caps_new_simple ("video/x-raw",
          "format", G_TYPE_STRING, "I420",
          "width", G_TYPE_INT, 384,
          "height", G_TYPE_INT, 288,
          "framerate", GST_TYPE_FRACTION, 25, 1,
          NULL);
  */
  //sempio puu complesso
  caps = gst_caps_new_full (
       gst_structure_new ("video/x-raw",
              "width", G_TYPE_INT, 384,
              "height", G_TYPE_INT, 288,
              "framerate", GST_TYPE_FRACTION, 25, 1,
              NULL),
       gst_structure_new ("video/x-bayer",
              "width", G_TYPE_INT, 384,
              "height", G_TYPE_INT, 288,
              "framerate", GST_TYPE_FRACTION, 25, 1,
              NULL),
       NULL);

  link_ok = gst_element_link_filtered (element1, element2, caps);
  gst_caps_unref (caps);

  if (!link_ok) {
    g_warning ("Fallito a creare l'elemento!");
  }

  return link_ok;
}


//utilizzo di GOption
gint main(gint argc, gchar* argv[])
{
     //creazione degli elementi
    //demux elemetno per splittare una sorgete multimediale nelle sue componenti ed
    //inviare ognuna di queste al proprio encoder
     GstElement *pipeline, *source, *demux;

     //main loop dell'applicazione
     GMainLoop *loop;

     /*inizializzillo gstream*/
     gst_init(&argc, &argv);

     /*creo la pipeline e i vari elementi*/
     pipeline = gst_pipeline_new("la mia nuova bellissima pipeline");
     source = gst_element_factory_make("filesrc", "source");
     g_object_set(source, "location", argv[1], NULL);
     demux = gst_element_factory_make("oggdemux", "demuxer");

     //check se gli elementi sono stati creati corretamnte
     if(!pipeline | !source | !demux){
         g_print("fallito a creare almento un elemento");
         return 0;
     }

     //aggiungo tutto alla pipeline
     gst_bin_add_many(GST_BIN(pipeline), source, demux, NULL);
     //collego gli elementi: source in modalità src mentre demux in modalità sink
     gst_element_link_pads(source, "src", demux, "sink");

     //sto in acolto nella creazione del nuovo pad
     g_signal_connect(demux, "pad-added", G_CALLBACK(cb_new_pad), NULL);

     /*creo il mainloop che iterea ul glib context. il contesto fa il check se ci soono nuovi messaggi
      * nel caso affermativo chiama la funzione handler e stampa il messaggio. Ciò fino a quando non
      * si esce dal loop*/
     gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);
     loop = g_main_loop_new(NULL, FALSE);
     g_main_loop_run(loop);

     /*pulisco tutto*/
     g_main_loop_unref(loop);
     return 0;
}

