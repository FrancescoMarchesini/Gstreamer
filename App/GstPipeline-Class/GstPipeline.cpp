#include "GstPipeline.h"
#include "gstUtility.h"

#include <sstream>
#include <unistd.h>
#include <string.h>

#include <inttypes.h>
#include <cuda_runtime.h>
#include <cuda.h>



gstPipeline::~gstPipeline()
{
    printf("%sDistruggo l'istanza gstPipeline\n", LOG_GST_PIPE_INFO);
    delete mBus;
    delete mAppsink;
    delete mPipeline;
    delete mWaitEvent;
    delete mWaitMutex;
}

gstPipeline::gstPipeline(std::string pipeline, uint32_t width, uint32_t height, uint32_t depth)
{

    printf("%sCreo l'istanza gstPipeline\n", LOG_GST_PIPE_INFO);

    mBus        = NULL;
    mAppsink    = NULL;
    mPipeline   = NULL;

    mWidth  = 0;
    mHeight = 0;
    mDepth  = 0;
    mSize   = 0;

    mWaitEvent = new QWaitCondition();
    mWaitMutex = new QMutex();
    mRingMutex = new QMutex();

    mLatestRinbuffer = 0;
    mLatestRetrived  = false;

    for( uint32_t n=0; n < NUM_RINGBUFFERS; n++)
    {
        mRingbufferCPU[n] = NULL;
        mRingbufferGPU[n] = NULL;
    }

    //1 init di gst
    if(!gstreamerInit()){
        printf("%sFallito init di gstreamer, ESCO\n", LOG_GST_PIPE_ERROR);
        exit(1);
    }

    mWidth = width;
    mHeight = height;
    mDepth = depth;
    mSize = (width * height * mDepth) / 8;
    mLaunchString = pipeline; //questo da rivedere in quanto superfluo

    if(!init())
    {
        printf("%sFallito init di istanza classe, ESCO\n", LOG_GST_PIPE_ERROR);
        exit(1);
    }

    //printf("%sgst-pipeline = %s\n", LOG_GST_PIPE_INFO, mLaunchString.c_str());
    //printf("%swidth = %d\n", LOG_GST_PIPE_INFO, mWidth);
    //printf("%sheight = %d\n", LOG_GST_PIPE_INFO,mHeight);
    //printf("%sdepth = %d\n", LOG_GST_PIPE_INFO, mDepth);
}

// onEOS
void gstPipeline::onEOS(_GstAppSink* sink, void* user_data)
{
    printf("%sGstreamer decoder End OF Stream \n", LOG_GST_PIPE_INFO);
}


// onPreroll
GstFlowReturn gstPipeline::onPreroll(_GstAppSink* sink, void* user_data)
{
    printf("%sGstramer decoder on Prerool \n", LOG_GST_PIPE_INFO);
    return GST_FLOW_OK;
}

#define release_return { gst_sample_unref(gstSample); return; }

// onBuffer
GstFlowReturn gstPipeline::onBuffer(_GstAppSink* sink, void* user_data)
{
    printf("%sGstramer decoder buffer \n", LOG_GST_PIPE_INFO);
    if(!user_data)
        return GST_FLOW_OK;

    //necessita di instanziare l'oggetto per chiamre le callback
    gstPipeline* dec = (gstPipeline*)user_data;

    dec->checkBuffer();
    dec->checkMsgBus();
    return GST_FLOW_OK;
}

bool gstPipeline::Capture(void **cpu, void **cuda, unsigned long timeout)
{
    mWaitMutex->lock();
    const bool wait_result = mWaitEvent->wait(mWaitMutex, timeout);
    printf(LOG_GST_PIPE_INFO"..........mWaitMutex\n");
    mWaitMutex->unlock();

    if(!wait_result)
        return false;

    mRingMutex->lock();
    printf("\n");
    const uint32_t latest = mLatestRinbuffer;
    const bool retrived = mLatestRetrived;
    mLatestRetrived = true;
    printf(LOG_GST_PIPE_INFO"..........mRingMutex\n");
    mRingMutex->unlock();

    if(retrived)
    {
        printf(LOG_GST_PIPE_INFO"retived\n");
        return false;
    }

    if(cpu != NULL)
    {
        printf(LOG_GST_PIPE_INFO"CPU ring buffer\n");
        *cpu = mRingbufferCPU[latest];
    }


    if(cuda != NULL)
    {
        printf(LOG_GST_PIPE_INFO"GPU ring buffer\n");
        *cuda = mRingbufferGPU[latest];
    }

    printf("\n");
    return true;
}

void gstPipeline::checkBuffer()
{
    if( !mAppsink )
        return;

    /** 1
     * block wating per i il buffer .................
     * serve per estrarre sample dalla pipeline
     * app_sink_pull_sample : questo metodo ferma finche un sample non è disponibile o quando si è in EOS
     * ritorna il sample solo quando lo stato di GST è in GST_PLAY
     * l'applicazione deve pullare sample in modo sufficente altrimenti si blocca tutto..
     * GstSample è un oggetto che contiente data, type, timming e altri metadati
     **/
    GstSample* gstSample = gst_app_sink_pull_sample(mAppsink);
    if(!gstSample)
    {
        printf("%sgstSample return NULL\n", LOG_GST_PIPE_ERROR);
        return;
    }

    /* 2 creazione del buffer*/
    GstBuffer* gstBuffer = gst_sample_get_buffer(gstSample);
    if(!gstBuffer)
    {
        printf("%sgsBuffer return NULL\n", LOG_GST_PIPE_ERROR);
        return;
    }

    /* 3 struttura contente le info per allocare la memoria*/
    GstMapInfo map;
    /* riempi l'indirizza di memoria di map con le info prese dai vari buffer*/
    if(!gst_buffer_map(gstBuffer, &map, GST_MAP_READ))
    {
        printf("%sgst_buffer_map() falllito \n", LOG_GST_PIPE_ERROR);
        return;
    }

    void* gstData = map.data;
    const uint32_t gstSize = map.size;
    if( !gstData )
    {
        printf("%sgst:buffer ha NULL pointer \n", LOG_GST_PIPE_ERROR);
        release_return;
    }

    /* 4 prendo le info dei caps dai sample */
    GstCaps * gstCaps = gst_sample_get_caps(gstSample);
    if(!gstCaps)
    {
        printf("%sgstbuffer ha NULL caps \n", LOG_GST_PIPE_ERROR);
        release_return;
    }

    GstStructure* gstCapsStructure = gst_caps_get_structure(gstCaps, 0);
    if(!gstCapsStructure)
    {
        printf("%sgst_caps ha NULL structure \n", LOG_GST_PIPE_ERROR);
        release_return;
    }

    /* 5 informazioni del buffere per allocare la memoria */
    int width = 0;
    int height = 0;

    if(!gst_structure_get_int(gstCapsStructure, "width", &width) ||
       !gst_structure_get_int(gstCapsStructure, "height", &height) )
    {
        printf("%sgst_caps ha le info cercate width e height \n", LOG_GST_PIPE_ERROR);
         release_return;
    }

    if(width < 1 || height < 1 )
         release_return;

    mWidth  = width;
    mHeight = height;
    mDepth  = (gstSize * 8) / (width * height);
    mSize   = gstSize;

    printf("%sla pipeline ha ricevuto %ix%i frame (%u bytes, %u bite per pixel )\n", LOG_GST_PIPE_INFO, width, height, gstSize, mDepth);

    /* 6 qui tramite il puntatore all'immagine alloca la memoria della GPU o utilizzo i pixel */
    if(!mRingbufferCPU[0])
    {
        for(uint32_t n=0; n < NUM_RINGBUFFERS; n++)
        {
            printf(LOG_GST_PIPE_INFO"buffer num : %d, indirizzo di memoria CPU: %p,  bytes : %u, valore: %u\n", n, (void*)&mRingbufferCPU[n], gstSize, (uint32_t*)mRingbufferCPU[n]);
            printf(LOG_GST_PIPE_INFO"buffer num : %d, indirizzo di memoria GPU: %p,  bytes : %u, valore: %u\n", n, (void*)&mRingbufferGPU[n], gstSize, (uint32_t*)mRingbufferGPU[n]);
            if(!&mRingbufferCPU || !mRingbufferGPU || gstSize == 0)
                printf("MALE MALE MALE \n");

              cudaHostAlloc(&mRingbufferCPU[n], (size_t)gstSize, cudaHostAllocMapped);
              cudaHostGetDevicePointer(&mRingbufferGPU[n], mRingbufferCPU[n], 0);
              memset(mRingbufferCPU[n], 0, (size_t)gstSize);
              printf(LOG_GST_PIPE_INFO"cudaAllocMapped %zu bytes, CPU %p GPU %p\n", (size_t)gstSize, mRingbufferCPU, mRingbufferGPU);
        }
    }

    /* 7 mi sposto nel buffer successivo */
    const uint32_t nextRingBuffer = (mLatestRinbuffer + 1 ) % NUM_RINGBUFFERS;

    printf(LOG_GST_PIPE_INFO"uso il ringbuffer #%u per il nuovo frame\n", nextRingBuffer);
    memcpy(mRingbufferCPU[nextRingBuffer], gstData, gstSize);

    /* 8 pulitura */
    gst_buffer_unmap(gstBuffer, &map);
    gst_sample_unref(gstSample);

    /* 9 metto i segnali sui thread in modo da serializzarli corretamente */
    mRingMutex->lock();
    mLatestRinbuffer = nextRingBuffer;
    mLatestRetrived = false;
    mRingMutex->unlock();
    mWaitEvent->wakeAll();
}

bool gstPipeline::buildLaunchString()
{
    std::ostringstream ss;

    const int flipMethod = 2;  //ruota di 180 la camera
    std::string cameraAddress = "root:root@192.168.1.90";
    const int latency = 100;

    ss << "rtspsrc location=rtsp://"<< cameraAddress <<"/axis-media/media.amp?";
    ss << "resolution="<< mWidth <<"x"<< mHeight <<" drop-on-latency=0 latency="<< latency <<" ! ";
    ss << "queue max-size-buffers=200 max-size-time=1000000000  max-size-bytes=10485760 min-threshold-time=10 ! ";
    ss << "rtph264depay ! h264parse ! omxh264dec ! ";
    ss << "nvvidconv flip-method=" << flipMethod << " ! video/x-raw, width=(int)1920, height=(int)720, format=(string)NV12 ! ";
    ss << "appsink name=mysink";

    mLaunchString = ss.str();
    printf("%sgst-pipeline = %s\n", LOG_GST_PIPE_INFO, mLaunchString.c_str());
    return true;
}

bool gstPipeline::init()
{

    //2 check degli errori
    GError* err = NULL;

    //3 init della stringa
    if(!buildLaunchString())
    {
        printf("%sFallito a creare la stringa di lancio\n", LOG_GST_PIPE_ERROR);
        exit(1);
    }

    mPipeline = gst_parse_launch(mLaunchString.c_str(), &err);
    if(err != NULL)
    {
        printf("%sFallito a fare il decoder della stringa\n", LOG_GST_PIPE_ERROR);
        printf("%s     (%s)\n", LOG_GST_PIPE_ERROR, err->message);
        g_error_free(err);
        return false;
    }

    //4 creazione dell'oggetto pipeline
    GstPipeline* pipeline = GST_PIPELINE(mPipeline);
    if(!pipeline)
    {
        printf("%sFallito a fare il cast tra gstElment nel gstPipeline \n", LOG_GST_PIPE_ERROR);
        return false;
    }

    //5 init dei messaggi dalla pipeline
    mBus = gst_pipeline_get_bus(pipeline);
    if(!mBus)
    {
        printf("%sFallito a riceve i messaggi dalla pipeline\n", LOG_GST_PIPE_ERROR);
        return false;
    }
    //callback di gstUtility per analizzarei messaggi generati
    gst_bus_add_watch(mBus, (GstBusFunc)gst_message_print, NULL);

    //6 gli elmenti appsrc e appsink per estrarre il flusso video dalla pipeline
    GstElement* appSinkElement = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");  //estraggo l'elemento appsink dalla pipeline tramite il suo nome "mysink"
    GstAppSink* appsink = GST_APP_SINK(appSinkElement);                             //creo l'oggetto appsink per uso futuro
    if( !appSinkElement || !appsink )
    {
        printf("%sFallito a richiedere l'oggetto appsink dalla pipeline\n", LOG_GST_PIPE_ERROR);
        return false;
    }

    mAppsink = appsink;

    //7 funzioni di call back per agire su appsink
    GstAppSinkCallbacks cb;
    memset(&cb, 0, sizeof(GstAppSinkCallbacks));

    cb.eos = onEOS;                     //callback End-Of_Stream
    cb.new_preroll = onPreroll;         //callback per inizializzare il video
    cb.new_sample = onBuffer;           //callback per ricevere i buffer data

    gst_app_sink_set_callbacks(mAppsink, &cb, (void*)this, NULL);

    return true;
}

bool gstPipeline::open()
{
    printf("%sMetto lo stato della pipeline su GST_STATE_PLAY\n", LOG_GST_PIPE_INFO);

    const GstStateChangeReturn ret = gst_element_set_state(mPipeline, GST_STATE_PLAYING);
    /**
     * ASYNC:  lo stato cambia dopo un po
     * ovvero quando il buffer è pieno e puo essere quandi utilizzato
     */
    if(ret == GST_STATE_CHANGE_ASYNC)
    {

        GstMessage* asyncMsg = gst_bus_timed_pop_filtered(mBus, 5* GST_SECOND, (GstMessageType)(GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_ERROR));
        if(asyncMsg != NULL)
        {
            gst_message_print(mBus, asyncMsg, this);
            gst_message_unref(asyncMsg);
        }
        else
            printf(LOG_GST_PIPE_INFO "Messaggio NULL da gst dopo avere messo lo stato su play\n");

    }
    else if(ret != GST_STATE_CHANGE_SUCCESS)
    {
        printf("%sFallito a mettere lo state della pipeline su PLAY (error %u)\n", LOG_GST_PIPE_ERROR, ret);
        return false;
    }

    checkMsgBus();
    usleep(100*1000);
    checkMsgBus();

    return true;
}

void gstPipeline::close()
{
    printf("%sMetto lo stato della pipeline su GST_STATE_NULL\n", LOG_GST_PIPE_INFO);
    const GstStateChangeReturn ret = gst_element_set_state(mPipeline, GST_STATE_NULL);
    if(ret != GST_STATE_CHANGE_SUCCESS)
    {
        printf("%sFallito a mettere lo state della pipeline su NULL (error %u)\n", LOG_GST_PIPE_ERROR, ret);
    }
    usleep(250*1000);

}

void gstPipeline::checkMsgBus()
{
    while(true)
    {
        GstMessage * msg = gst_bus_pop(mBus);
        if(!msg)
            break;

        gst_message_print(mBus, msg, this);
        gst_message_unref(msg);
    }
}
