#include "GstPipeline.h"

int main(int argc, char *argv[])
{

    gstPipeline* pipe = new gstPipeline("bella li", 1280, 960, 12);
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

    }

    pipe->close();
    if(pipe !=NULL)
    {
       delete pipe;
       pipe = NULL;
    }


   return 0;
}

