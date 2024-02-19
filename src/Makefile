ROOTPATH := ../
include ../config.mak

SOURCES = $(filter-out main.cpp,$(wildcard *.cpp */*.cpp))
OBJS_R = $(patsubst %.cpp, ../$(BUILDPATH)/%.o, $(SOURCES))
OBJS_D = $(patsubst %.cpp, ../$(BUILDPATH)/%_d.o, $(SOURCES))
MAIN_R = ../$(BUILDPATH)/main.o
MAIN_D = ../$(BUILDPATH)/main_d.o

../$(PLUGIN_NAME)$(EXE): ../libclef/$(BUILDPATH)/libclef.a $(OBJS_R) $(MAIN_R)
	$(CXX) -o $@ $(MAIN_R) $(OBJS_R) $(LDFLAGS_R)
	strip $@

../$(PLUGIN_NAME)_d$(EXE): ../libclef/$(BUILDPATH)/libclef_d.a $(OBJS_D) $(MAIN_D)
	$(CXX) -o $@ $(MAIN_D) $(OBJS_D) $(LDFLAGS_D)

../$(BUILDPATH)/lib$(PLUGIN_NAME).a: ../libclef/$(BUILDPATH)/libclef.a $(filter-out ../$(BUILDPATH)/main.o,$(OBJS_R))
	rm -f $@
	gcc-ar -rc $@ $(OBJS_R)

../$(BUILDPATH)/lib$(PLUGIN_NAME)_d.a: ../libclef/$(BUILDPATH)/libclef_d.a $(filter-out ../$(BUILDPATH)/main_d.o,$(OBJS_R))
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
