Multithreding:

generazione video di un forma onda con frequenza X.
L'audio ed il video sono eseguti in parallelo grazie ai thread.

di seguita la visualizzazione della pipeline che:

![Alt text](./pipeline.png?raw=true "Title")


la pipeline del codice che puo essere esegui da riga di comando è la seguente:
gst-launch-1.0 --gst-debug=3  audiotestsrc 'freq=125.0' ! tee name=t ! queue ! audioconvert ! audioresample ! autoaudiosink t. ! queue ! wavescope 'shader=5' 'style=2' ! nvvidconv ! 'video/x-raw(memory:NVMM), format=(string)NV12' ! autovideosink

come da documentazione  è possibile giorcare sui seguenti comandi per avere effetti varie

tramite il comando:
>gst-inspect-1.0 wavescope è possibilie vedere i parametri dello shader e quindi modificarli nel codice alla riga 81:
 81 : g_object_set(data.visual, "shader", 3, "style", 1, NULL); 

>gst-inspect-1.0 audiotestsrc per modificare la frequanza del tono
 80 : g_object_set(data.audio_source, "freq", 115.0, NULL);


freq
shader
style
