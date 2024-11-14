cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

# options

## support dependencies
option(ENABLE_IMGUI "enable ImGui support")


message(STATUS "ENABLE_IMGUI     = ${ENABLE_IMGUI}")


# variables

set(PROJECT_NAME Vulkan_Compute)

set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/bin)
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)
set(DATA_DIR ${OUTPUT_DIR}/data)
set(SHADER_DIR ${DATA_DIR}/shaders)

set(SHADER_LANGUAGE glsl)
set(SHADER_BINARY_EXTENSION spv)


# create project

set(CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD ${CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}/$<CONFIG>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CACHEFILE_DIR}/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CACHEFILE_DIR}/$<CONFIG>)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_CACHEFILE_DIR}/$<CONFIG>)
if(CMAKE_GENERATOR MATCHES "Visual Studio")
	set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY ${OUTPUT_DIR})
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif() # Visual Studio

project(${PROJECT_NAME} LANGUAGES CXX)


include(cmake_scripts/Utilities.cmake) # import utility functions
include(cmake_scripts/Dependencies.cmake) # add dependent projects


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER cmake) # hide cmake generated targets


add_subdirectory(projects)
