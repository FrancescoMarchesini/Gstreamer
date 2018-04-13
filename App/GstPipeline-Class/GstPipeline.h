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
      * @brief Capture cattura ring buffer di grandezza 20M ogni timeout
      * @param cpu  puntatore alla memoria cpu
      * @param cuda puntatore alla memoria cuda dove spostare i dati
      * @param timeout  variabile temporale per il sample data
      * @return
      */
     bool Capture( void** cpu, void** cuda, unsigned long timeout=ULONG_MAX );

     /**
      * @brief convertRGBA convertire il flusso video da NV12 a RGBA
      * @param cpu  memoria cpu
      * @param cuda memoria cuda
      * @return
      */
     bool ConvertRGBA(void *input, void **ouput);

     /**
      * @brief GetWidth grandezza dell'immagine
      * @return
      */
     inline uint32_t GetWidth() const	  { return mWidth; }

     /**
      * @brief GetHeight altezza dell'immagine
      * @return
      */
     inline uint32_t GetHeight() const	  { return mHeight; }

     /**
      * @brief GetPixelDepth il numero di bit per pixel, NVMM = 12,per ogni componente RGBA
      * @return
      */
     inline uint32_t GetPixelDepth() const { return mDepth; }

     /**
      * @brief GetSize grandezza del buffer = W x H x Depth / 8
      * @return
      */
     inline uint32_t GetSize() const		  { return mSize; }

private:

     /**
      * @brief onEOS callback per fine dello stream
      * @param sink  elemento gst di uscita dell'elemnto
      * @param user_data gli elementi che si vuole analizzare
      */
     static void onEOS(_GstAppSink* sink, void* user_data);

     /**
      * @brief onPreroll callback per immagazzinare i dati prima di mettore lo stato su play
      * @param sink elemneto di uscita
      * @param user_data oggetto sul quale agire
      * @return
      */
     static GstFlowReturn onPreroll(_GstAppSink* sink, void* user_data);

     /**
      * @brief onBuffer callback nel durante della ricezione dei chunck dello stream video
      * @param sink elmento di uscita
      * @param user_data oggetto sul quale agire
      * @return
      */
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

     static const uint32_t NUM_RINGBUFFERS = 16;        //grandezza del ring baffer è 16

     void* mRingbufferCPU[NUM_RINGBUFFERS];             //buffer della cpu
     void* mRingbufferGPU[NUM_RINGBUFFERS];             //buffer della GPU


     QWaitCondition* mWaitEvent;    //un thread dice ad un altro thread se una condizione è stata raggiunta

     QMutex* mWaitMutex;            //qmutex determina che un solo thread in un solo instante può accedere ad una risorsa/oggetto/codice
     QMutex* mRingMutex;

     uint32_t mLatestRGBA;                              //ultimo frame preso
     uint32_t mLatestRinbuffer;                         //utlimo frame preso del ring buffer
     bool     mLatestRetrived;                          //condizione di blocco per i thread

     void* mRGBA[NUM_RINGBUFFERS];                      //vettore per la conversione
};

#endif

