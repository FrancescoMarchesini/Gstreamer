#include <gst/gst.h>

typedef struct _costumData{
    GstElement *pipeline;
    GstElement *source;             //fonte dati
    GstElement *demuxer;            //splitaggio sorgenti audio e video
    GstElement *conv_audio;         //convert audio
    GstElement *out_audio;          //uscita audio
    GstElement *conv_video;         //converter video
    GstElement *out_video;          //uscita video
    GstElement *caps;               //format video
    GstCaps *video_caps;
}costumData;

static void pad_added_handler(GstElement *src, GstPad *new_pad, costumData *data);

int main(int argc, char *argv[])
{
    costumData data;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    gboolean terminate = FALSE;

    /*init gst*/
    //inizializzzazione di tutte le strutture interne
    //check dei plugin diponibili
    //esecuzione degli aromenti passati da riga di comando
    gst_init(&argc, &argv);

    /*creo gli elementi*/
    //args1: elemento che si vuole creare
    //args2: nome che si vuole dare all'istanza creata, se metto NULL gstream ne dara uno univoco
    //videotestsrc: genera dei dati a scopi di debug
    //autovideosink: consuma dei dati, ovvero visualizza a video i dati generati da source, la scelta
    //               del player è fatta in automatico da gstreamer
    data.source      = gst_element_factory_make("uridecodebin",      "source"); //uridecodebin will internally instantiate all the necessary elements (sources, demuxers and decoders) to turn a URI into raw audio and/or video streams.
    data.demuxer     = gst_element_factory_make("oggdemux",          "ogg-demuxer");
    data.conv_audio  = gst_element_factory_make("audioconvert",      "converter_audio");
    data.out_audio   = gst_element_factory_make("autoaudiosink",     "output_audio");
    data.conv_video  = gst_element_factory_make("nvvidconv",         "converter_video");
    data.out_video   = gst_element_factory_make("autovideosink",     "output_video");
    data.caps        = gst_element_factory_make("capsfilter",        "caps");

    /*creo un pipeline vuota*/
    data.pipeline = gst_pipeline_new("test-pipeline");

    if(!data.pipeline || !data.source || !data.demuxer || !data.conv_audio || !data.conv_video || !data.out_audio || !data.out_video)
    {
        g_printerr("faliito a creare gli elementi\n");
        return -1;
    }

    /*costruisco la pipeline*/
    //aggiungo tutti gli elmenti creati al contenitori BIN, il quale è un tipo di pipeline
   // gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.demuxer, data.conv_audio, data.out_audio, data.conv_video, data.caps, data.out_video, NULL);
     gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.conv_video, data.caps, data.out_video, NULL);
    //link degli elementi all'interno della pipeline
    if(gst_element_link_many(data.conv_video, data.caps, data.out_video, NULL) != TRUE)// ||
      // gst_element_link_many(data.conv_audio, data.out_audio, NULL) != TRUE)
    {
        g_printerr("Non sono riuscito a linkare gli elementi video\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    data.video_caps = gst_caps_from_string("video/x-raw(memory:NVMM), format=(string)NV12");
    g_object_set (G_OBJECT (data.caps), "caps", data.video_caps, NULL);
    gst_caps_unref(data.video_caps);

    /*modifico le proprietà della sorgente*/
    //per leggere le proprietà g_object_get();
    g_object_set(data.source, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);
    //caps per il videoconverter

    /* Connect to the pad-added signal */
    //questa è la funzione di callback per ricevere i messaggi
    g_signal_connect(data.source, "pad_added", G_CALLBACK(pad_added_handler), &data);

    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Fallito a fare play");
        gst_object_unref(data.pipeline);
        return -1;
    }

    /*aspetto la fine del video per chiuder*/
    bus = gst_element_get_bus(data.pipeline);
    do{
         msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /*parsing dei messaggi*/
    if(msg!=NULL)
    {
        GError *err;
        gchar *debug_info;

        switch(GST_MESSAGE_TYPE(msg))
        {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Errore nella ricezione dell'elemento %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("inforamzioni di debug: %s\n", debug_info ? debug_info: "none");
                g_clear_error(&err);
                g_free(debug_info);
                terminate = TRUE;
                break;
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream ricevuta\n");
                terminate = TRUE;
                break;
            case GST_MESSAGE_STATE_CHANGED:
                 /* We are only interested in state-changed messages from the pipeline */
                 if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
                   GstState old_state, new_state, pending_state;
                   gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                   g_print ("Stato della pipeline cambiato da %s a %s:\n",
                     gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
                 }
                 break;
            default:
                g_print("Messaggi insaspettati ricevuti\n");
                break;
        }
        gst_message_unref(msg);
        }
    }while(!terminate);


    /*libero ler risorse*/
    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    return 0;
}

/*tutte le volte che arriva un messaggio al pad viene eseguita questa funzione*/
static void pad_added_handler(GstElement *src, GstPad *new_pad, costumData *data)
{
    GstPad *sink_pad = gst_element_get_static_pad(data->conv_audio, "sink");
    GstPadLinkReturn ret;

    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_print ("Ricevuto nuovo pad '%s' da '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));
    if (gst_pad_is_linked (sink_pad)) {
      g_print ("Il pad e' gia stato linkato ignoro.\n");
      goto exit;
    }

    /*analizizzo il nuovo pad*/
    new_pad_caps = gst_pad_get_current_caps (new_pad);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);
    if (!g_str_has_prefix (new_pad_type, "audio/x-raw")) {
      g_print ("il source pad caps e' '%s' file audio/x-raw, ignoro\n", new_pad_type);
        goto exit;
    }

    /* Attempt the link */
    ret = gst_pad_link (new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
       g_print ("Tipo '%s' ma link fallito.\n", new_pad_type);
    } else {
      g_print ("Link ok (tipo '%s').\n", new_pad_type);
    }

    exit:
    /* Unreference the new pad's caps, if we got them */
        if (new_pad_caps != NULL)
        gst_caps_unref (new_pad_caps);

        /* Unreference the sink pad */
        gst_object_unref (sink_pad);

}
