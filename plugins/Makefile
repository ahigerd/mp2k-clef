ROOTPATH := ../
include ../config.mak

OBJS_R = $(filter-out ../$(BUILDPATH)/gui/% ../$(BUILDPATH)/gui_d/% %_d.o ../$(BUILDPATH)/main.o,$(wildcard ../$(BUILDPATH)/*.o ../$(BUILDPATH)/*/*.o))
OBJS_D = $(filter-out ../$(BUILDPATH)/gui/% ../$(BUILDPATH)/gui_d/% ../$(BUILDPATH)/main_d.o,$(wildcard ../$(BUILDPATH)/*_d.o ../$(BUILDPATH)/*/*_d.o))

../aud_$(PLUGIN_NAME).$(DLL): $(OBJS_R) ../libclef/$(BUILDPATH)/libclef.a $(wildcard ../libclef/src/plugins/*.h) clefplugin.cpp Makefile
	$(CXX) -shared -o $@ $(CXXFLAGS_R) -DBUILD_AUDACIOUS clefplugin.cpp $(shell pkg-config --cflags --libs audacious) $(OBJS_R) $(LDFLAGS_R)
	strip $@

../aud_$(PLUGIN_NAME)_d.$(DLL): $(OBJS_D) ../libclef/$(BUILDPATH)/libclef_d.$(DLL) $(wildcard ../libclef/src/plugins/*.h) clefplugin.cpp Makefile
	$(CXX) -shared -o $@ $(CXXFLAGS_D) -DBUILD_AUDACIOUS clefplugin.cpp $(shell pkg-config --cflags --libs audacious) $(OBJS_D) $(LDFLAGS_D)

../in_$(PLUGIN_NAME).$(DLL): $(OBJS_R) ../libclef/$(BUILDPATH)/libclef.a $(wildcard ../libclef/src/plugins/*.h) clefplugin.cpp Makefile
	$(CXX) -shared -o $@ $(CXXFLAGS_R) -DBUILD_WINAMP clefplugin.cpp $(OBJS_R) $(LDFLAGS_R)
	strip $@

../in_$(PLUGIN_NAME)_d.$(DLL): $(OBJS_D) ../libclef/$(BUILDPATH)/libclef_d.$(DLL) $(wildcard ../libclef/src/plugins/*.h) clefplugin.cpp Makefile
	$(CXX) -shared -o $@ $(CXXFLAGS_D) -DBUILD_WINAMP clefplugin.cpp $(OBJS_D) $(LDFLAGS_D)

../$(PLUGIN_NAME).clap: $(OBJS_R) ../libclef/$(BUILDPATH)/libclef.a $(wildcard ../libclef/src/plugins/*.h) clefplugin.cpp ../libclef/src/plugin/clapplugin.cpp Makefile
	$(CXX) -shared -o $@ $(CXXFLAGS_R) -DBUILD_CLAP -Iclap/include clefplugin.cpp ../libclef/src/plugin/clapplugin.cpp $(OBJS_R) $(LDFLAGS_R)

../$(PLUGIN_NAME)_d.clap: $(OBJS_D) ../libclef/$(BUILDPATH)/libclef_d.a $(wildcard ../libclef/src/plugins/*.h) clefplugin.cpp ../libclef/src/plugin/clapplugin.cpp Makefile
	$(CXX) -shared -o $@ $(CXXFLAGS_D) -DBUILD_CLAP -Iclap/include clefplugin.cpp ../libclef/src/plugin/clapplugin.cpp $(OBJS_D) $(LDFLAGS_D)

FORCE:
