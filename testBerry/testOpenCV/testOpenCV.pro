#-------------------------------------------------
#
# Project created by QtCreator 2014-04-03T13:28:21
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = testOpenCV
CONFIG   += console
CONFIG   -= app_bundle
QMAKE_CXXFLAGS += -std=c++0x

TEMPLATE = app
INCLUDEPATH += /usr/include/opencv
INCLUDEPATH += /home/pi/git/robidouille/raspicam_cv
LIBS += -L/usr/lib \
-lopencv_core \
-lopencv_imgproc \
-lopencv_highgui \
-lopencv_ml \
-lopencv_video \
-lopencv_features2d \
-lopencv_calib3d \
-lopencv_objdetect \
-lopencv_contrib \
-lopencv_legacy \
-lopencv_flann \
-L/home/pi/git/robidouille/raspicam_cv \
-lraspicamcv \
-L/home/pi/git/raspberrypi/userland/build/lib \
-lmmal_core \
-lmmal \
-lmmal_util \
-lvcos \
-lbcm_host \

SOURCES += \
    SubstractorTest.cpp
