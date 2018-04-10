#include "oggvorbisplayer.h"

/* bus_call() funzione per bus handler dai vari elementi
 * args 1 : oggetto bus
 * args 2 : messaggio del bus
 * args 3 : puntatore ai dati del bus
 */
inline static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    //instanzio il main loop sul quale verranno richiesti i bus in modo indipendente dai thread di GST
    GMainLoop *loop = (GMainLoop*) data;

    //maneggio i vari casi possibili dei messaggi del bus
    switch(GST_MESSAGE_TYPE(msg)){
        case GST_MESSAGE_EOS : //messaggio End Of Stream EOS
            g_print("Fine dello stream\n");
            //esco dal main loop
            g_main_loop_quit(loop);
        break;

    case GST_MESSAGE_ERROR: {//messaggi di errore dai vari bus
        gchar *debug;
        GError *error;

        //parso i messaggi di errore riempendo direttamte i puntatori apppena istanziati
        gst_message_parse_error(msg, &error, &debug);
        //rilascio la momeria del char debug
        g_free(debug);

        //print dei messaggi
        g_printerr("Orrore: %s\n", error->message);
        g_error_free(error);

        //se c'è un errore all'interno di un elemento esco e chiudo il todos
        g_main_loop_quit(loop);
        break;
       }
    default:
        break;
    }

    //ritorno true in modo da stare in ascolto per tutta la pipeline.
    //se mettissi false, dopo il primo elemento analizzato il buss_call non funzionerebbe più
    return true;
}

/* on_pad_added(): funzione per aggiungere i pad, interfaccie input/ouput dei singoli elementi,ai singoli elemeti in modo dinamico
 * args 1: elemento al quale aggiungere il pad
 * args 2: il pad da aggiungere
 * args 3: puntatore ai dati dell'elemento
 */
inline void on_pad_added(GstElement *element, GstPad *pad, gpointer data)
{
    //sink pad, ovvero interaccia di destra che serve per portare i dati pad source dell'elemento succesivo
    GstPad *sinkpad;
    GstElement *decoder = (GstElement *)data;

    //collego, link, questo pad, con il decoder-vorbis sink pad
    //g_print("pad creato dinamicamente, link demuxer/decoder\n");

    //prendo dalla pipeline l'elemento in questione per far il link
    sinkpad = gst_element_get_static_pad(decoder, "sink");

    //faccio il link tra il pad e questo elemento
    gst_pad_link(pad, sinkpad);

    //rialsco loggetto
    gst_object_unref(sinkpad);

}

//bool OggVorbisPlayer::gstreamInit()
inline bool gstreamInit()
{
    int argc = 0;
    if( ! gst_init_check(&argc, NULL, NULL)){
        g_print("Fallito a fare il gst_init !! \n");
        return false;
    }

    //stampo la version di gstream
    guint ver[] = {0, 0, 0};
    gchar *str= NULL;
    gst_version(&ver[0], &ver[1], &ver[2], &ver[3]);
    if(ver[3] == 1)
        str = "(CSV)";
    else if(ver[3] == 2)
        str = "(Prerelease)";
    else
        str = "";

    g_print("Gstream initializzato versione : %u.%u.%u\n", ver[0], ver[1], ver[2], str);

    return true;
}

OggVorbisPlayer::OggVorbisPlayer()
{
    loop = NULL;
    file = NULL;

    pipeline = NULL;
    source = NULL;
    demuxer = NULL;
    decoder = NULL;
    conv = NULL;
    sink = NULL;


    bus  = NULL;
    bus_watch_id = 0;

    g_print("istazio main loop\n");
    loop = g_main_loop_new(NULL, FALSE);
}

bool OggVorbisPlayer::gstreamPipeLine(char *_file)
{

    pipeline    = gst_pipeline_new("audio-player");
    source      = gst_element_factory_make("filesrc",           "file-source"); //cerco file da filesystem
    demuxer     = gst_element_factory_make("oggdemux",          "ogg-demuxer");
    decoder     = gst_element_factory_make("vorbisdec",         "vorbis-decoder");
    conv        = gst_element_factory_make("audioconvert",      "converter");
    sink        = gst_element_factory_make("autoaudiosink",     "audio-ouput");
    file        = _file;
    //check sugli elementi creati
    if( !pipeline || !source || !demuxer || !decoder || !conv || !sink ){
        g_printerr("Almeno uno degli elementi non puo essere creato e quindi esco\n");
        return -1;
    }

    //faccio il setup della pipeline

    //setto l'input file name come elemento sorgente
    g_object_set(G_OBJECT(source), "location", file, NULL);

    //aggiungo alla pipeline l'handler dei messagg in modo da poter accedere un per volta i vari elementi
    //e quindi ai bus dei messaggi
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    //aggiungo i singoli elementi alla pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, demuxer, decoder, conv, sink, NULL);

    //link tutti gli elementi per formare un unica pipiline:
    //file source --> ogg-demuxer --> vorbis-decoder --> converter --> alsa out
    gst_element_link(source, demuxer);
    gst_element_link_many(decoder, conv, sink, NULL);
    //N.B: il demuxer e il decoder sono linkati dinamicamente attraverso la funzione on_pad_added
    //ciò è dovuto al fatto che ogg-demuxer può contenere piu stream, esempio audio e video. Per questo
    //tramite il link dinamico degli elementi è lo stesso Gstream che si prende carico di vedere quanti
    //stream ci sono ed aggiungere all'evenienza, runtime, tutti le interfacce, pad, necessarie tra un elemento e
    //l'altro per far fluire i dati correttamente
    g_signal_connect(demuxer, "pad_added", G_CALLBACK(on_pad_added), decoder);
    return true;
}

OggVorbisPlayer* OggVorbisPlayer::create(char* file)
{

    if(!gstreamInit()){
        g_print("fallito a creare l'api gst\n");
        return NULL;
    }

    OggVorbisPlayer *player = new OggVorbisPlayer();
    if( !player ){
        g_print("fallito a creare l'istanza di OggVorbisPlayer");
        return NULL;
    }

    if( !player->gstreamPipeLine(file) ){
        g_print("fallito a creare la pipelie");
        return NULL;
    }

    return player;
}

bool OggVorbisPlayer::play()
{
    //setto la pipeline con stato di play
    g_print("Sono in Playing: %s\n", file);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    return true;
}

void OggVorbisPlayer::run()
{
    //itero
    g_print("nell'iterazione...\n");
    g_main_loop_run(loop);
}

bool OggVorbisPlayer::stop()
{
    //quando finisco il main loop pulisco tutto
    g_print("ok mi fermo, STO Play\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    return true;
}

OggVorbisPlayer::~OggVorbisPlayer()
{
    //distruggo la pipeline
    g_print("Distruggo la pipeline\n");
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
}

