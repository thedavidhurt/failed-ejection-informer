QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    wintoastlib/wintoastlib.cpp

HEADERS += \
    customtoasthandler.h \
    mainwindow.h \
    wintoastlib/wintoastlib.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    sivvus_pendrive_icon_4.png

RESOURCES += \
    image.qrc

RC_ICONS = sivvus_pendrive_icon_4_TnQ_icon.ico
