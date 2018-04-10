#ifndef OGGVORBISPLAYER_H
#define OGGVORBISPLAYER_H
#include <gst/gst.h>
#include <glib.h>


class OggVorbisPlayer
{
public:
    static OggVorbisPlayer* create(char *file);

    ~OggVorbisPlayer();

    bool play();
    void run();
    bool stop();


private:

    OggVorbisPlayer();
    bool gstreamPipeLine(char *file);

    GMainLoop *loop;
    gchar *file;

    //dichiaro gli element
    GstElement *pipeline;
    GstElement *source;
    GstElement *demuxer;
    GstElement *decoder;
    GstElement *conv;
    GstElement *sink;

    //oggeto bus per i messaggi
    GstBus *bus;
    //id del bus corrente
    guint bus_watch_id;
};

#endif // OGGVORBISPLAYER_H
