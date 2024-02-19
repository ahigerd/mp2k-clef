PLUGIN_NAME = sample

cli: "$(PLUGIN_NAME).exe"

all: cli plugins

plugins: winamp foobar

audacious: "aud_$(PLUGIN_NAME).dll"

winamp: "in_$(PLUGIN_NAME).dll"

foobar: "foo_input_$(PLUGIN_NAME).dll"

libclef\src:
	git submodules update --init --recursive

!include depends.mak

.cpp.obj:
	$(CPP) /std:c++latest /DUNICODE /D_UNICODE /DNDEBUG /O2 /EHsc /I src /I libclef\src /I plugins\foobar2000 /I plugins /c /Fo$@ $<

plugins\foobarplugin.obj: plugins\clefplugin.cpp libclef\src\plugins\foobarplugin.h FORCE
	$(CPP) /std:c++latest /DUNICODE /D_UNICODE /DNDEBUG /DBUILD_FOOBAR /O2 /EHsc /I src /I libclef\src /I plugins\foobar2000 /I plugins /c /Fo$@ plugins\clefplugin.cpp

plugins\audaciousplugin.obj: plugins\clefplugin.cpp libclef\src\plugins\audaciousplugin.h FORCE
	$(CPP) /std:c++latest /DUNICODE /D_UNICODE /DNDEBUG /DBUILD_AUDACIOUS /O2 /EHsc /I src /I libclef\src /c /Fo$@ plugins\clefplugin.cpp

plugins\winampplugin.obj: plugins\clefplugin.cpp libclef\src\plugins\winampplugin.h FORCE
	$(CPP) /std:c++latest /DUNICODE /D_UNICODE /DNDEBUG /DBUILD_WINAMP /O2 /EHsc /I src /I libclef\src /c /Fo$@ plugins\clefplugin.cpp

"$(PLUGIN_NAME).exe": src\main.obj
	link.exe /subsystem:console /out:$@ $**

"aud_$(PLUGIN_NAME).dll": plugins\audaciousplugin.obj
	link.exe /dll user32.lib /out:$@ $**

"in_$(PLUGIN_NAME).dll": plugins\winampplugin.obj
	link.exe /dll user32.lib /out:$@ $**

"foo_input_$(PLUGIN_NAME).dll": plugins\foobarplugin.obj plugins\foobar2000\SDK\input.obj plugins\foobar2000\SDK\guids.obj \
		plugins\foobar2000\SDK\file_info.obj plugins\foobar2000\SDK\file_info_impl.obj plugins\foobar2000\SDK\replaygain_info.obj \
		plugins\foobar2000\SDK\console.obj plugins\foobar2000\SDK\service.obj plugins\foobar2000\SDK\cfg_var.obj \
		plugins\foobar2000\SDK\abort_callback.obj plugins\foobar2000\SDK\audio_chunk.obj plugins\foobar2000\SDK\audio_chunk_channel_config.obj \
		plugins\foobar2000\SDK\filesystem.obj plugins\foobar2000\SDK\filesystem_helper.obj plugins\foobar2000\SDK\componentversion.obj \
		plugins\foobar2000\SDK\main_thread_callback.obj plugins\foobar2000\SDK\utility.obj plugins\foobar2000\SDK\playable_location.obj \
		plugins\pfc\string_base.obj plugins\pfc\other.obj plugins\pfc\string_conv.obj plugins\pfc\bit_array.obj \
		plugins\pfc\utf8.obj plugins\pfc\sort.obj plugins\pfc\win-objects.obj plugins\pfc\guid.obj plugins\pfc\audio_math.obj \
		plugins\pfc\timers.obj plugins\pfc\audio_sample.obj plugins\pfc\pathUtils.obj plugins\pfc\stringNew.obj \
		plugins\foobar2000\foobar2000_component_client\component_client.obj
	link.exe /dll user32.lib ole32.lib shell32.lib plugins\foobar2000\shared\shared.lib /out:$@ $**

clean: FORCE
	-cmd /c for /r %%f in (*.obj,*.dll,*.exp,*.exe) do del /q %%f
	-del /q depends.tmp

FORCE:
