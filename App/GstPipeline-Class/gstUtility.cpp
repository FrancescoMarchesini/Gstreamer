/*
 * Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "gstUtility.h"

#include <gst/gst.h>
#include <stdint.h>
#include <stdio.h>

inline const char* gst_debug_level_str( GstDebugLevel level )
{
        switch (level)
        {
                case GST_LEVEL_NONE:	return "GST_LEVEL_NONE   ";
                case GST_LEVEL_ERROR:	return "GST_LEVEL_ERROR  ";
                case GST_LEVEL_WARNING:	return "GST_LEVEL_WARNING";
                case GST_LEVEL_INFO:	return "GST_LEVEL_INFO   ";
                case GST_LEVEL_DEBUG:	return "GST_LEVEL_DEBUG  ";
                case GST_LEVEL_LOG:		return "GST_LEVEL_LOG    ";
                case GST_LEVEL_FIXME:	return "GST_LEVEL_FIXME  ";
#ifdef GST_LEVEL_TRACE
                case GST_LEVEL_TRACE:	return "GST_LEVEL_TRACE  ";
#endif
                case GST_LEVEL_MEMDUMP:	return "GST_LEVEL_MEMDUMP";
                default:				return "<unknown>        ";
    }
}

#define SEP "              "

void rilog_debug_function(GstDebugCategory* category, GstDebugLevel level,
                          const gchar* file, const char* function,
                          gint line, GObject* object, GstDebugMessage* message,
                          gpointer data)
{
        if( level > GST_LEVEL_ERROR /*GST_LEVEL_INFO*/ )
                return;

        const char* typeName  = " ";
        const char* className = " ";

        if( object != NULL )
        {
                typeName  = G_OBJECT_TYPE_NAME(object);
                className = G_OBJECT_CLASS_NAME(object);
        }

        printf(LOG_GST_PIPE_INFO "%s %s %s\n" SEP "%s:%i  %s\n" SEP "%s\n",
                        gst_debug_level_str(level), typeName,
                        gst_debug_category_get_name(category), file, line, function,
                gst_debug_message_get(message));

}


bool gstreamerInit()
{
    int argc = 0;
    //char* argv[] = {"nome"};

    if(!gst_init_check(&argc, NULL, NULL))
    {
        printf(LOG_GST_PIPE_ERROR"Fallito init Gstreamer, ESCO");
        return false;
    }

    //versione di gstreamer in uso
    uint32_t ver[] = { 0, 0, 0, 0 };
    gst_version(&ver[0], &ver[1], &ver[2], &ver[3]);
    printf(LOG_GST_PIPE_INFO "inizializzato gstream con la versione %u.%u.%u.%u\n", ver[0], ver[1], ver[2], ver[3]);

    // debugging
    gst_debug_remove_log_function(gst_debug_log_default);

    if( true )
    {
        gst_debug_add_log_function(rilog_debug_function, NULL, NULL);

        gst_debug_set_active(true);
        gst_debug_set_colored(false);
     }

     return true;
}

static void gst_print_one_tag(const GstTagList * list, const gchar * tag, gpointer user_data)
{
  int i, num;

  num = gst_tag_list_get_tag_size (list, tag);
  for (i = 0; i < num; ++i) {
    const GValue *val;

    /* Note: when looking for specific tags, use the gst_tag_list_get_xyz() API,
     * we only use the GValue approach here because it is more generic */
    val = gst_tag_list_get_value_index (list, tag, i);
    if (G_VALUE_HOLDS_STRING (val)) {
      printf("\t%20s : %s\n", tag, g_value_get_string (val));
    } else if (G_VALUE_HOLDS_UINT (val)) {
      printf("\t%20s : %u\n", tag, g_value_get_uint (val));
    } else if (G_VALUE_HOLDS_DOUBLE (val)) {
      printf("\t%20s : %g\n", tag, g_value_get_double (val));
    } else if (G_VALUE_HOLDS_BOOLEAN (val)) {
      printf("\t%20s : %s\n", tag,
          (g_value_get_boolean (val)) ? "true" : "false");
    } else if (GST_VALUE_HOLDS_BUFFER (val)) {
      //GstBuffer *buf = gst_value_get_buffer (val);
      //guint buffer_size = GST_BUFFER_SIZE(buf);

      printf("\t%20s : buffer of size %u\n", tag, /*buffer_size*/0);
    } /*else if (GST_VALUE_HOLDS_DATE_TIME (val)) {
      GstDateTime *dt = (GstDateTime*)g_value_get_boxed (val);
      gchar *dt_str = gst_date_time_to_iso8601_string (dt);
      printf("\t%20s : %s\n", tag, dt_str);
      g_free (dt_str);
    }*/ else {
      printf("\t%20s : tag of type '%s'\n", tag, G_VALUE_TYPE_NAME (val));
    }
  }
}

static const char* gst_stream_status_string( GstStreamStatusType status )
{
        switch(status)
        {
                case GST_STREAM_STATUS_TYPE_CREATE:	return "CREATE";
                case GST_STREAM_STATUS_TYPE_ENTER:		return "ENTER";
                case GST_STREAM_STATUS_TYPE_LEAVE:		return "LEAVE";
                case GST_STREAM_STATUS_TYPE_DESTROY:	return "DESTROY";
                case GST_STREAM_STATUS_TYPE_START:		return "START";
                case GST_STREAM_STATUS_TYPE_PAUSE:		return "PAUSE";
                case GST_STREAM_STATUS_TYPE_STOP:		return "STOP";
                default:							return "UNKNOWN";
        }
}

gboolean gst_message_print(_GstBus* bus, _GstMessage* msg, void* user_data)
{
    GError *err;
    gchar *debug_info;

    switch(GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debug_info);
            printf(LOG_GST_PIPE_ERROR"Errore nella ricezione dell'elemento %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            printf(LOG_GST_PIPE_ERROR"inforamzioni di debug: %s\n", debug_info ? debug_info: "none");
            g_clear_error(&err);
            g_free(debug_info);
            //g_main_loop_quit (data->mainLoop);
            break;
        case GST_MESSAGE_EOS:
            printf(LOG_GST_PIPE_INFO"End-Of-Stream ricevuta\n");
            //g_main_loop_quit (data->mainLoop);
            break;
        case GST_MESSAGE_STATE_CHANGED:
        {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
            if(GST_MESSAGE_SRC(msg) == GST_OBJECT(user_data))
            {
                printf(LOG_GST_PIPE_INFO"Stato della pipeline cambiato da %s a %s:\n", gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));

                if(new_state = GST_STATE_PLAYING)
                {
                //quando sono in play analizzo lo stream
                //analyze_streams(data);
                }
            }
            break;
        }
        case GST_MESSAGE_STREAM_STATUS:
        {
            GstStreamStatusType streamStatus;
            gst_message_parse_stream_status(msg, &streamStatus, NULL);

            printf(LOG_GST_PIPE_INFO "gstreamer stream status %s ==> %s\n",
                                                            gst_stream_status_string(streamStatus),
                                                            GST_OBJECT_NAME(msg->src));
            break;
         }
         case GST_MESSAGE_TAG:
         {
            GstTagList *tags = NULL;

            gst_message_parse_tag(msg, &tags);


            gchar* txt = gst_tag_list_to_string(tags);



            printf(LOG_GST_PIPE_INFO "gstreamer %s %s\n", GST_OBJECT_NAME(msg->src), txt);

            g_free(txt);
            gst_tag_list_free(tags);
            break;
           }
           default:
           {
                printf(LOG_GST_PIPE_INFO "gstreamer msg %s ==> %s\n", gst_message_type_get_name(GST_MESSAGE_TYPE(msg)), GST_OBJECT_NAME(msg->src));
                                                                                    break;
           }

    }

    //voglio continuare a ricevere messaggi
    return TRUE;
}
