GST Event and Pad

I dati che flusicono attraverso la pipeline sono una combinazione di buffer e eventi:
Buffer: contegnono i dati multimediali
Event: le meta informazioni

Buffer:
Contiene i dati che fluisco nella pipline creata. Un elmento src crea un nuovo buffer e lo passa all'elmento
succesivo per mezzo del Pad. I buffer sono creati automaticamente dagli elementi

ogni buffer contiene
    Un puntatore alla regione di memoria
    Un timeStamp
    Un refcount, ovvero un contatore che indica quanti elementi stanno usando quel buffer, se nussen elemento
                 sta usando il buffer, il buffer viene distrutto
    Buffer flags

Es : buffer creato --> aloocazione memoria --> data in it --> pass to next element
     element legge il buffer -> applica qualcosa -> dereferenzia il buffer --> rialscio dei dati e distruzione


Event

informazioni sui buffer:
    seek
    flushes
    EOS
