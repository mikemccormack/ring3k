
HEADERS += \
	registryitem.h \
	registrymodel.h \
	registryvalue.h

SOURCES += \
	regedit.cpp \
	registryitem.cpp \
	registrymodel.cpp \
	registryvalue.cpp

INCLUDEPATH += ../libntreg

LIBS += ../libntreg/libntreg.a

CONFIG += qt

