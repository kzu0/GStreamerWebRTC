#-------------------------------------------------
#
# Project created by QtCreator 2023-06-15T15:11:01
#
#-------------------------------------------------

QT       += core gui network websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GStreamerWebRTC
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

CONFIG += link_pkgconfig

PKGCONFIG += gstreamer-1.0
PKGCONFIG += gstreamer-pbutils-1.0
PKGCONFIG += gstreamer-video-1.0
PKGCONFIG += gstreamer-audio-1.0
PKGCONFIG += gstreamer-app-1.0
PKGCONFIG += gstreamer-plugins-bad-1.0
PKGCONFIG += gstreamer-webrtc-1.0
PKGCONFIG += gstreamer-sdp-1.0
PKGCONFIG += gstreamer-plugins-base-1.0
#PKGCONFIG += gstreamer-plugins-good-1.0
PKGCONFIG += json-c

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    QGstWebRTCObject.cpp \
    QSignalingServer.cpp

HEADERS += \
        mainwindow.h \
    QGstWebRTCObject.h \
    QSignalingServer.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    HTML/js/lib/adapter.js \
    HTML/js/main.js \
    HTML/css/main.css \
    HTML/index.html
