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

libclef/src:
	git submodule update --init --recursive

ifeq ($(CROSS),msvc)
depends.mak: libclef/src $(wildcard src/*.h src/*.cpp src/*/*.h src/*/*.cpp plugins/*.cpp libclef/src/*.cpp libclef/src/*/*.h libclef/src/*/*.cpp libclef/src/*/*.h)
	$(WINE) cmd /c buildvs.cmd depends

foo_input_$(PLUGIN_NAME).$(DLL) in_$(PLUGIN_NAME).$(DLL) aud_$(PLUGIN_NAME).$(DLL) clean: FORCE depends.mak
	MAKEFLAGS= $(WINE) nmake /f msvc32.mak $@
else
libclef/$(BUILDPATH)/libclef.a libclef/$(BUILDPATH)/libclef_d.a: libclef/src $(wildcard libclef/src/*.cpp libclef/*/*.h libclef/src/*/*.cpp libclef/*/*/*.h)
	$(MAKE) -C libclef $(BUILDPATH)/$(notdir $@)

$(BUILDPATH)/Makefile.d: $(wildcard src/*.cpp src/*/*.cpp src/*.h src/*/*.h) Makefile src/Makefile config.mak
	$(MAKE) -C src ../$@

$(PLUGIN_NAME)$(EXE): src/Makefile $(BUILDPATH)/Makefile.d config.mak libclef/$(BUILDPATH)/libclef.a
	$(MAKE) -C src ../$@

$(PLUGIN_NAME)_d$(EXE): src/Makefile $(BUILDPATH)/Makefile.d config.mak libclef/$(BUILDPATH)/libclef_d.a
	$(MAKE) -C src ../$@

$(BUILDPATH)/lib$(PLUGIN_NAME).a: src/Makefile $(BUILDPATH)/Makefile.d config.mak libclef/$(BUILDPATH)/libclef.a
	$(MAKE) -C src ../$@

$(BUILDPATH)/lib$(PLUGIN_NAME)_d.a: src/Makefile $(BUILDPATH)/Makefile.d config.mak libclef/$(BUILDPATH)/libclef_d.a
	$(MAKE) -C src ../$@

gui/Makefile: gui/gui.pro libclef/gui/gui.pri Makefile config.mak
	cd gui && $(QMAKE) BUILDPATH=../$(BUILDPATH) PLUGIN_NAME=$(PLUGIN_NAME) CLEF_LDFLAGS="$(LDFLAGS_R)"

gui/Makefile.debug: gui/gui.pro libclef/gui/gui.pri Makefile config.mak
	cd gui && $(QMAKE) -o Makefile.debug BUILD_DEBUG=1 BUILDPATH=../$(BUILDPATH) PLUGIN_NAME=$(PLUGIN_NAME) CLEF_LDFLAGS="$(LDFLAGS_D)"

$(PLUGIN_NAME)_gui$(EXE): src/Makefile $(BUILDPATH)/Makefile.d config.mak libclef/$(BUILDPATH)/libclef.a gui/Makefile $(BUILDPATH)/lib$(PLUGIN_NAME).a
	$(MAKE) -C gui

$(PLUGIN_NAME)_gui_d$(EXE): src/Makefile $(BUILDPATH)/Makefile.d config.mak libclef/$(BUILDPATH)/libclef_d.a gui/Makefile.debug $(BUILDPATH)/lib$(PLUGIN_NAME)_d.a
	$(MAKE) -C gui -f Makefile.debug

aud_$(PLUGIN_NAME).$(DLL): $(PLUGIN_NAME)$(EXE) libclef/$(BUILDPATH)/libclef.a plugins/Makefile config.mak plugins/clefplugin.cpp
	$(MAKE) -C plugins ../$@

aud_$(PLUGIN_NAME)_d.$(DLL): libclef/$(BUILDPATH)/libclef_d.a $(PLUGIN_NAME)_d$(EXE) plugins/Makefile config.mak plugins/clefplugin.cpp
	$(MAKE) -C plugins ../$@

ifeq ($(OS),Windows_NT)
in_$(PLUGIN_NAME).$(DLL): $(PLUGIN_NAME)$(EXE) libclef/$(BUILDPATH)/libclef.a plugins/Makefile config.mak plugins/clefplugin.cpp
	$(MAKE) -C plugins ../$@

in_$(PLUGIN_NAME)_d.$(DLL): $(PLUGIN_NAME)_d$(EXE) libclef/$(BUILDPATH)/libclef_d.a plugins/Makefile config.mak plugins/clefplugin.cpp
	$(MAKE) -C plugins ../$@
else
in_$(PLUGIN_NAME).dll in_$(PLUGIN_NAME)_d.dll: FORCE
	$(MAKE) CROSS=mingw $@
endif

$(PLUGIN_NAME).clap: $(PLUGIN_NAME)$(EXE) libclef/$(BUILDPATH)/libclef.a plugins/Makefile config.mak plugins/clefplugin.cpp
	$(MAKE) -C plugins ../$@

$(PLUGIN_NAME)_d.clap: $(PLUGIN_NAME)_d$(EXE) libclef/$(BUILDPATH)/libclef_d.a plugins/Makefile config.mak plugins/clefplugin.cpp
	$(MAKE) -C plugins ../$@

guiclean: FORCE
	-[ -f gui/Makefile ] && $(MAKE) -C gui distclean
	-[ -f gui/Makefile.debug ] && $(MAKE) -C gui -f Makefile.debug distclean

clean: guiclean FORCE
	-rm -f $(BUILDPATH)/*.o $(BUILDPATH)/*/*.o $(BUILDPATH)/Makefile.d
	-rm -f $(PLUGIN_NAME)$(EXE) $(PLUGIN_NAME)_d$(EXE) $(PLUGIN_NAME)_gui$(EXE) $(PLUGIN_NAME)_gui_d$(EXE) *.$(DLL)
	-$(MAKE) -C libclef clean
endif

FORCE:
