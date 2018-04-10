GST Bus

é un sistema che si prende carico dei messaggi inoltrati(forwarding) dai thread all'thread dell'applicazione stessa.
Il bus è un thread indipendente da gstream

ogni pipeline contiene gia di per se un bus, e quindi l'unico cosa da fare per l'app è istanziare un hadler dei
messaggi provenientei dal sistema bus. Quando il mainloop sta girando questo fa il check di nuovi messaggi


