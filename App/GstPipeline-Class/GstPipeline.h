#ifndef __GSTREAMER_PIPELINE_H__
#define __GSTREAMER_PIPELINE_H__

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string>


#include <QWaitCondition>
#include <QMutex>

class gstPipeline
{
public:
     /*
      * Distruttore della classe
      */
    ~gstPipeline();

    /**
      * @brief gstPipeline costruttore della classe
      * @param pipeline la stringa della pipeline di gststreamer
      * @param width larghezza dell'immagine
      * @param height altezza dell'immagine
      * @param depth quanti canli ha l'immagine
      */
     gstPipeline(std::string pipeline, uint32_t width, uint32_t height, uint32_t depth);

     /**
      * @brief open apertura della camera per riceve il flusso dati
      * @return
      */
     bool open();

     /**
      * @brief close chiusura della camera e pulitura
      */
     void close();

     /**
      * @brief Capture
      * @param cpu
      * @param cuda
      * @param timeout
      * @return
      */
     bool Capture( void** cpu, void** cuda, unsigned long timeout=ULONG_MAX );


     //--------------------------------------------------
     inline uint32_t GetWidth() const	  { return mWidth; }
     inline uint32_t GetHeight() const	  { return mHeight; }
     inline uint32_t GetPixelDepth() const { return mDepth; }
     inline uint32_t GetSize() const		  { return mSize; }

private:
     static void onEOS(_GstAppSink* sink, void* user_data);
     static GstFlowReturn onPreroll(_GstAppSink* sink, void* user_data);
     static GstFlowReturn onBuffer(_GstAppSink* sink, void* user_data);

     /**
      * @brief init di gstreamer
      * @return true se tutto è ok
      */
     bool init();

     /**
      * @brief buildLaunchString creazione della stringa di lancio della pipeline
      * @return
      */
     bool buildLaunchString();

     /**
      * @brief checkMessage check dei messaggi della pipeline
      */
     void checkMsgBus();

     /**
      * @brief checkBuffer fai il check dei buffer , ovvero contenitori chunk data
      */
     void checkBuffer();

     GstBus*         mBus;           //bus per ottenere le informazioni dalla pipeline a run tim
     GstAppSink*     mAppsink;       //appSink per estrarre il flusso video dalla pipeline
     GstElement*     mPipeline;      //la pipeline

     std::string     mLaunchString;   //stringa contente le pipeline

     uint32_t        mWidth;        //largezza immagine
     uint32_t        mHeight;       //altezza immagine
     uint32_t        mDepth;        //quanti canali ha l'immagine
     uint32_t        mSize;         //grandezza in byte del buffer

     QWaitCondition* mWaitEvent;    //un thread dice ad un altro thread se una condizione è stata raggiunta

     QMutex* mWaitMutex;            //qmutex determina che un solo thread in un solo instante può accedere ad una risorsa/oggetto/codice
     QMutex* mRingMutex;

     static const uint32_t NUM_RINGBUFFERS = 16;
     void* mRingbufferCPU[NUM_RINGBUFFERS];
     void* mRingbufferGPU[NUM_RINGBUFFERS];

     uint32_t mLatestRGBA;
     uint32_t mLatestRinbuffer;
     bool     mLatestRetrived;

     void* mRGBA[NUM_RINGBUFFERS];
};

#endif

