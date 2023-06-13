include config.mak

cli: $(PLUGIN_NAME)$(EXE)

debug: $(PLUGIN_NAME)_d$(EXE)

gui: $(PLUGIN_NAME)_gui$(EXE)

guidebug: $(PLUGIN_NAME)_gui_d$(EXE)

all: cli plugins gui

plugins: audacious

includes: seq2wav/src $(wildcard seq2wav/src/*.h seq2wav/src/*/*.h)
	$(MAKE) -C seq2wav includes

audacious: aud_$(PLUGIN_NAME).$(DLL)

winamp: in_$(PLUGIN_NAME).dll

foobar: foo_input_$(PLUGIN_NAME).dll

seq2wav/src:
	git submodule update --init --recursive

ifeq ($(CROSS),msvc)
depends.mak: seq2wav/src $(wildcard src/*.h src/*.cpp src/*/*.h src/*/*.cpp plugins/*.cpp seq2wav/src/*.cpp seq2wav/src/*/*.h seq2wav/src/*/*.cpp seq2wav/src/*/*.h)
	$(WINE) cmd /c buildvs.cmd depends

foo_input_$(PLUGIN_NAME).$(DLL) in_$(PLUGIN_NAME).$(DLL) aud_$(PLUGIN_NAME).$(DLL) clean: FORCE depends.mak
	MAKEFLAGS= $(WINE) nmake /f msvc32.mak $@
else
seq2wav/$(BUILDPATH)/libseq2wav.a seq2wav/$(BUILDPATH)/libseq2wav_d.a: seq2wav/src $(wildcard seq2wav/src/*.cpp seq2wav/*/*.h seq2wav/src/*/*.cpp seq2wav/*/*/*.h)
	$(MAKE) -C seq2wav $(BUILDPATH)/$(notdir $@)

$(BUILDPATH)/Makefile.d: $(wildcard src/*.cpp src/*/*.cpp src/*.h src/*/*.h) Makefile src/Makefile config.mak
	$(MAKE) -C src ../$@

$(PLUGIN_NAME)$(EXE): includes src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav.a
	$(MAKE) -C src ../$@

$(PLUGIN_NAME)_d$(EXE): includes src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav_d.a
	$(MAKE) -C src ../$@

$(BUILDPATH)/lib$(PLUGIN_NAME).a: includes src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav.a
	$(MAKE) -C src ../$@

$(BUILDPATH)/lib$(PLUGIN_NAME)_d.a: includes src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav_d.a
	$(MAKE) -C src ../$@

gui/Makefile: includes gui/gui.pro seq2wav/gui/gui.pri Makefile config.mak
	cd gui && qmake BUILDPATH=../$(BUILDPATH) PLUGIN_NAME=$(PLUGIN_NAME) S2W_LDFLAGS="$(LDFLAGS_R)"

gui/Makefile.debug: includes gui/gui.pro seq2wav/gui/gui.pri Makefile config.mak
	cd gui && qmake -o Makefile.debug BUILD_DEBUG=1 BUILDPATH=../$(BUILDPATH) PLUGIN_NAME=$(PLUGIN_NAME) S2W_LDFLAGS="$(LDFLAGS_D)"

$(PLUGIN_NAME)_gui$(EXE): includes src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav.a gui/Makefile $(BUILDPATH)/lib$(PLUGIN_NAME).a
	$(MAKE) -C gui

$(PLUGIN_NAME)_gui_d$(EXE): includes src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav_d.a gui/Makefile.debug $(BUILDPATH)/lib$(PLUGIN_NAME)_d.a
	$(MAKE) -C gui -f Makefile.debug

aud_$(PLUGIN_NAME).$(DLL): includes $(PLUGIN_NAME)$(EXE) seq2wav/$(BUILDPATH)/libseq2wav.a plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@

aud_$(PLUGIN_NAME)_d.$(DLL): includes seq2wav/$(BUILDPATH)/libseq2wav_d.a $(PLUGIN_NAME)_d$(EXE) plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@

ifeq ($(OS),Windows_NT)
in_$(PLUGIN_NAME).$(DLL): includes $(PLUGIN_NAME)$(EXE) seq2wav/$(BUILDPATH)/libseq2wav.a plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@

in_$(PLUGIN_NAME)_d.$(DLL): includes $(PLUGIN_NAME)_d$(EXE) seq2wav/$(BUILDPATH)/libseq2wav_d.a plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@
else
in_$(PLUGIN_NAME).dll in_$(PLUGIN_NAME)_d.dll: FORCE
	$(MAKE) CROSS=mingw $@
endif

guiclean: FORCE
	-[ -f gui/Makefile ] && $(MAKE) -C gui distclean
	-[ -f gui/Makefile.debug ] && $(MAKE) -C gui -f Makefile.debug distclean

clean: guiclean FORCE
	-rm -f $(BUILDPATH)/*.o $(BUILDPATH)/*/*.o $(BUILDPATH)/Makefile.d
	-rm -f $(PLUGIN_NAME)$(EXE) $(PLUGIN_NAME)_d$(EXE) $(PLUGIN_NAME)_gui$(EXE) $(PLUGIN_NAME)_gui_d$(EXE) *.$(DLL)
	-$(MAKE) -C seq2wav clean
endif

FORCE:
