
HEADERS += registryitem.h

SOURCES += \
	regedit.cpp \
	registryitem.cpp

INCLUDEPATH += ../libntreg

LIBS += ../libntreg/libntreg.a

CONFIG += qt

