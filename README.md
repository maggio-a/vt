VideoTracker
============

Progetto finale del corso di Reti ad hoc e di sensori, Corso di Laurea Magistrale in Informatica, Università di Pisa.

Anno accademico 2014-2015

[Writeup](VideoTracker/TODO.txt)

Dipendenze
==========

* OpenCV (http://opencv.org/)
* RaspiCam C++ (http://www.uco.es/investiga/grupos/ava/node/40)

La versione di RaspiCam C++ contenuta in questo repository corregge un bug che impediva di configurare correttamente l'esposizione (shutter speed) della telecamera.

Compilazione
============

Per compilare OpenCV e RaspiCam C++ si fa riferimento alla relativa documentazione. Per compilare il progetto su Raspbian usando cmake:

    cd VideoTracker
    mkdir build
    cd build
    cmake ..
    make

Se si dispone di una webcam USB è possibile disabilitare le dipendenze da RaspiCam C++ passando a cmake lo switch `-DRPI=OFF`. In questo caso come sorgente video viene usato un oggetto `cv::VideoCapture`:

    cd VideoTracker
    mkdir build
    cd build
    cmake -DRPI=OFF ..
    make

