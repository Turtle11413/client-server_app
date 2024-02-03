QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES +=	\
	client.cc \
	main.cc

HEADERS +=	\
	client.h

DESTDIR     = build/
MOC_DIR		  = build/moc
OBJECTS_DIR = build/obj

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS+=target
