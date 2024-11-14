function(sub_dir_list DIR RECURSE RESULT)
	set(FOLDERS "")
	if(RECURSE)
		file(GLOB_RECURSE FOLDERS LIST_DIRECTORIES true ${DIR}/*)
	else() # RECURSE
		file(GLOB FOLDERS LIST_DIRECTORIES true ${DIR}/*)
	endif() # RECURSE

	# remove files
	set(FOLDERS_TEMP "")
	foreach(child ${FOLDERS})
		if(IS_DIRECTORY ${child})
			list(APPEND FOLDERS_TEMP ${child})
		endif() # IS_DIRECTORY
	endforeach()
	set(FOLDERS ${FOLDERS_TEMP})
	list(FILTER FOLDERS EXCLUDE REGEX "^.*\.git.*$")

	set(RESULT_TEMP "")
	foreach(child ${FOLDERS})
		cmake_path(RELATIVE_PATH child BASE_DIRECTORY ${DIR} OUTPUT_VARIABLE child)
		list(APPEND RESULT_TEMP ${child})
	endforeach()
	set(${RESULT} ${RESULT_TEMP} PARENT_SCOPE)
endfunction()
