Multithreding:

generazione video di un forma onda con frequenza X.
L'audio ed il video sono eseguiti in parallelo grazie ai thread di gstreamer.

la pipeline generata è la seguente:

![Alt text](./pipeline.png?raw=true "Title")


La pipeline può essere eseguita anche da riga di comando nel seguente modo:

>gst-launch-1.0 --gst-debug=3  audiotestsrc 'freq=125.0' ! tee name=t ! queue ! audioconvert ! audioresample ! autoaudiosink t. ! queue ! wavescope 'shader=5' 'style=2' ! nvvidconv ! 'video/x-raw(memory:NVMM), format=(string)NV12' ! autovideosink

E' possibilie giocare sui varie effetti modificando i seguenti parametri della pipeline:

freq=125
shader=5
style=2

per sapere i valori da dare alle seguenti variabili è possibili indagare i due oggetti gstreamer il seguente comando

>gst-inspect-1.0 wavescope :  per modificare lo shader
 nel codice .cpp il valore è settato alla seguente riga
 81 : g_object_set(data.visual, "shader", 3, "style", 1, NULL); 

>gst-inspect-1.0 audiotestsrc : per modificare la frequanza del tono
 nel codice .cpp il valore è settato alla seguente riga
 80 : g_object_set(data.audio_source, "freq", 115.0, NULL);


