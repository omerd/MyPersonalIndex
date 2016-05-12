include(forms/forms.pri)
include(classes/classes.pri)
include(state/state.pri)
include(models/models.pri)
include(ui/ui.pri)
include(usercontrols/usercontrols.pri)
include(prices/prices.pri)
include(calculators/calculators.pri)

VERSION = 320
DEFINES += APP_VERSION="$$VERSION"
DEFINES += QWT_DLL=1
DEFINES += QwtDll=1

win32{
    INCLUDEPATH += ../../qwt-6.1.2/src

    CONFIG(debug, debug|release): LIBS += ../../../build-qwt-QT_4_8_3-Debug/lib/qwtd.lib
        else: LIBS += ../../../build-qwt-QT_4_8_3-Debug/lib/qwt.lib
}

unix:!macx{
    INCLUDEPATH += /usr/include/qwt/
    LIBS += /usr/lib/libqwt.so
}

macx{
    INCLUDEPATH += shared/qwt
    LIBS += shared/qwt/lib/libqwt.dylib
}

CONFIG(debug, debug|release): DEFINES += CLOCKTIME

RESOURCES += icons/icons.qrc

QT += sql \
    network

SOURCES += main.cpp
RC_FILE = mypersonalindex.rc

unix {
    # in debug mode...
    CONFIG(debug, debug|release):DESTDIR = debug
    else:DESTDIR = release
}
