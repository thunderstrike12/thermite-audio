# Find and add project targets
function(find_and_add_targets)
    # Find project directories
    set(PROJECTS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/projects")
    file(GLOB PROJECT_DIRS RELATIVE "${PROJECTS_ROOT}" "${PROJECTS_ROOT}/*")

    foreach (project ${PROJECT_DIRS})
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
            add_executable(${target_name} ${PROJECT_SOURCES})

            # Generate a small config source file that the engine can access to know where the current relative project assets live
            set(gen_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/${target_name}")
            file(MAKE_DIRECTORY "${gen_dir}")
            set(cfg_cpp "${gen_dir}/tmt_project_config.cpp")
            file(WRITE "${cfg_cpp}" "extern \"C\" const char* TMT_PROJECT_RELATIVE_ASSETS_DIR = \"projects/${target_name}/assets\";\n")

            target_sources(${target_name} PRIVATE "${cfg_cpp}")

            # Set runtime output directory to its own folder inside /bin/
            set_target_properties(${target_name} PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${target_name}"
            )

            # Set the output executable name
            if (THERMITE_EDITOR_BUILD)
                set_target_properties(${target_name} PROPERTIES OUTPUT_NAME "thermite")
            else ()
                set_target_properties(${target_name} PROPERTIES OUTPUT_NAME "${target_name}")
            endif ()

            # Disables the windows console, for release game
            if (MSVC
                    AND NOT THERMITE_EDITOR_BUILD
                    AND NOT THERMITE_DEBUG_BUILD
                    AND CMAKE_BUILD_TYPE STREQUAL "Release")
                target_link_options(${target_name} PRIVATE
                        "/SUBSYSTEM:WINDOWS"
                        "/ENTRY:mainCRTStartup"
                )
            endif ()

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

            graphite_bundle_crash_diagnostics("${CMAKE_BINARY_DIR}/bin/${target_name}")

            # Copy over auxialliary assets for developer use
            if (THERMITE_DEVELOPER_BUILD)
                copy_directory_to_output(${target_name}
                        "${CMAKE_SOURCE_DIR}/engine/assets"
                        "engine/assets"
                )
                copy_directory_to_output(${target_name}
                        "${CMAKE_SOURCE_DIR}/editor/assets"
                        "editor/assets"
                )
            endif ()

            # Link with the Thermite Engine
            target_link_libraries(${target_name} PRIVATE thermite-engine)

            # If this is an editor build, also link the editor library
            if (THERMITE_EDITOR_BUILD)
                target_link_libraries(${target_name} PRIVATE thermite-editor)
            endif ()

            # Add compile definitons
            target_compile_definitions(${target_name} PRIVATE NOMINMAX) # Makes windows.h not define min and max
            # Add some build state macros for code
            if (THERMITE_EDITOR_BUILD)
                target_compile_definitions(${target_name} PRIVATE THERMITE_EDITOR=1)
            endif ()
            if (THERMITE_DEBUG_BUILD)
                target_compile_definitions(${target_name} PRIVATE THERMITE_DEBUG=1)
            endif ()
        endif ()
    endforeach ()
endfunction()

function(copy_directory_to_output TARGET_NAME SOURCE_DIR OUTPUT_DIR)
    add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SOURCE_DIR}"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/${OUTPUT_DIR}"
            COMMENT "Copying ${SOURCE_DIR} to ${TARGET_NAME} output directory"
    )
endfunction()