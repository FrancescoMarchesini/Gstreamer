#include <gst/gst.h>
#include <iostream>

typedef struct _costumData
{
    GstElement *playbin;        /*elemento che gestira il dotos*/

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
    GstStateChangeReturn ret;
    gint flags;
    GIOChannel *io_stdin;


    /*init gst*/
    //inizializzzazione di tutte le strutture interne
    //check dei plugin diponibili
    //esecuzione degli aromenti passati da riga di comando
    gst_init(&argc, &argv);

    /*creo l'elmento playbin*/
    data.playbin = gst_element_factory_make("playbin", "playbin");
    if(!data.playbin)
    {
        g_printerr("Non tutti gli elementi sono stati creati\n");
        return -1;
    }


    /*setto il uri del file da eseguire*/
    g_object_set(data.playbin, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_cropped_multilingual.webm", NULL);
    /*setto gli uri per i sottotitolu ed il font*/
    g_object_set (data.playbin, "suburi", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer_gr.srt", NULL);
    g_object_set (data.playbin, "subtitle-font-desc", "Sans, 18", NULL);

    /*setto il flags per ricevere le informazioni sui file*/
    g_object_get(data.playbin, "flags", &flags, NULL);      //scrivo nell'indirizzo di memoria di flags gli stati tramite la funzione get
    flags |= GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO | GST_PLAY_FLAG_TEXT;     //eanble audio e video rendering
    //flags &= ~GST_PLAY_FLAG_TEXT;                           //ignoro lo stream del testo
    g_object_set(data.playbin, "flags", flags, NULL);

    /*determino la velocita di connesione con il file per la riproduzione*/
    g_object_set(data.playbin, "connection-speed", 56, NULL);

    /*setto il bus per ricevere le informazioni dalla pipeline*/
    /*aspetto la fine del video per chiuder*/
    bus = gst_element_get_bus(data.playbin);
    gst_bus_add_watch(bus, (GstBusFunc)handle_message, &data);

    /*aggiungo la funzione per vedere i tasti premuti da tastiera*/
#ifdef G_OS_WIN32
  io_stdin = g_io_channel_win32_new_fd (fileno (stdin));
#else
  io_stdin = g_io_channel_unix_new (fileno (stdin));
#endif
  g_io_add_watch (io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, &data);

    /*incomincio il play*/
    ret = gst_element_set_state(data.playbin, GST_STATE_PLAYING);
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Fallito a fare play");
        gst_object_unref(data.playbin);
        return -1;
    }

    /*setto il gmainloop to run*/
    data.mainLoop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data.mainLoop);

    /*libero ler risorse*/
    g_main_loop_unref(data.mainLoop);
    g_io_channel_unref(io_stdin);
    gst_object_unref(bus);
    gst_element_set_state(data.playbin, GST_STATE_NULL);
    gst_object_unref(data.playbin);
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
    g_object_get(data->playbin, "n-video", &data->n_video, NULL);
    g_object_get(data->playbin, "n-audio", &data->n_audio, NULL);
    g_object_get(data->playbin, "n-text",  &data->n_text, NULL);
    g_print("%d video stream(s), %d audio stream(s), %d text stream(s)\n", data->n_video, data->n_audio, data->n_text);

    /*in base al numero di elementi vao a prendere i tag di video*/
    g_print("\n");
    for(i=0; i< data->n_video; i++)
    {
        tags = NULL;
        /*prendo le informazioni dal tag*/
        g_signal_emit_by_name(data->playbin, "get-video-tags", i, &tags);
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
        g_signal_emit_by_name(data->playbin, "get-audio-tags", i, &tags);
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
        g_signal_emit_by_name(data->playbin, "get-text-tags", i, &tags);
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

    g_object_get(data->playbin, "current_video", &data->current_video, NULL);
    g_object_get(data->playbin, "current_audio", &data->current_audio, NULL);
    g_object_get(data->playbin, "current_text" , &data->current_text, NULL);

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
            if(GST_MESSAGE_SRC(msg) == GST_OBJECT(data->playbin))
            {
                g_print ("Stato della pipeline cambiato da %s a %s:\n", gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));

                if(new_state = GST_STATE_PLAYING)
                {
                    //quando sono in play analizzo lo stream
                    analyze_streams(data);
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
          g_main_loop_quit (data->mainLoop);
    }
    if (index < 0 || index >= data->n_audio) {
      g_printerr ("numero canale non trovato\n");
    } else {
      /* If the input was a valid audio stream index, set the current audio stream */
      g_print ("Metto l'audio sul canale' %d\n", index);
      g_object_set (data->playbin, "current-audio", index, NULL);
    }
  }
  g_free (str);
  return TRUE;
}
