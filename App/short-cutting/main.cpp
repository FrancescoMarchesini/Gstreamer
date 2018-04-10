#include <gst/audio/audio.h>
#include <gst/gst.h>
#include <iostream>
#include <string.h>
//pkg-config --list-all |grep -i gstreamer |sort

/// appsrc : il seguente elemento serve per immettere dati dall'applicazione nella pipeline di gstreamer
///     *pull : richiede dati dall'applicazione ogni momento che li necessita
///     *push : è l'applicazione che push i dati quando c'è ne è bisogno
/// appsink: il seguente elemento server per riprendere i dati dalla pipeline e mandarli all'applicazione

#define CHUNK_SIZE 1024     /*la quantita di dati in byte che mandiamo in ogni buffer*/
#define SAMPLE_RATE 44100   /*sample che mandiamo ogni secondo*/

typedef struct _costumData
{
    GstElement *pipeline, *app_source, *tee, *audio_queue, *audio_convert1, *audio_resample, *audio_sink;
    GstElement *video_queue, *audio_convert2, *visual, *video_convert, *video_sink;
    GstElement *app_queue, *app_sink;

    //cose di prima che adesso non so se possono servirvire
    gint n_video;               /*numero di stream video nel file*/
    gint n_audio;               /*numero di stream audio nel file*/
    gint n_text;                /*numero di stream testi nel file*/

    gint current_video;         /*video corrente in play*/
    gint current_audio;         /*audio corrente in play*/
    gint current_text;          /*testo corrente in play*/
    ///////////////////////////////////////////////////////////

    guint64 num_sample;         /*numero di sample generati*/
    gfloat a, b, c, d;          /*generazione della forma d'onda*/

    guint sourceid;             /*id per controllare la Gsource*/

    GMainLoop *mainLoop;        /*Glib main looop*/
}CustomData;

/*flags del playbin*/
typedef enum {
    GST_PLAY_FLAG_VIDEO         = (1 << 0),     /*output video desiderato*/
    //GST_PLAY_FLAG_AUDIO         = (1 << 1),     /*output audio desiderato*/
    GST_PLAY_FLAG_TEXT          = (1 << 2)      /*output testo desiderato*/
}GstPlayFlag;

/* Questo metodo è chiamato da GSource nel mainloop, per inserire 1024 byte in appsrc
 * ....
 * ....
 * ....
 */
static gboolean push_data(CustomData *data);

/* quando appsrc necessita di dati, trigghera questa il segnale il quale fa eseguire la
 * funzione per generare nuovi dati. in questo caso mettiamo un handler(gestore) inattivo,
 * cosi che è lo stesso main loop a chimare la funzione...
 */
static void start_feed (GstElement *source, guint size, CustomData *data);

/* quando appsrc ha abbastanza dati triggera questa funzione e ferma l'invio di dati
 * ovverro togliamo il gestore inattivo dal main loop
 */
static void stop_feed(GstElement *source, CustomData *data);

/* appsink a ricevuto il buffer, ovvero il punto in cui i dati possono venire estratti
 * dalla pipeline
 */
static void new_sample(GstElement *sink, CustomData *data);

/* funzione per maneggiare gli errori e messaggi dello stato della pipeline */
static gboolean handle_message(GstBus *bus, GstMessage *msg, CustomData *data);

static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond, CustomData *data);

int main(int argc, char *argv[])
{
    CustomData data;
    GstPad *tee_audio_pad, *tee_video_pad, *tee_app_pad;
    GstPad *queue_audio_pad, *queue_video_pad, *queue_app_pad;
    GstAudioInfo info;
    GstCaps *audio_caps;
    GstCaps *video_caps;
    GstElement *caps; //per non dimenticare
    GstBus *bus;

    //vecchio codice//////////////
    GstMessage *msg;
    GstStateChangeReturn ret;
    gint flags;
    GIOChannel *io_stdin;
    ////////////////////////////

    /* inizializzazione preliminare della struttura */
    memset(&data, 0, sizeof(data));

    /* init gst
     * inizializzzazione di tutte le strutture interne
     * check dei plugin diponibili
     * esecuzione degli argomenti passati da riga di comando
     */
    gst_init(&argc, &argv);

    /*creo gli elementi*/
    data.app_source     =         gst_element_factory_make("appsrc", "audio_source");
    data.tee            =         gst_element_factory_make("tee", "tee");
    /*elementi audio*/
    data.audio_queue    =         gst_element_factory_make("queue", "audio_queue");
    data.audio_convert1 =         gst_element_factory_make("audioconvert", "audio_convert1");
    data.audio_resample =         gst_element_factory_make("audioresample", "audio_resample");
    data.audio_sink     =         gst_element_factory_make("autoaudiosink", "audio_sink");
    /*elementi video*/
    data.video_queue    =         gst_element_factory_make("queue", "video_queue");
    data.audio_convert2 =         gst_element_factory_make("audioconvert", "audio_convert2");
    data.visual         =         gst_element_factory_make("wavescope", "visual");
    data.video_convert  =         gst_element_factory_make("nvvidconv", "csp");         //come video converter uilizzare sempre quello di nvidia altrimenti non funziona un cazzo !!!!!!!
    caps                =         gst_element_factory_make("capsfilter", "caps");
    data.video_sink     =         gst_element_factory_make("autovideosink", "video_sink");
    /*elementi app*/
    data.app_queue      =         gst_element_factory_make ("queue", "app_queue");
    data.app_sink       =         gst_element_factory_make ("appsink", "app_sink");

    /*creo l a pipeline*/
    data.pipeline = gst_pipeline_new("test-pipeline");
    if( !data.pipeline || !data.app_source || !data.tee || !data.audio_queue || !data.audio_convert1 ||
        !data.audio_resample || !data.audio_sink || !data.video_queue || !data.audio_convert2 ||!data.visual ||
        !data.video_convert || !data.video_sink || !data.app_queue || !data.app_sink)
    {
        g_printerr("fallito a creare gli elementi\n");
        return -1;
    }

    /* parametrizzo l'elemento wave scope */
    g_object_set(data.visual, "shader", 3, "style", 1, NULL);   //il video è uno shader della frequenza audio

    /* parametrizzo appsrc ovvero l'immetere i dati nella pipeline */
    gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, NULL);
    audio_caps = gst_audio_info_to_caps(&info);
    g_object_set(data.app_source, "caps", audio_caps, "format", GST_FORMAT_TIME, NULL);

    /* funzioni callback per trigghera le funzioni di need data e enough data */
    g_signal_connect(data.app_source, "need-data", G_CALLBACK(start_feed), &data);
    g_signal_connect(data.app_source, "enough-data", G_CALLBACK(stop_feed), &data);

    /* parametrizzo appsink ovvero l'estrarre i dati dalla pipeline */
    g_object_set(data.app_sink, "emit-signals", TRUE, "caps", audio_caps, NULL);
    g_signal_connect(data.app_sink, "new_sample", G_CALLBACK(new_sample), &data);
    gst_caps_unref(audio_caps);

    /*aggiungo tutti gli elementi alla pipeline*/
    gst_bin_add_many(GST_BIN(data.pipeline), data.app_source, data.tee, data.audio_queue, data.audio_convert1, data.audio_resample, data.audio_sink,
                      data.video_queue, data.audio_convert2, data.visual, data.video_convert, caps, data.video_sink,
                      data.app_queue, data.app_sink, NULL);

    /*linko gli elementi che hanno il pad always*/
    if(gst_element_link_many(data.app_source, data.tee, NULL) != TRUE ||
       gst_element_link_many(data.audio_queue, data.audio_convert1, data.audio_resample, data.audio_sink, NULL) != TRUE ||
       gst_element_link_many(data.video_queue, data.audio_convert2, data.visual, data.video_convert, caps, data.video_sink, NULL) != TRUE ||
       gst_element_link_many (data.app_queue, data.app_sink, NULL) != TRUE)
    {
        g_printerr("fallito a fare il link dei pad always\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    //aggiungo il caps per il video convert nvidia
    video_caps = gst_caps_from_string("video/x-raw(memory:NVMM), format=(string)NV12");
    g_object_set(G_OBJECT(caps), "caps", video_caps, NULL);
    gst_caps_unref(video_caps);

    /*faccio il link manuale degli elementi che hanno il pad request*/
    /*audio*/
    tee_audio_pad = gst_element_get_request_pad(data.tee, "src_%u");
    g_print("ottenuta richiesta per il pad %s per il ramo audio della pipeline\n", gst_pad_get_name(tee_audio_pad));
    queue_audio_pad = gst_element_get_static_pad(data.audio_queue, "sink");
    /*video*/
    tee_video_pad = gst_element_get_request_pad(data.tee, "src_%u");
    g_print("ottenuta richiesta per il pad %s per il ramo video della pipeline\n", gst_pad_get_name(tee_video_pad));
    queue_app_pad = gst_element_get_static_pad(data.app_queue, "sink");
    /*check se tutto è ok*/
    if(gst_pad_link(tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
       gst_pad_link(tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK ||
       gst_pad_link (tee_app_pad, queue_app_pad) != GST_PAD_LINK_OK)
    {
        g_printerr("L'elemento tee non puo essere linkato\n");
        gst_object_unref(data.pipeline);
        return -1;
    }
    gst_object_unref(queue_audio_pad);
    gst_object_unref(queue_video_pad);
    gst_object_unref(queue_app_pad);

    /*setto il bus per ricevere le informazioni dalla pipeline
     *aspetto la fine del video per chiuder
     */
    bus = gst_element_get_bus(data.pipeline);
    gst_bus_add_watch(bus, (GstBusFunc)handle_message, &data);
    gst_bus_add_watch(bus, (GstBusFunc)handle_keyboard, &data);
    gst_object_unref (bus);

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

    /*setto il gmainloop to run*/
    data.mainLoop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data.mainLoop);

    /* rilascio gli elementi */
    gst_element_release_request_pad(data.tee, tee_audio_pad);
    gst_element_release_request_pad(data.tee, tee_video_pad);
    gst_element_release_request_pad(data.tee, tee_app_pad);
    gst_object_unref(tee_audio_pad);
    gst_object_unref(tee_video_pad);
    gst_object_unref(tee_app_pad);

    /*libero ler risorse*/
    //g_main_loop_unref(data.mainLoop);
   // g_io_channel_unref(io_stdin);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    return 0;
}

//speigazione breve : genero i dati per appsrc
static gboolean push_data (CustomData *data)
{
    GstBuffer *buffer;      //buffer di gstream per riceve i chunck di dati
    GstFlowReturn ret;
    int i;
    GstMapInfo map;
    gint16 *raw;
    gint num_samples = CHUNK_SIZE / 2;  /* ogni sample è di 16 bit */
    gfloat freq;

    /* creo un buffer vuoto della grandezza del chunk size
     * un buffer è semplicemente un unita di dati, buffer differenti possono avere grandezze differenti
     * GstBuffer sono astrazione di GstMemory, e un buffer può contenete piu oggetti GstMemory
     * ogni buffer ha un time-stamp e una durata, con i quali in opgni momento di determinato quando e come il
     * il contenuto del buffer deve essere lavorato. Time stamping è molto complesso e delicato......
     */
    buffer = gst_buffer_new_and_alloc(CHUNK_SIZE);

    /* setto il time stamp
     * ogni GST_SECOND generto 441000(SAMPLE_DATA) di dati
     */
    GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(data->num_sample, GST_SECOND, SAMPLE_RATE);
    /* setto la durata del buffer
     * num_samples = CHUNK SIZE / 2 = 16 bit
     * GST_SECOND
     * SAMPLE_RATE 44100
     * quindi ogni buffer dura la meta della sua grandezza per secondo ?????
     */
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(num_samples, GST_SECOND, SAMPLE_RATE);

    /* genero i dati che mi riempino il buffer e che scorreranno dentro la pipeline */
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    raw = (gint16*)map.data;
    data->c += data->d;
    data->d -= data->c / 1000;
    freq = 1100 + 1000 * data->d;
    for(i=0; i < num_samples; i++)
    {
        data->a += data->b;
        data->b -= data->a / freq;
        raw[i] = (gint16)(500 * data->a);
    }
    /* una volta riempieto il buffer mi stacco e quindi unmap */
    gst_buffer_unmap(buffer, &map);
    data->num_sample += num_samples;

    /* NB: qui avviene il push dei dati da una cosa esterna di gstream nella pipeline */
    g_signal_emit_by_name(data->app_source, "push_buffer", buffer, &ret);

    /* libero il buffer una volta che ho fatto quello che doveva fare */
    gst_buffer_unref(buffer);

    if(ret != GST_FLOW_OK)
    {
        /* se qulacosa non segue bene il suo corso fermo tutto*/
        return false;
    }

    return true;
}

//speigazione breve : appsrc chiede i dati
static void start_feed (GstElement *source, guint size, CustomData *data)
{
    if(data->sourceid == 0){
        g_print("Incomincio a immettere dati\n");
        data->sourceid = g_idle_add((GSourceFunc) push_data, data);
    }
}

//spiegazione breve : appsrc ha abbastanza dati
static void stop_feed(GstElement *source, CustomData *data)
{
    if(data->sourceid != 0)
    {
        g_print("fermo l'invio\n");
        g_source_remove(data->sourceid);
        data->sourceid = 0;
    }
}

//spiegazione breve : l'elemto finale della pipeline
static void new_sample(GstElement *sink, CustomData *data)
{
    GstSample *sample;

    /* Prendo il buffer */
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if(sample)
    {
        /* quando riceviamo il buffer stampiamo il carattere, qui avvere la magia */
        g_print("*");
        gst_sample_unref(sample);
    }
}

//funzione per estrarre inforamzioni dagli stream e print su console*/
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

//spiegazione breve: mangiare i messaggi
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
