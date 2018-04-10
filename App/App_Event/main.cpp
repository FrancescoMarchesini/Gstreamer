#include <algorithm>
#include <iostream>
#include <signal.h>
#define LOG_MAIN "[LOG_MAIN] "
#include <gst/gst.h>

static void seek_to_time (GstElement *element, guint64 time_ns)
{
  GstEvent *event;

  event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
                  GST_SEEK_FLAG_NONE,
                  GST_SEEK_METHOD_SET, time_ns,
                  GST_SEEK_TYPE_NONE, G_GUINT64_CONSTANT (0));
  gst_element_send_event (element, event);
}
