#include <gst/gst.h>

int main(int argc, char *argv[])
{
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    /*init gst*/
    //inizializzzazione di tutte le strutture interne
    //check dei plugin diponibili
    //esecuzione degli aromenti passati da riga di comando
    gst_init(&argc, &argv);

    /*costruisco la pipeline*/
    //costruzione della pipeline di gstream in automatico
    //playbin elemento che gestiosci l'intera pipeline
    //https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm
    pipeline = gst_parse_launch("playbin uri=", NULL);

    /*faccio play*/
    //stati di play / pause / etc...
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /*aspetto la fine del video per chiuder*/
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /*libero ler risorse*/
    if(msg!=NULL)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}

