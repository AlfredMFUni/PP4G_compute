# renderer
set(DEPENDENCY_NAME_VULKAN Vulkan)
set(DEPENDENCY_NAME_GLFW glfw)

set(DEPENDENCY_TAG_GLFW 3.3.8)


# support
set(DEPENDENCY_NAME_GLM glm)
set(DEPENDENCY_NAME_IMGUI imgui)
set(DEPENDENCY_NAME_STB stb)

set(DEPENDENCY_TAG_GLM 0.9.9.8)
set(DEPENDENCY_TAG_IMGUI v1.88)
set(DEPENDENCY_TAG_STB master)



include(CheckLanguage)
include(FetchContent)



# renderer
message(STATUS "Fetch renderer dependencies")

## vulkan
message(CHECK_START "Check dependency: ${DEPENDENCY_NAME_VULKAN}")
set(Vulkan_INCLUDE_DIR ${VENDOR_DIR}/VulkanSDK/Include)
find_library(Vulkan_LIBRARY NAMES vulkan-1 PATHS ${VENDOR_DIR}/VulkanSDK/Lib)
find_program(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE glslangValidator PATHS ${VENDOR_DIR}/VulkanSDK/Bin)
find_program(Vulkan_GLSLC_EXECUTABLE glslc PATHS ${VENDOR_DIR}/VulkanSDK/Bin)
find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
	message(CHECK_FAIL "Can not find dependency: ${DEPENDENCY_NAME_VULKAN}")
	return()
endif() # NOT Vulkan_FOUND
message(CHECK_PASS "Complete")


message(CHECK_START "Fetch dependency: ${DEPENDENCY_NAME_GLFW}")
FetchContent_Declare(${DEPENDENCY_NAME_GLFW}
	GIT_REPOSITORY https://github.com/glfw/glfw
	GIT_TAG ${DEPENDENCY_TAG_GLFW})
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_INSTALL OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(${DEPENDENCY_NAME_GLFW})
set_target_properties(${DEPENDENCY_NAME_GLFW} PROPERTIES FOLDER dependencies)
set_target_properties(update_mappings PROPERTIES FOLDER dependencies)
message(CHECK_PASS "Complete")


# support
message(STATUS "Fetch support dependencies")

## glm (header only)
message(CHECK_START "Fetch dependency: ${DEPENDENCY_NAME_GLM}")
FetchContent_Declare(${DEPENDENCY_NAME_GLM}
	GIT_REPOSITORY https://github.com/g-truc/glm
	GIT_TAG ${DEPENDENCY_TAG_GLM})
FetchContent_MakeAvailable(${DEPENDENCY_NAME_GLM})
message(CHECK_PASS "Complete")

## imgui
if(ENABLE_IMGUI)
	message(CHECK_START "Fetch dependency: ${DEPENDENCY_NAME_IMGUI}")


	FetchContent_Declare(${DEPENDENCY_NAME_IMGUI}
		GIT_REPOSITORY https://github.com/ocornut/imgui
		GIT_TAG ${DEPENDENCY_TAG_IMGUI})
	FetchContent_MakeAvailable(${DEPENDENCY_NAME_IMGUI})

	set(imgui_source_files
		${imgui_SOURCE_DIR}/imgui.h
		${imgui_SOURCE_DIR}/imgui.cpp
		${imgui_SOURCE_DIR}/imgui_draw.cpp
		${imgui_SOURCE_DIR}/imgui_demo.cpp
		${imgui_SOURCE_DIR}/imgui_tables.cpp
		${imgui_SOURCE_DIR}/imgui_widgets.cpp)
	list(APPEND imgui_source_files
		${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.h
		${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
		${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
		${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)

	add_library(${DEPENDENCY_NAME_IMGUI} STATIC ${imgui_source_files})
	target_include_directories(${DEPENDENCY_NAME_IMGUI} PUBLIC ${imgui_SOURCE_DIR})

	# add vulkan
	target_link_libraries(${DEPENDENCY_NAME_IMGUI} PRIVATE Vulkan::Vulkan)
	# add glfw
	target_link_libraries(${DEPENDENCY_NAME_IMGUI} PRIVATE ${DEPENDENCY_NAME_GLFW})

	set_target_properties(${DEPENDENCY_NAME_IMGUI} PROPERTIES FOLDER dependencies)


	message(CHECK_PASS "Complete")
endif() # ENABLE_IMGUI

## stb (header only)
message(CHECK_START "Fetch dependency: ${DEPENDENCY_NAME_STB}")
FetchContent_Declare(${DEPENDENCY_NAME_STB}
	GIT_REPOSITORY https://github.com/nothings/stb
	GIT_TAG ${DEPENDENCY_TAG_STB})
FetchContent_MakeAvailable(${DEPENDENCY_NAME_STB})
message(CHECK_PASS "Complete")
