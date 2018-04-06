#include <gst/gst.h>
#include <iostream>

typedef struct _costumData
{
    GstElement *pipeline;        /*elemento che gestira il dotos*/
    GstElement *audio_source, *tee, *audio_queue, *audio_convert, *audio_resample, *audio_sink;
    GstElement *video_queue, *visual, *video_convert, *video_sink;

    GstElement *caps;
    GstCaps *video_caps;

    GstPad *tee_audio_pad, *tee_video_pad;
    GstPad *queue_audio_pad, *queue_video_pad;

    gint n_video;               /*numero di stream video nel file*/
    gint n_audio;               /*numero di stream audio nel file*/
    gint n_text;                /*numero di stream testi nel file*/

    gint current_video;         /*video corrente in play*/
    gint current_audio;         /*audio corrente in play*/
    gint current_text;          /*testo corrente in play*/

    GMainLoop *mainLoop;        /*Glib main looop*/
}CustomData;

/*flags del playbin*/
typedef enum {
    GST_PLAY_FLAG_VIDEO         = (1 << 0),     /*output video desiderato*/
    GST_PLAY_FLAG_AUDIO         = (1 << 1),     /*output audio desiderato*/
    GST_PLAY_FLAG_TEXT          = (1 << 2)      /*output testo desiderato*/
}GstPlayFlag;


/*dichairazione delle funzione per gestire i messaggi gi gstream e input da tastiera*/
static gboolean handle_message(GstBus *bus, GstMessage *msg, CustomData *data);
static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond, CustomData *data);

int main(int argc, char *argv[])
{
    CustomData data;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    gint flags;
    GIOChannel *io_stdin;


    /*init gst*/
    //inizializzzazione di tutte le strutture interne
    //check dei plugin diponibili
    //esecuzione degli aromenti passati da riga di comando
    gst_init(&argc, &argv);

    /*creo gli elementi*/
    data.audio_source   =         gst_element_factory_make("audiotestsrc", "audio_source");
    data.tee            =         gst_element_factory_make("tee", "tee");
    /*elementi audio*/
    data.audio_queue    =         gst_element_factory_make("queue", "audio_queue");
    data.audio_convert  =         gst_element_factory_make("audioconvert", "audio_convert");
    data.audio_resample =         gst_element_factory_make("audioresample", "audio_resample");
    data.audio_sink     =         gst_element_factory_make("autoaudiosink", "audio_sink");
    /*elementi video*/
    data.video_queue    =         gst_element_factory_make("queue", "video_queue");
    data.visual         =         gst_element_factory_make("wavescope", "visual");
    data.video_convert  =         gst_element_factory_make("nvvidconv", "csp");         //come video converter uilizzare sempre quello di nvidia altrimenti non funziona un cazzo !!!!!!!
    data.caps           =         gst_element_factory_make("capsfilter", "caps");
    data.video_sink     =         gst_element_factory_make("autovideosink", "video_sink");

    /*creo l a pipeline*/
    data.pipeline = gst_pipeline_new("test-pipeline");
    if( !data.pipeline || !data.audio_source || !data.tee || !data.audio_queue || !data.audio_convert || !data.audio_resample || !data.audio_sink ||
            !data.video_queue || !data.visual ||!data.video_convert || !data.video_sink)
    {
        g_printerr("fallito a creare gli elementi\n");
        return -1;
    }

    /*configure gli elementi*/
    g_object_set(data.audio_source, "freq", 115.0, NULL);       //la sorgente audio è un semplice tono
    g_object_set(data.visual, "shader", 3, "style", 1, NULL);   //il video è uno shader della frequenza audio

    /*aggiungo tutti gli elementi alla pipeline*/
    gst_bin_add_many(GST_BIN(data.pipeline), data.audio_source, data.tee, data.audio_queue, data.audio_convert, data.audio_resample, data.audio_sink,
                      data.video_queue, data.visual, data.video_convert, data.caps, data.video_sink, NULL);

    /*linko gli elementi che hanno il pad always*/
    if(gst_element_link_many(data.audio_source, data.tee, NULL) != TRUE ||
       gst_element_link_many(data.audio_queue, data.audio_convert, data.audio_resample, data.audio_sink, NULL) != TRUE ||
       gst_element_link_many(data.video_queue, data.visual, data.video_convert, data.caps, data.video_sink, NULL) != TRUE)
    {
        g_printerr("fallito a fare il link dei pad always\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    //aggiungo il caps per il video convert nvidia
    data.video_caps = gst_caps_from_string("video/x-raw(memory:NVMM), format=(string)NV12");
    g_object_set(G_OBJECT(data.caps), "caps", data.video_caps, NULL);

    /*faccio il link manuale degli elementi che hanno il pad request*/
    /*audio*/
    data.tee_audio_pad = gst_element_get_request_pad(data.tee, "src_%u");
    g_print("ottenuta richiesta per il pad %s per il ramo audio della pipeline\n", gst_pad_get_name(data.tee_audio_pad));
    data.queue_audio_pad = gst_element_get_static_pad(data.audio_queue, "sink");
    /*video*/
    data.tee_video_pad = gst_element_get_request_pad(data.tee, "src_%u");
    g_print("ottenuta richiesta per il pad %s per il ramo video della pipeline\n", gst_pad_get_name(data.tee_video_pad));
    data.queue_video_pad = gst_element_get_static_pad(data.video_queue, "sink");
    /*check se tutto è ok*/
    if(gst_pad_link(data.tee_audio_pad, data.queue_audio_pad) != GST_PAD_LINK_OK ||
       gst_pad_link(data.tee_video_pad, data.queue_video_pad) != GST_PAD_LINK_OK)
    {
        g_printerr("L'elemento tee non puo essere linkato\n");
        gst_object_unref(data.pipeline);
        return -1;
    }
    gst_object_unref(data.queue_audio_pad);
    gst_object_unref(data.queue_video_pad);

    /*setto il bus per ricevere le informazioni dalla pipeline*/
    /*aspetto la fine del video per chiuder*/
    //bus = gst_element_get_bus(data.pipeline);
   // gst_bus_add_watch(bus, (GstBusFunc)handle_keyboard, &data);

    /*aggiungo la funzione per vedere i tasti premuti da tastiera*/
#ifdef G_OS_WIN32
  io_stdin = g_io_channel_win32_new_fd (fileno (stdin));
#else
  io_stdin = g_io_channel_unix_new (fileno (stdin));
#endif
  g_io_add_watch (io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, &data);

    /*incomincio il play*/
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Fallito a fare play");
        gst_object_unref(data.pipeline);
        return -1;
    }

    bus = gst_element_get_bus(data.pipeline);
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /*setto il gmainloop to run*/
    //data.mainLoop = g_main_loop_new(NULL, FALSE);
    //g_main_loop_run(data.mainLoop);

    gst_element_release_request_pad(data.tee, data.tee_audio_pad);
    gst_element_release_request_pad(data.tee, data.tee_video_pad);
    gst_object_unref(data.tee_audio_pad);
    gst_object_unref(data.tee_video_pad);

    /*libero ler risorse*/
    //g_main_loop_unref(data.mainLoop);
   // g_io_channel_unref(io_stdin);
    if(msg != NULL)
        gst_object_unref(bus);
    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    return 0;
}

/*funzione per estrarre inforamzioni dagli stream e print su console*/
static void analyze_streams(struct _costumData *data)
{
    gint i;
    GstTagList *tags;
    gchar *str;
    guint rate;

    /*leggo le proprieta delgli streams*/
    g_object_get(data->pipeline, "n-video", &data->n_video, NULL);
    g_object_get(data->pipeline, "n-audio", &data->n_audio, NULL);
    g_object_get(data->pipeline, "n-text",  &data->n_text, NULL);
    g_print("%d video stream(s), %d audio stream(s), %d text stream(s)\n", data->n_video, data->n_audio, data->n_text);

    /*in base al numero di elementi vao a prendere i tag di video*/
    g_print("\n");
    for(i=0; i< data->n_video; i++)
    {
        tags = NULL;
        /*prendo le informazioni dal tag*/
        g_signal_emit_by_name(data->pipeline, "get-video-tags", i, &tags);
        if(tags)
        {
            g_print("video stream %d:\n", i);
            gst_tag_list_get_string(tags, GST_TAG_VIDEO_CODEC, &str);
            g_print("   codec: %s\n", str ? str : "sconosciuto");
            g_free(str);
            gst_tag_list_free(tags);
        }
    }

    /*in base al numero di elementi vao a prendere i tag di audio video e test*/
    g_print("\n");
    for(i=0; i< data->n_audio; i++)
    {
        tags = NULL;
        /*prendo le informazioni dal tag*/
        g_signal_emit_by_name(data->pipeline, "get-audio-tags", i, &tags);
        if(tags)
        {
            g_print("audio stream %d:\n", i);
            if(gst_tag_list_get_string(tags, GST_TAG_AUDIO_CODEC, &str))
            {
                g_print("   codec: %s\n", str);
                g_free(str);
            }
            if(gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &str))
            {
                g_print("   lingua %s:\n", str);
                g_free(str);
            }
            if(gst_tag_list_get_uint(tags, GST_TAG_BITRATE, &rate))
            {
                g_print("   bitrate %d:\n\n", rate);
            }
            gst_tag_list_free(tags);
        }
    }

    /*in base al numero di elementi vao a prendere i tag di audio video e test*/
    g_print("\n");
    for(i=0; i< data->n_text; i++)
    {
        tags = NULL;
        g_print("sottotitoli stream %d:\n", i);
        /*prendo le informazioni dal tag*/
        g_signal_emit_by_name(data->pipeline, "get-text-tags", i, &tags);
        if(tags)
        {
            if(gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &str))
            {
                g_print("    lingua %s:\n", str);
                g_free(str);
            }
            gst_tag_list_free(tags);
        }
        else
        {
            g_print ("sottotitolo tag non trovato\n");
        }
    }

    g_object_get(data->pipeline, "current_video", &data->current_video, NULL);
    g_object_get(data->pipeline, "current_audio", &data->current_audio, NULL);
    g_object_get(data->pipeline, "current_text" , &data->current_text, NULL);

    g_print("\n");
    g_print("video %d attualmente in play,  audio stream %d,  text steam %d\n",
            data->current_video, data->current_audio, data->current_text);
    g_print ("Type any number and hit ENTER to select a different audio stream\n");
}

static gboolean handle_message(GstBus *bus, GstMessage *msg, CustomData *data)
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
            g_main_loop_quit (data->mainLoop);
            break;
        case GST_MESSAGE_EOS:
            g_print("End-Of-Stream ricevuta\n");
            g_main_loop_quit (data->mainLoop);
            break;
        case GST_MESSAGE_STATE_CHANGED:
        {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
            if(GST_MESSAGE_SRC(msg) == GST_OBJECT(data->pipeline))
            {
                g_print ("Stato della pipeline cambiato da %s a %s:\n", gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));

                if(new_state = GST_STATE_PLAYING)
                {
                    //quando sono in play analizzo lo stream
                    //analyze_streams(data);
                }
             }
        }break;
    }

    //voglio continuare a ricevere messaggi
    return TRUE;
}


static gboolean handle_keyboard (GIOChannel *source, GIOCondition cond, CustomData *data) {
  gchar *str = NULL;

  if (g_io_channel_read_line (source, &str, NULL, NULL, NULL) == G_IO_STATUS_NORMAL) {
    int index = g_ascii_strtoull (str, NULL, 0);
    if(index >5)
    {
        gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
        gst_object_unref(data->pipeline);
    }
  }
  g_free (str);

  return TRUE;
}
