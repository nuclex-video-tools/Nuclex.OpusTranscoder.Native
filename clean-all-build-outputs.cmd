::!%WinDir%\System32\cmd.exe
@ECHO OFF
setlocal enabledelayedexpansion

set dirs=
for /d %%D in (".\*") do (
  set dirs=!dirs! "%%D\bin"
)
for /d %%D in (".\*") do (
  set dirs=!dirs! "%%D\obj"
)
for /d %%D in (".\third-party\*") do (
  set dirs=!dirs! "%%D\bin"
)
for /d %%D in (".\third-party\*") do (
  set dirs=!dirs! "%%D\obj"
)
for %%D in (%dirs%) do (
  if exist "%%D" (
    rmdir /s /q "%%D"
  )
)

endlocal
