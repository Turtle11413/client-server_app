QT -= gui
QT += core network

CONFIG += c++17 console
CONFIG -= app_bundle

SOURCES +=	\
	server.cc \
	main.cc

HEADERS +=	\
	server.h

DESTDIR     = build/
MOC_DIR		  = build/moc
OBJECTS_DIR = build/obj

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS+=target
