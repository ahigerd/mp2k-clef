isEmpty(BUILDPATH) {
  error("BUILDPATH must be set")
}
BUILDPATH = $$absolute_path($$BUILDPATH)
include($$BUILDPATH/../seq2wav/gui/gui.pri)

HEADERS += channelwidget.h
SOURCES += channelwidget.cpp

SOURCES += main.cpp ../plugins/s2wplugin.cpp
