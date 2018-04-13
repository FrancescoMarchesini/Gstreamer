#include "GstPipeline.h"


int main(int argc, char *argv[])
{
    gstPipeline* pipe = new gstPipeline("bella", 1280, 960, 12);

    if(!pipe)
    {
        printf("Cazzo, Cazzo, Cazzo\n");
        exit(1);
    }

    /*inizio lo streaming */
    if(!pipe->open() )
    {
        printf("Merda, Merda, Merda\n");
        exit(1);
    }

    while(true)
    {
        void *imgCPU = NULL;
        void *imgCuda = NULL;

        if( !pipe->Capture(&imgCPU, &imgCuda, 1000))
        {
            printf("Figa, Figa, Figa\n");
        }

        void* imgRGBA = NULL;

        if( !pipe->ConvertRGBA(imgCuda, &imgRGBA))
        {
            printf("scurrile\n");
        }

        //un puntatore void non può essere deferenziato direttamente.
        //float * a = (float*)imgRGBA[1]; // questo da errore

        //per accedere ed usarlo bisogna fare un cast esclicito.
        float* bella_li = static_cast<float*>(imgRGBA);
        //printf("%0.f", *bella_li);

    }

    pipe->close();
    if(pipe !=NULL)
    {
       delete pipe;
       pipe = NULL;
    }


   return 0;
}

