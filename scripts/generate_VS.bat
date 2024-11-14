@echo off
setlocal disabledelayedexpansion



set ENABLE_IMGUI=ON



call scripts\find_cmake
if %errorlevel% GTR 0 (
	echo find_cmake returned %errorlevel%
	exit /b 1
)


call scripts\find_vulkan
if %errorlevel% GTR 0 (
	echo find_vulkan returned %errorlevel%
	exit /b 1
)



mkdir cmake
pushd cmake
	call %CMAKE_FILE%^
	 -DENABLE_IMGUI=%ENABLE_IMGUI%^
	 -DBUILD_SHARED_LIBS=OFF^
	 -DCMAKE_CONFIGURATION_TYPES=Debug;Release;RelWithDebInfo^
	 -G "Visual Studio 17 2022" -A x64 ..
popd



echo.
echo [31;4mdon't forget to point vulkan to use our local copies of the layers[0m
echo [31m1. open %VULKAN_CONFIG_FILE%[0m
echo [31m2. press "Find Layers..." [top right hand corner][0m
echo [31m3. press "Add..." and select "%CD%\vendor\%VULKAN_BIN%\Bin" in the dialog window, press OK[0m
echo [31m4. press OK to close the "Find Layers" dialog[0m
echo [31m5. select "Overriding Layers by the Vulkan Configurator"[0m
echo [31m6. make sure that "Continue Overriding Layers on Exit" is the only checkbox selected[0m
echo [31m7. close the Vulkan Configurator[0m
echo [31m8. a popup dialog will appear on closing the app, select "Yes"[0m
