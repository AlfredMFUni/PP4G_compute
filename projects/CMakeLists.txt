function(build_project FOLDER_NAME)
	set(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME})

	sub_dir_list(${BASE_DIR} false SUBFOLDER_NAMES)
	if(SUBFOLDER_NAMES)
		set(TARGET_DIRS ${SUBFOLDER_NAMES})
		list(TRANSFORM TARGET_DIRS PREPEND ${BASE_DIR}/)
		set(TARGET_NAMES ${SUBFOLDER_NAMES})
		list(TRANSFORM TARGET_NAMES PREPEND ${FOLDER_NAME}_)

		set(HAS_SUBPROJECTS ON)
	else() # SUBFOLDER_NAMES
		set(SUBFOLDER_NAMES "${FOLDER_NAME}")
		set(TARGET_DIRS ${BASE_DIR})
		set(TARGET_NAMES ${SUBFOLDER_NAMES})

		set(HAS_SUBPROJECTS OFF)
	endif() # SUBFOLDER_NAMES


	foreach(p IN ZIP_LISTS SUBFOLDER_NAMES TARGET_DIRS TARGET_NAMES)
		set(SUBTARGET_NAME ${p_0})
		set(TARGET_DIR ${p_1})
		set(TARGET_NAME ${p_2})


		message(CHECK_START "Generating project: ${TARGET_NAME}")


		# source files
		file(GLOB SOURCE_FILES_BASE ${BASE_DIR}/*.*)
		list(FILTER SOURCE_FILES_BASE EXCLUDE REGEX "^.*/.git/.*$")
		file(GLOB_RECURSE SOURCE_FILES_PROJECT ${TARGET_DIR}/*.*)
		list(FILTER SOURCE_FILES_PROJECT EXCLUDE REGEX "^.*/.git/.*$")
		if(HAS_SUBPROJECTS)
			list(REMOVE_ITEM SOURCE_FILES_PROJECT SOURCE_FILES_BASE) # remove base files from project files
			if(NOT SOURCE_FILES_PROJECT)
				message(CHECK_FAIL "${TARGET_DIR} contains no files")
				continue()
			endif() # NOT SOURCE_FILES_PROJECT
		else() # HAS_SUBPROJECTS
			if(NOT SOURCE_FILES_BASE)
				message(CHECK_FAIL "${BASE_DIR} contains no files")
				continue()
			endif() # NOT SOURCE_FILES_BASE
		endif() # HAS_SUBPROJECTS


		# add files to IDE tree
		if(HAS_SUBPROJECTS)
			if(SOURCE_FILES_BASE)
				source_group(TREE ${BASE_DIR} PREFIX "" FILES ${SOURCE_FILES_BASE})
				source_group(TREE ${TARGET_DIR} PREFIX "${SUBTARGET_NAME}" FILES ${SOURCE_FILES_PROJECT})
			else() # SOURCE_FILES_BASE
				source_group(TREE ${TARGET_DIR} PREFIX "" FILES ${SOURCE_FILES_PROJECT})
			endif() # SOURCE_FILES_BASE
		else() # HAS_SUBPROJECTS
			source_group(TREE ${TARGET_DIR} PREFIX "" FILES ${SOURCE_FILES_PROJECT})
		endif() # HAS_SUBPROJECTS


		# shaders
		file(GLOB SHADER_FILES LIST_DIRECTORIES false ${SHADER_DIR}/${SHADER_LANGUAGE}/${TARGET_NAME}/*.*)
		list(FILTER SHADER_FILES EXCLUDE REGEX "^.*\.${SHADER_BINARY_EXTENSION}$")
		list(FILTER SHADER_FILES EXCLUDE REGEX "^.*\.embed$")
		source_group("shaders_${SHADER_LANGUAGE}" FILES ${SHADER_FILES}) # add to IDE folder


		# create project
		add_executable(${TARGET_NAME}
			${SOURCE_FILES_BASE} ${SOURCE_FILES_PROJECT}
			${SHADER_FILES})


		# renderer
		target_link_libraries(${TARGET_NAME} PUBLIC Vulkan::Vulkan)
		target_link_libraries(${TARGET_NAME} PRIVATE ${DEPENDENCY_NAME_GLFW})


		# dependencies
		## glm (header only)
		target_include_directories(${TARGET_NAME} PUBLIC ${glm_SOURCE_DIR})
		target_compile_definitions(${TARGET_NAME} PUBLIC GLM_FORCE_RADIANS)
		target_compile_definitions(${TARGET_NAME} PUBLIC GLM_FORCE_LEFT_HANDED)
		target_compile_definitions(${TARGET_NAME} PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

		## imgui
		if(ENABLE_IMGUI)
			target_link_libraries(${TARGET_NAME} PUBLIC ${DEPENDENCY_NAME_IMGUI})

			target_compile_definitions(${TARGET_NAME} PUBLIC MAGPIE_IMGUI)
		endif() # ENABLE_IMGUI

		## stb (header only)
		target_include_directories(${TARGET_NAME} PUBLIC ${stb_SOURCE_DIR})


		# IDE
		if(HAS_SUBPROJECTS)
			set_target_properties(${TARGET_NAME} PROPERTIES FOLDER ${FOLDER_NAME}) # put project in IDE folder
		endif() # HAS_SUBPROJECTS
		if(CMAKE_GENERATOR MATCHES "Visual Studio")
			set_target_properties(${TARGET_NAME} PROPERTIES LINK_OPTIONS "/SUBSYSTEM:WINDOWS") # app creates its own window
			set_target_properties(${TARGET_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${OUTPUT_DIR}) # set $(WorkingDir)
		endif() # Visual Studio


		message(CHECK_PASS "Complete")
	endforeach()
endfunction(build_project)


sub_dir_list(${CMAKE_CURRENT_SOURCE_DIR} false FOLDER_NAMES)

foreach(FOLDER_NAME ${FOLDER_NAMES})
	build_project(${FOLDER_NAME})
endforeach(FOLDER_NAME)
