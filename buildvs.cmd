@echo off
echo. 2> depends.tmp
for /r %%f in (*.cpp) do (
  SET "_o=%%f"
  call cmd /c if "%%_o:main.cpp=%%"=="%%_o%%" echo "gbamp2wav.exe" "in_gbamp2wav.dll" "aud_gbamp2wav.dll": "%%_o:~0,-4%%.obj">>depends.tmp
  call cmd /c if "%%_o:main.cpp=%%"=="%%_o%%" echo "foo_input_gbamp2wav.dll": "%%_o:~0,-4%%.obj">>depends.tmp
  echo.>>depends.tmp
)
type depends.tmp | find \src\ > depends.mak
del depends.tmp
if [%*] NEQ [depends] nmake /f msvc32.mak %*
