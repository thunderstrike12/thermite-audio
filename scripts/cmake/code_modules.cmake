function(create_symlink SRC DST)
	if (EXISTS "${SRC}")
		#set(ASSETS_DST )
		
		# Make sure the parent dir exists so CREATE_LINK doesn't fail
		file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${target_name}")
		
		if (EXISTS "${DST}")
		    if (IS_SYMLINK "${DST}")
		        message(STATUS "Assets symlink already exists: ${DST}")
		    else()
		        message(FATAL_ERROR
		            "Path '${DST}' already exists and is not a symlink.\n"
		            "Delete it if you want CMake to manage it.\n")
		    endif()
		else()
		    message(STATUS "Creating assets symlink: ${DST} -> ${SRC}")
		    file(CREATE_LINK "${SRC}" "${DST}" SYMBOLIC)
		endif()
	endif()
endfunction()


# Find and add project targets
function(find_and_add_targets)
	# Find project directories
	set(PROJECTS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/projects")
	file(GLOB PROJECT_DIRS RELATIVE "${PROJECTS_ROOT}" "${PROJECTS_ROOT}/*")

	foreach(project ${PROJECT_DIRS})
		# Make sure the project has at least a main.cpp file 
		set(project_dir "${PROJECTS_ROOT}/${project}")
		if (IS_DIRECTORY "${project_dir}" AND EXISTS "${project_dir}/main.cpp")
			# Use folder name as target name
			set(target_name "${project}")
			message(STATUS "Auto-adding project target: ${target_name}")
		
			# Glob all the source files
			file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
				${project_dir}/*.cpp
				${project_dir}/*.hpp
			)
		
			# Add executable
			add_executable(${target_name}
				${PROJECT_SOURCES}
			)
			
			# Set runtime output directory to its own folder inside /bin/
			set_target_properties(${target_name} PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${target_name}"
				# Disables the windows console
				# WIN32_EXECUTABLE TRUE
			)
			# Disables the windows console
			#if(MSVC)
			#	target_link_options(${target_name} PRIVATE "/ENTRY:mainCRTStartup")
			#endif()

			# Copy over dll files for FMOD
			set(FMOD_POSTFIX "$<$<CONFIG:Debug>:L>")
			add_custom_command(
		        TARGET ${target_name} POST_BUILD
		        COMMAND ${CMAKE_COMMAND} -E copy
		                "${CMAKE_SOURCE_DIR}/extern/fmod/lib/fmod${FMOD_POSTFIX}.dll"
		                "${CMAKE_BINARY_DIR}/bin/${target_name}/fmod${FMOD_POSTFIX}.dll"
		
		        COMMAND ${CMAKE_COMMAND} -E copy
		                "${CMAKE_SOURCE_DIR}/extern/fmod/lib/fmodstudio${FMOD_POSTFIX}.dll"
		                "${CMAKE_BINARY_DIR}/bin/${target_name}/fmodstudio${FMOD_POSTFIX}.dll"
			)

			# Make sure target has assets folder
			file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${target_name}/assets")

			# Create symlink for asset folders to its executable
			message(STATUS "Finding and attempting symlinks to found asset folders..") 
			set(GAME_ASSETS_SRC "${project_dir}/assets")
			set(ENGINE_ASSETS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/engine/assets")
			set(EDITOR_ASSETS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/editor/assets")

			file(MAKE_DIRECTORY "${GAME_ASSETS_SRC}")

			create_symlink("${GAME_ASSETS_SRC}" "${CMAKE_BINARY_DIR}/bin/${target_name}/assets/game")
			create_symlink("${ENGINE_ASSETS_SRC}" "${CMAKE_BINARY_DIR}/bin/${target_name}/assets/engine")

			# If this is an editor build, symlink editor assets
			if(THERMITE_EDITOR_BUILD)
				create_symlink("${EDITOR_ASSETS_SRC}" "${CMAKE_BINARY_DIR}/bin/${target_name}/assets/editor")
			endif()

			# Link with the Thermite Engine
			target_link_libraries(${target_name} PRIVATE thermite-engine)
		
			# If this is an editor build, also link the editor library
			if(THERMITE_EDITOR_BUILD)
				target_link_libraries(${target_name} PRIVATE thermite-editor)
			endif()

			# Add some build state macros for code
			if (THERMITE_EDITOR_BUILD)
				target_compile_definitions(${target_name} PRIVATE THERMITE_EDITOR=1)
			endif()
			if (THERMITE_DEBUG_BUILD)
				target_compile_definitions(${target_name} PRIVATE THERMITE_DEBUG=1)
			endif()

			# Set warning level
			# if(THERMITE_CI)
			# 	target_compile_options(${target_name} PRIVATE /W4 /permissive- /WX)
			# else()
			# 	target_compile_options(${target_name} PRIVATE /W4 /permissive-)
			# endif()
		endif()
	endforeach()
endfunction()