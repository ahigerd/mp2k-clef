include config.mak

cli: $(PLUGIN_NAME)$(EXE)

debug: $(PLUGIN_NAME)_d$(EXE)

all: cli plugins

plugins: audacious

includes: $(wildcard seq2wav/src/*.h seq2wav/src/*/*.h)
	$(MAKE) -C seq2wav includes

audacious: aud_$(PLUGIN_NAME).$(DLL)

winamp: in_$(PLUGIN_NAME).dll

foobar: foo_input_$(PLUGIN_NAME).dll

seq2wav/src:
	git submodules update --init --recursive

ifeq ($(CROSS),msvc)
depends.mak:  seq2wav/src $(wildcard src/*.h src/*.cpp src/*/*.h src/*/*.cpp plugins/*.cpp seq2wav/src/*.cpp seq2wav/src/*/*.h seq2wav/src/*/*.cpp seq2wav/src/*/*.h)
	$(WINE) cmd /c buildvs.cmd depends

foo_input_$(PLUGIN_NAME).$(DLL) in_$(PLUGIN_NAME).$(DLL) aud_$(PLUGIN_NAME).$(DLL) clean: FORCE depends.mak
	MAKEFLAGS= $(WINE) nmake /f msvc32.mak $@
else
seq2wav/$(BUILDPATH)/libseq2wav.a seq2wav/$(BUILDPATH)/libseq2wav_d.$(DLL): seq2wav/src $(wildcard seq2wav/src/*.cpp seq2wav/*/*.h seq2wav/src/*/*.cpp seq2wav/*/*/*.h)
	$(MAKE) -C seq2wav $(BUILDPATH)/$(notdir $@)

$(BUILDPATH)/Makefile.d: $(wildcard src/*.cpp src/*/*.cpp src/*.h src/*/*.h) Makefile src/Makefile config.mak
	$(MAKE) -C src ../$@

$(PLUGIN_NAME)$(EXE): includes src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav.a
	$(MAKE) -C src ../$(PLUGIN_NAME)$(EXE)

$(PLUGIN_NAME)_d$(EXE): includes src/Makefile $(BUILDPATH)/Makefile.d config.mak seq2wav/$(BUILDPATH)/libseq2wav_d.$(DLL)
	$(MAKE) -C src ../$(PLUGIN_NAME)_d$(EXE)

aud_$(PLUGIN_NAME).$(DLL): includes $(PLUGIN_NAME)$(EXE) seq2wav/$(BUILDPATH)/libseq2wav.a plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@

aud_$(PLUGIN_NAME)_d.$(DLL): includes seq2wav/$(BUILDPATH)/libseq2wav_d.$(DLL) $(PLUGIN_NAME)_d$(EXE) plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@

ifeq ($(OS),Windows_NT)
in_$(PLUGIN_NAME).$(DLL): includes $(PLUGIN_NAME)$(EXE) seq2wav/$(BUILDPATH)/libseq2wav.a plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@

in_$(PLUGIN_NAME)_d.$(DLL): includes $(PLUGIN_NAME)_d$(EXE) seq2wav/$(BUILDPATH)/libseq2wav_d.$(DLL) plugins/Makefile config.mak plugins/s2wplugin.cpp
	$(MAKE) -C plugins ../$@
else
in_$(PLUGIN_NAME).dll in_$(PLUGIN_NAME)_d.dll: FORCE
	$(MAKE) CROSS=mingw $@
endif

clean: FORCE
	-rm -f $(BUILDPATH)/*.o $(BUILDPATH)/*/*.o $(BUILDPATH)/Makefile.d
	-rm -f $(PLUGIN_NAME)$(EXE) $(PLUGIN_NAME)_d$(EXE) *.$(DLL)
	-$(MAKE) -C seq2wav clean
endif

FORCE:
