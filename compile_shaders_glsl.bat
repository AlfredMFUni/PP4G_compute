@echo off
setlocal enabledelayedexpansion


call scripts\find_vulkan ON
if %errorlevel% GTR 0 (
	echo find_vulkan returned %errorlevel%
	exit /b 1
)


for /R "bin/data/shaders/glsl" %%f in (.) do (
	pushd %%f
		for %%i in (*) do (
			set temp=%%i
			if "!temp:~-4!" neq ".spv" if "!temp:~-6!" neq ".embed" (
				rem found shader
				%VULKAN_SHADER_COMPILER_FILE% -V %%i -o "!temp!.spv"
			)
		)
	popd
)
