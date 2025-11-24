# Find and add project targets
function(find_and_add_targets)
	# Find project directories
	set(PROJECTS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/projects/")
	file(GLOB PROJECT_DIRS RELATIVE "${PROJECTS_ROOT}" "${PROJECTS_ROOT}/*")

	foreach(project ${PROJECT_DIRS})
		# Make sure the project has at least a main.cpp file 
		set(project_dir "${PROJECTS_ROOT}/${project}")
		if (IS_DIRECTORY "${project_dir}" AND EXISTS "${project_dir}/main.cpp")
			# Use folder name as target name
			set(target_name "${project}")
			message(STATUS "Auto-adding project target: ${target_name}")
		
			# Glob all the source files
			file(GLOB_RECURSE PROJECT_SOURCES
				${project_dir}/*.cpp
				${project_dir}/*.hpp
			)
		
			# Add executable
			add_executable(${target_name}
				${PROJECT_SOURCES}
			)
		
			# Link with the Thermite Engine
			target_link_libraries(${target_name} PRIVATE thermite-engine)
		
			# Add some build state macros for code
			if (THERMITE_EDITOR_BUILD)
				target_compile_definitions(${target_name} PRIVATE THERMITE_EDITOR=1)
			endif()
			if (THERMITE_DEBUG_BUILD)
				target_compile_definitions(${target_name} PRIVATE THERMITE_DEBUG=1)
			endif()
		endif()
	endforeach()
endfunction()
