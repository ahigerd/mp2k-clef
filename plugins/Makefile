ROOTPATH := ../
include ../config.mak

OBJS_R = $(filter-out %_d.o ../$(BUILDPATH)/main.o,$(wildcard ../$(BUILDPATH)/*.o ../$(BUILDPATH)/*/*.o))
OBJS_D = $(filter-out ../$(BUILDPATH)/main_d.o,$(wildcard ../$(BUILDPATH)/*_d.o ../$(BUILDPATH)/*/*_d.o))

../aud_$(PLUGIN_NAME).$(DLL): $(OBJS_R) ../seq2wav/$(BUILDPATH)/libseq2wav.a $(wildcard ../seq2wav/includes/plugins/*.h) s2wplugin.cpp Makefile
	$(CXX) -shared -o $@ $(CXXFLAGS_R) -DBUILD_AUDACIOUS s2wplugin.cpp $(shell pkg-config --cflags --libs audacious) $(OBJS_R) $(LDFLAGS_R)
	strip $@

../aud_$(PLUGIN_NAME)_d.$(DLL): $(OBJS_D) ../seq2wav/$(BUILDPATH)/libseq2wav_d.$(DLL) $(wildcard ../seq2wav/includes/plugins/*.h) s2wplugin.cpp Makefile
	$(CXX) -shared -o $@ $(CXXFLAGS_D) -DBUILD_AUDACIOUS s2wplugin.cpp $(shell pkg-config --cflags --libs audacious) $(OBJS_D) $(LDFLAGS_D)

../in_$(PLUGIN_NAME).$(DLL): $(OBJS_R) ../seq2wav/$(BUILDPATH)/libseq2wav.a $(wildcard ../seq2wav/includes/plugins/*.h) s2wplugin.cpp Makefile
	$(CXX) -shared -o $@ $(CXXFLAGS_R) -DBUILD_WINAMP s2wplugin.cpp $(OBJS_R) $(LDFLAGS_R)
	strip $@

../in_$(PLUGIN_NAME)_d.$(DLL): $(OBJS_D) ../seq2wav/$(BUILDPATH)/libseq2wav_d.$(DLL) $(wildcard ../seq2wav/includes/plugins/*.h) s2wplugin.cpp Makefile
	$(CXX) -shared -o $@ $(CXXFLAGS_D) -DBUILD_WINAMP s2wplugin.cpp $(OBJS_D) $(LDFLAGS_D)

FORCE:
