ROOTPATH := ../
include ../config.mak

SOURCES = $(wildcard *.cpp */*.cpp)
OBJS_R = $(patsubst %.cpp, ../$(BUILDPATH)/%.o, $(SOURCES))
OBJS_D = $(patsubst %.cpp, ../$(BUILDPATH)/%_d.o, $(SOURCES))

../$(PLUGIN_NAME)$(EXE): ../seq2wav/$(BUILDPATH)/libseq2wav.a $(OBJS_R)
	$(CXX) -o $@ $(OBJS_R) $(LDFLAGS_R)
	strip $@

../$(PLUGIN_NAME)_d$(EXE): ../seq2wav/$(BUILDPATH)/libseq2wav_d.a $(OBJS_D)
	$(CXX) -o $@ $(OBJS_D) $(LDFLAGS_D)

../$(BUILDPATH)/lib$(PLUGIN_NAME).a: ../seq2wav/$(BUILDPATH)/libseq2wav.a $(OBJS_R)
	rm -f $@
	gcc-ar -rc $@ $(OBJS_R)

../$(BUILDPATH)/lib$(PLUGIN_NAME)_d.a: ../seq2wav/$(BUILDPATH)/libseq2wav_d.a $(OBJS_D)
	rm -f $@
	gcc-ar -rc $@ $(OBJS_D)

../$(BUILDPATH)/%.o: %.cpp Makefile ../config.mak
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_R) -c -o $@ $<

../$(BUILDPATH)/%_d.o: %.cpp Makefile ../config.mak
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_D) -c -o $@ $<

../$(BUILDPATH)/Makefile.d: main.cpp $(SOURCES) $(wildcard *.h */*.h) Makefile ../config.mak
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MT ../$(BUILDPATH)/main.o -MM -MF - main.cpp > $@
	$(CXX) $(CXXFLAGS) -MT ../$(BUILDPATH)/main_d.o -MM -MF - main.cpp >> $@
	$(foreach src, $(SOURCES), $(CXX) $(CXXFLAGS) -MT $(patsubst %.cpp, ../$(BUILDPATH)/%.o, $(src)) -MM -MF - $(src) >> $@;)
	$(foreach src, $(SOURCES), $(CXX) $(CXXFLAGS) -MT $(patsubst %.cpp, ../$(BUILDPATH)/%_d.o, $(src)) -MM -MF - $(src) >> $@;)

include ../$(BUILDPATH)/Makefile.d

FORCE:
