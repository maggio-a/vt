DIPENDENZE
==========

* OpenCV (http://opencv.org/)
* RaspiCam C++ (http://www.uco.es/investiga/grupos/ava/node/40)

La versione di RaspiCam C++ contenuta in questo repository corregge un bug che impediva di configurare correttamente l'esposizione (shutter speed) della telecamera.

COMPILAZIONE
============

Per compilare OpenCV e RaspiCam C++ si fa riferimento alla relativa documentazione. Per compilare il progetto su Raspbian:

    cd VideoTracker
    mkdir build
    cd build
    cmake ..
    make

È possibile disabilitare le dipendenze da RaspiCam C++ e utilizzare un oggetto `cv::VideoCapture` come sorgente video (utile se si usa ad esempio una webcam USB) passando a cmake lo switch `-DRPI=OFF`:

    cd VideoTracker
    mkdir build
    cd build
    cmake -DRPI=OFF ..
    make



# README #

This README would normally document whatever steps are necessary to get your application up and running.

### What is this repository for? ###

* Quick summary
* Version
* [Learn Markdown](https://bitbucket.org/tutorials/markdowndemo)

### How do I get set up? ###

* Summary of set up
* Configuration
* Dependencies
* Database configuration
* How to run tests
* Deployment instructions

### Contribution guidelines ###

* Writing tests
* Code review
* Other guidelines

### Who do I talk to? ###

* Repo owner or admin
* Other community or team contact