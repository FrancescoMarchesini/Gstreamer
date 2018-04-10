sudo apt-get install libqtgstreamer-1.0-0  libqtgstreamer-dev

Guida alla lettura di gst

L'oggetto piu importante di gstream è GstElement, dal quale sono derivati tutti gli altri.

un elemento è un blocco che può avere un input ed un ouput

Source elments: questo elemento genera i dati che saranno utilizzati succesivamente
nella pipeline, es lettura file da disco, oppure accesso alla scheda audio.

Filtri e filtri-vari: questi elementi hanno sia input che output, ed operano su dato proveniente
dagli input(sink), e danno il dato hai propri ouput.

Sink Elements: sono gli elementi finali della pipeline. accettano data ma non producono ninete
