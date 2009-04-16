
HEADERS += \
	registryitem.h \
	registrymodel.h

SOURCES += \
	regedit.cpp \
	registryitem.cpp \
	registrymodel.cpp

INCLUDEPATH += ../libntreg

LIBS += ../libntreg/libntreg.a

CONFIG += qt

