#ifndef MYGST_H
#define MYGST_H

#include <gst/gst.h>
#include <string.h>
#include <sstream>


class myGst
{
public:
    static myGst* create();
    bool open();
    void close();

private:
    myGst();
    bool init();
    bool buildLaunchstr();

    GstElement *mPipeline;
    GstBus *mBus;
    GstMessage *mMsg;

    std::string mLaunchstring;

};

#endif // MYGST_H
