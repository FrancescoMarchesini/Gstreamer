#include "GstPipeline.h"

#include <QThread>
#include <QMutex>
#include <QtCore>

#include <stdio.h>
#include <stdlib.h>

/*
 * "the producer" scrive i dati nel buffer finche non si raggiunge la fine del buffer, quando
 * si giunge alla riempitura del buffer si ricomincia da zero. "The consumer" thread legge i
 * dati dal buffer nel momento in cui vengono scritto dal producer "
 *
 * la "wait condition" permette di avere un alta cooccorenza, ovvero permette la lettura e la
 * scrittura da e in un solo buffer da due thread, in modo conccorenziale.
 *
 * le classi "producer" e "consumer" sono derivate dalla classe qthread. il circola buffer
 * invece è solo una variabile globale
 *
 */

/*buffer circolare*/
const int DataSize = 10000;         //l'ammontare di dati generati al "producer"

const int BufferSize = 8192;        //grandezza del circul buffer, essendo piu piccola del data size ad un punto finira e ricominciare a rimpire il buffer
char buffer[BufferSize];

QWaitCondition bufferNotEmpty;      //condizione per il "consumer": "The producer" dice che ha iniziato a scrivere nel buffer e quindi il "consumer" può leggere i dati e stamparli
QWaitCondition bufferNotFull;       //condizione per il "producer": "The consumer" dice che ha letto i dati e che quindi il "producer" può generane altri
QMutex mutex;                       //la condizione di blocco, ovvero la srializzazione dei thread fino ad una QWaitCondition
int numUsedBytes = 0;               //dati contenuti nel buffer ad un dato istante

/* The Producer */
class Producer : public QThread
{
public:
    Producer(QObject *parent = NULL) : QThread(parent)
    {
    }

    void run() override
    {
        //il producer genera i dati
        for(int i  = 0; i < DataSize; i++)
        {
            mutex.lock();
            //prima di scrivere nel circulabuffer, fa il check se il buffer è pieno
            if(numUsedBytes ==  BufferSize)
            {
                //se il buffer è pieno aspetto la condizione
                bufferNotFull.wait(&mutex);
            }
            mutex.unlock();

            mutex.lock();
            ++numUsedBytes; //aumento di n la grandezza dei dati scritto
            bufferNotEmpty.wakeAll(); //essendo che è sicuramente > 0 lascio andare il consumer
            mutex.unlock();
        }
    }
};

class Consumer : public QThread
{
public:
    Consumer(QObject *parent = NULL) : QThread(parent)
    {
    }

    void run() override
    {
        for (int i = 0; i < DataSize; ++i) {
            mutex.lock();
            if (numUsedBytes == 0)
                bufferNotEmpty.wait(&mutex);
            mutex.unlock();

            printf("%c", buffer[i % BufferSize]);

            mutex.lock();
            --numUsedBytes;
            bufferNotFull.wakeAll();
            mutex.unlock();
        }
        printf("\n");
    }

signals:
    void stringConsumed(const QString &text);
};

int main(int argc, char *argv[])
{

    QCoreApplication app(argc, argv);
    Producer producer;
    Consumer consumer;
    producer.start();
    consumer.start();
    producer.wait();
    consumer.wait();

   // return 0;


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
            printf("sizeof imgCPU %lu\n", sizeof(imgCPU));
            printf("sizeof imgGPU %lu\n", sizeof(imgCuda));
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

