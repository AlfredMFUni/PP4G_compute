@echo off

set VULKAN_VERSION=1.2.182.0
set VULKAN_BIN=VulkanSDK
set VULKAN_ZIP_NAME=%VULKAN_BIN%-%VULKAN_VERSION%
set VULKAN_SDK="%CD%\vendor\VulkanSDK"

set VULKAN_SHADER_COMPILER_FILE="%CD%\vendor\%VULKAN_BIN%\Bin\glslangValidator.exe"
set VULKAN_LAYER_RENDERDOC_DLL_FILE="%CD%\vendor\%VULKAN_BIN%\Bin\renderdoc.dll"
set VULKAN_LAYER_RENDERDOC_JSON_FILE="%CD%\vendor\%VULKAN_BIN%\Bin\renderdoc.json"
set VULKAN_LAYER_VALIDATION_DLL_FILE="%CD%\vendor\%VULKAN_BIN%\Bin\VkLayer_khronos_validation.dll"
set VULKAN_LAYER_VALIDATION_JSON_FILE="%CD%\vendor\%VULKAN_BIN%\Bin\VkLayer_khronos_validation.json"
set VULKAN_HEADER_FILE="%CD%\vendor\%VULKAN_BIN%\Include\vulkan\vulkan.h"
set VULKAN_LIB_FILE="%CD%\vendor\%VULKAN_BIN%\Lib\vulkan-1.lib"
set VULKAN_CONFIG_FILE="%CD%\vendor\%VULKAN_BIN%\Tools\vkconfig.exe"

if exist %VULKAN_SHADER_COMPILER_FILE% if exist %VULKAN_LAYER_RENDERDOC_DLL_FILE% if exist %VULKAN_LAYER_RENDERDOC_JSON_FILE% if exist %VULKAN_LAYER_VALIDATION_DLL_FILE% if exist %VULKAN_LAYER_VALIDATION_JSON_FILE% if exist %VULKAN_HEADER_FILE% if exist %VULKAN_LIB_FILE% if exist %VULKAN_CONFIG_FILE% (
	goto vulkan_sdk_found
)


echo unpacking vulkan...

set TEMP_NAME=temp
mkdir vendor
pushd vendor
	rem delete old %VULKAN_BIN%
	rmdir /s /q %VULKAN_BIN%

	rem unzip %VULKAN_ZIP_NAME% into a folder called %VULKAN_BIN%
	Powershell.exe Expand-Archive -LiteralPath %VULKAN_ZIP_NAME%.zip -Force -DestinationPath %VULKAN_BIN%
popd

if exist %VULKAN_SHADER_COMPILER_FILE% if exist %VULKAN_LAYER_RENDERDOC_DLL_FILE% if exist %VULKAN_LAYER_RENDERDOC_JSON_FILE% if exist %VULKAN_LAYER_VALIDATION_DLL_FILE% if exist %VULKAN_LAYER_VALIDATION_JSON_FILE% if exist %VULKAN_HEADER_FILE% if exist %VULKAN_LIB_FILE% if exist %VULKAN_CONFIG_FILE% (
	goto vulkan_sdk_found
)

echo failed to unpack vulkan
exit /b 1


:vulkan_sdk_found


echo vulkan @ %VULKAN_SDK%
