
TEMPLATE	= lib
CONFIG	+= qt plugin warn_off
QMAKE_CXXFLAGS += /MP
#CONFIG	+= x86_64

BOOSTPATH = $$(BOOST_PATH)
QTPATH = $$(QTDIR)

VAA3DPATH = ../../../../v3d_external/v3d_main
IMGMANAGERPATH = ../v3d_imgManagerMK
STATSLEARNERPATH = ../StatsLearner
V3DTOOLPATH = ../../../released_plugins/v3d_plugins

INCLUDEPATH += $$BOOSTPATH
INCLUDEPATH += $$QTPATH/demos/shared
INCLUDEPATH += $$QTPATH/include/QtOpenGL
INCLUDEPATH += $$IMGMANAGERPATH
INCLUDEPATH += $$IMGMANAGERPATH/imgAnalyzer
INCLUDEPATH += $$IMGMANAGERPATH/imgProcessor
INCLUDEPATH += ../NeuronStructNavigator
INCLUDEPATH += $$STATSLEARNERPATH
INCLUDEPATH += ./
INCLUDEPATH += $$V3DTOOLPATH/swc2mask_cylinder
INCLUDEPATH += $$VAA3DPATH/v3d
INCLUDEPATH	+= $$VAA3DPATH/basic_c_fun
INCLUDEPATH += $$VAA3DPATH/neuron_editing
INCLUDEPATH += $$VAA3DPATH/common_lib/include
INCLUDEPATH += $$VAA3DPATH/3drenderer

LIBS += -L$$BOOSTPATH/lib64-msvc-12.0
LIBS += -L$$IMGMANAGERPATH -lv3d_imgManagerMK
LIBS += -L../NeuronStructNavigator -lNeuronStructNavigator

FORMS += fragmentedTraceUI.ui
FORMS += progressMonitor.ui

RESOURCES += FragTracer_Resource.qrc

HEADERS	+= Fragmented_Auto-trace_plugin.h
HEADERS += FragTraceControlPanel.h
HEADERS += FragTraceManager.h
HEADERS += $$V3DTOOLPATH/swc2mask_cylinder/my_surf_objs.h
HEADERS += FragTracer_Define.h
HEADERS += FragmentEditor.h
HEADERS += FragmentPostProcessor.h
HEADERS += FragTraceImgProcessor.h
HEADERS += FragTraceTester.h
HEADERS += progressMonitor.h

SOURCES	+= $$VAA3DPATH/basic_c_fun/v3d_message.cpp
SOURCES	+= Fragmented_Auto-trace_plugin.cpp
SOURCES += FragTraceControlPanel.cpp
SOURCES += FragTraceManager.cpp
SOURCES += FragmentEditor.cpp
SOURCES += FragmentPostProcessor.cpp
SOURCES += FragTraceImgProcessor.cpp
SOURCES += FragTraceTester.cpp
SOURCES += progressMonitor.cpp

TARGET	= $$qtLibraryTarget(Fragmented_Auto-trace)
DESTDIR	= ../../../../v3d_external/bin/plugins/Fragmented_Auto-trace/
