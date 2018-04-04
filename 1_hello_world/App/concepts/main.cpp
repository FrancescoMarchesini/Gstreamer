#include <gst/gst.h>

int main(int argc, char *argv[])
{
    GstElement *pipeline, *source, *filter, *videoconverter, *testo, *sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

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
    source = gst_element_factory_make("videotestsrc", "source");
    filter = gst_element_factory_make("vertigotv", "vertigo");
    videoconverter = gst_element_factory_make("nvvidconv", "conv"); //This element converts from one color space (e.g. RGB) to another one (e.g. YUV)
    sink = gst_element_factory_make("autovideosink", "sink");

    /*creo un pipeline vuota*/
    pipeline = gst_pipeline_new("test-pipeline");
    if(!pipeline || !source ||!filter || !videoconverter || !sink)
    {
        g_printerr("faliito a creare gli elementi\n");
        return -1;
    }

    testo = gst_element_factory_make("capsfilter", "testo");
    g_assert (testo != NULL); /* should always exist */


    /*costruisco la pipeline*/
    //aggiungo tutti gli elmenti creati al contenitori BIN, il quale è un tipo di pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, filter, videoconverter, testo, sink, NULL);
    //link degli elementi all'interno della pipeline
    if(gst_element_link_many(source, filter, videoconverter, testo, sink, NULL) != TRUE)
    {
        g_printerr("NOn sono riuscito a linkare gli elementi");
        gst_object_unref(pipeline);
        return -1;
    }
    GstCaps *video_caps;
    video_caps = gst_caps_from_string("video/x-raw(memory:NVMM), format=(string)NV12");
    g_object_set (G_OBJECT (testo), "caps", video_caps, NULL);
    gst_caps_unref(video_caps);

    /*modifico le proprietà della sorgente*/
    //per leggere le proprietà g_object_get();
     g_object_set(source, "pattern", 0, NULL);

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Fallito a fare play");
        gst_object_unref(pipeline);
        return -1;
    }

    /*aspetto la fine del video per chiuder*/
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

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
                break;
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream ricevuta\n");
                break;
            default:
                g_print("Messaggi insaspettati ricevuti\n");
                break;
        }
        gst_message_unref(msg);
    }


    /*libero ler risorse*/
    gst_message_unref(msg);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}

