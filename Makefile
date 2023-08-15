_default: cli

include config.mak

cli: $(PLUGIN_NAME)$(EXE)

debug: $(PLUGIN_NAME)_d$(EXE)

gui: $(PLUGIN_NAME)_gui$(EXE)

guidebug: $(PLUGIN_NAME)_gui_d$(EXE)

all: cli plugins gui

plugins: audacious

audacious: aud_$(PLUGIN_NAME).$(DLL)

winamp: in_$(PLUGIN_NAME).dll

foobar: foo_input_$(PLUGIN_NAME).dll

clap: $(PLUGIN_NAME).clap

clapdebug: $(PLUGIN_NAME)_d.clap

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

$(PLUGIN_NAME)$(EXE): src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav.a
	$(MAKE) -C src ../$@

$(PLUGIN_NAME)_d$(EXE): src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav_d.a
	$(MAKE) -C src ../$@

$(BUILDPATH)/lib$(PLUGIN_NAME).a: src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav.a
	$(MAKE) -C src ../$@

$(BUILDPATH)/lib$(PLUGIN_NAME)_d.a: src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav_d.a
	$(MAKE) -C src ../$@

gui/Makefile: gui/gui.pro seq2wav/gui/gui.pri Makefile config.mak
	cd gui && $(QMAKE) BUILDPATH=../$(BUILDPATH) PLUGIN_NAME=$(PLUGIN_NAME) S2W_LDFLAGS="$(LDFLAGS_R)"

gui/Makefile.debug: gui/gui.pro seq2wav/gui/gui.pri Makefile config.mak
	cd gui && $(QMAKE) -o Makefile.debug BUILD_DEBUG=1 BUILDPATH=../$(BUILDPATH) PLUGIN_NAME=$(PLUGIN_NAME) S2W_LDFLAGS="$(LDFLAGS_D)"

$(PLUGIN_NAME)_gui$(EXE): src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav.a gui/Makefile $(BUILDPATH)/lib$(PLUGIN_NAME).a
	$(MAKE) -C gui

$(PLUGIN_NAME)_gui_d$(EXE): src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav_d.a gui/Makefile.debug $(BUILDPATH)/lib$(PLUGIN_NAME)_d.a
	$(MAKE) -C gui -f Makefile.debug

aud_$(PLUGIN_NAME).$(DLL): $(PLUGIN_NAME)$(EXE) seq2wav/$(BUILDPATH)/libseq2wav.a plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@

aud_$(PLUGIN_NAME)_d.$(DLL): seq2wav/$(BUILDPATH)/libseq2wav_d.a $(PLUGIN_NAME)_d$(EXE) plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@

ifeq ($(OS),Windows_NT)
in_$(PLUGIN_NAME).$(DLL): $(PLUGIN_NAME)$(EXE) seq2wav/$(BUILDPATH)/libseq2wav.a plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@

in_$(PLUGIN_NAME)_d.$(DLL): $(PLUGIN_NAME)_d$(EXE) seq2wav/$(BUILDPATH)/libseq2wav_d.a plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@
else
in_$(PLUGIN_NAME).dll in_$(PLUGIN_NAME)_d.dll: FORCE
	$(MAKE) CROSS=mingw $@
endif

$(PLUGIN_NAME).clap: $(PLUGIN_NAME)$(EXE) seq2wav/$(BUILDPATH)/libseq2wav.a plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@

$(PLUGIN_NAME)_d.clap: $(PLUGIN_NAME)_d$(EXE) seq2wav/$(BUILDPATH)/libseq2wav_d.a plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@

guiclean: FORCE
	-[ -f gui/Makefile ] && $(MAKE) -C gui distclean
	-[ -f gui/Makefile.debug ] && $(MAKE) -C gui -f Makefile.debug distclean

clean: guiclean FORCE
	-rm -f $(BUILDPATH)/*.o $(BUILDPATH)/*/*.o $(BUILDPATH)/Makefile.d
	-rm -f $(PLUGIN_NAME)$(EXE) $(PLUGIN_NAME)_d$(EXE) $(PLUGIN_NAME)_gui$(EXE) $(PLUGIN_NAME)_gui_d$(EXE) *.$(DLL)
	-$(MAKE) -C seq2wav clean
endif

FORCE:
