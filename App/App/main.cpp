#include <algorithm>
//#include "glDisplay.h"
//#include "glTexture.h"
#include <iostream>
#include <signal.h>


#define LOG_MAIN "[LOG_MAIN] "
/*
bool recived_signal = false;

void signale_handler(int sign)
{
    if(sign == SIGINT)
    {
        printf("%ssegnale ricevuto\n", LOG_MAIN);
        recived_signal = true;
    }
}

int main(int argc, char** argv)
{
    printf("%sBella li\n", LOG_MAIN);


    if(signal(SIGINT, signale_handler) == SIG_ERR){
        printf("%sFallito a recipire i segnali esco\n", LOG_MAIN);
        return 0;
    }

    printf("%sInizializzazione statica di glDiplay\n", LOG_MAIN);
    int winWidth = 512;
    int winHeight = 512;
    glDisplay *display = glDisplay::Create();

    while( !recived_signal )
    {

           if( display != NULL)
           {
               char title[256];
               sprintf(title, "[LOG_MAIN] %04.1f FPS", display->GetFPS());
               display->SetTitle(title);

               display->UserEvents();

               display->BeginRender();


               display->EndRender();
           }
    }


    if(display != NULL)
    {
        delete display;
        display = NULL;
    }

    return 0;
}

*/

#include <gst/gst.h>

/*
int main(int argc, char* argv[])
{
    //gchar è un interno della libreria glib
    const gchar *nano_str;
    guint major, minor, micro, nano;

    //Istanzio un elemento
    GstElement *element;

    gst_init(&argc, &argv);

    gst_version(&major, &minor, &micro, &nano);

    if(nano == 1)
        nano_str = "(CSV)";
    else if(nano == 2)
        nano_str = "(Prerelease)";
    else
        nano_str = "";

    printf("questo programma è linkato con gstream %d.%d.%d %s\n", major, minor, micro, nano_str);

    element = gst_element_factory_make("fakesrc", "source");
    if( !element ){
        printf("Fallito a creare elemento di tipo 'fakesrc'\n");
        return -1;
    }

    gst_object_unref(GST_OBJECT(element));
    return 0;

}
*/

//utilizzo di GOption
int main(int argc, char* argv[])
{
    gboolean silent = FALSE;
    gchar *savefile =  NULL;
    GOptionContext *ctx; //server per parsare da linea di comando
    GError *err = NULL;
    GOptionEntry enteries[] = {
        {"silent", 's', 0, G_OPTION_ARG_NONE, &silent,
        "non printo un cazzo perchè sono silent"},
        {"output", 'o', 0, G_OPTION_ARG_STRING, &savefile,
        "salvo un xml che rappresenta la pipeline ed esco", "FILE"},
        {NULL}
    };

    ctx = g_option_context_new("myGstGlibFuckinApp");
    g_option_context_add_main_entries(ctx, enteries, NULL);
    g_option_context_add_group(ctx, gst_init_get_option_group());
    if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
       g_print ("Fallito ad inizializzare il todos: %s\n", err->message);
       g_clear_error (&err);
       g_option_context_free (ctx);
       return 1;
     }
     g_option_context_free (ctx);

     printf ("Eseguimi con --help per vedere le opzioni dell'app.\n");

     return 0;
}

