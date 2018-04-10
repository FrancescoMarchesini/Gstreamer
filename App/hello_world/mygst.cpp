#include "mygst.h"
#include "gstUtility.h"

myGst::myGst()
{
    mPipeline = NULL;
    mBus      = NULL;
    mMsg      = NULL;
}

bool myGst::buildLaunchstr()
{
    std::ostringstream ss;
    ss  << "playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";
    mLaunchstring = ss.str();
    printf(LOG_GSTREAMER "gstream decoder pipeline string \n");
    printf(LOG_GSTREAMER "%s\n", mLaunchstring.c_str());
    return true;
}

bool myGst::init()
{
    if( !buildLaunchstr() )
    {
        printf(LOG_GSTREAMER "gstream decoder fallito a creare la pipeline");
        return false;
    }

    mPipeline = gst_parse_launch(mLaunchstring.c_str(), NULL);

    GstPipeline* pipeline = GST_PIPELINE(mPipeline);

    if( !pipeline )
    {
            printf(LOG_GSTREAMER "gstreamer failed to cast GstElement into GstPipeline\n");
            return false;
    }

    mBus = gst_element_get_bus (mPipeline);

    if( !mBus )
    {
            printf(LOG_GSTREAMER "gstreamer failed to retrieve GstBus from pipeline\n");
            return false;
    }

    return true;
}

myGst *myGst::create()
{
    if( !gstreamerInit()    )
    {
        printf(LOG_GSTREAMER "aiiia fallito a fare l'init di gstream");
        return NULL;
    }

    myGst* cam = new myGst();

    if(!cam)
        return NULL;

    if(!cam->init())
    {
        printf(LOG_GSTREAMER "aiaia fallito a creare la camera");
        return NULL;
    }

    return cam;
}

bool myGst::open()
{
    printf(LOG_GSTREAMER "gstreamer metto lo stato su :  GST_STATE_PLAYING\n");

    gst_element_set_state(mPipeline, GST_STATE_PLAYING);
    /*const GstStateChangeReturn result = gst_element_set_state(mPipeline, GST_STATE_PLAYING);

    if( result != GST_STATE_CHANGE_SUCCESS )
    {
        printf(LOG_GSTREAMER "gstreamer fallito a mettere su play cazzo (error %u)\n", result);
        return false;
    }
    */
    return true;
}

void myGst::close()
{
    mMsg = gst_bus_timed_pop_filtered (mBus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (mMsg != NULL)
        gst_message_unref (mMsg);
      gst_object_unref (mBus);
      gst_element_set_state (mPipeline, GST_STATE_NULL);
      gst_object_unref (mPipeline);
}
