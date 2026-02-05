# Shader Compilation Inspired by https://github.com/eliemichel/SlangWebGPU/blob/main/cmake/SlangUtils.cmake

function(compile_shaders)
    # Python interpreter
    find_package(Python3 REQUIRED)

    set(SHADER_COMPILER ${CMAKE_SOURCE_DIR}/extern/vulkan_sdk/bin/slangc.exe)
    set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/engine/assets/shaders)
    set(SHADER_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/engine/assets/shaders/bin)
    set(SHADER_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/compile_shaders.py)
    file(GLOB_RECURSE SLANG_SOURCES
        "${SHADER_SOURCE_DIR}/*.vx.slang"
        "${SHADER_SOURCE_DIR}/*.px.slang"
        "${SHADER_SOURCE_DIR}/*.cs.slang"
    )

    foreach(SHADER ${SLANG_SOURCES})
        # Find the path relative to the shader source directory
        # e.g. "debug/visibility.cs.slang"
        file(RELATIVE_PATH REL_PATH ${SHADER_SOURCE_DIR} ${SHADER})

        # Remove the ".slang" extension from the relative path
        string(REPLACE ".slang" "" REL_PATH_NO_EXT ${REL_PATH})

        # Output SPIR-V and dep-file paths
        set(OUTPUT_SPV "${SHADER_OUTPUT_DIR}/${REL_PATH_NO_EXT}.spv")
        set(DEPFILE "${SHADER_OUTPUT_DIR}/${REL_PATH_NO_EXT}.dep")

        set(DEPFILE_OPT)
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.21.0")
            list(APPEND DEPFILE_OPT DEPFILE "${DEPFILE}")
        else()
            message(AUTHOR_WARNING
                "CMake < 3.21 does not support depfiles. Shader dependencies won't be tracked."
            )
        endif()

        set(SHADER_DEBUG_FLAG "")
        if (THERMITE_DEBUG_BUILD)
            set(SHADER_DEBUG_FLAG --debug)
        endif()

        add_custom_command(
            OUTPUT ${OUTPUT_SPV}
            COMMAND ${Python3_EXECUTABLE}
                    ${SHADER_SCRIPT}
                    --compiler ${SHADER_COMPILER}
                    --input ${SHADER}
                    --output ${OUTPUT_SPV}
                    --root ${SHADER_SOURCE_DIR}
                    --depfile ${DEPFILE}
                    ${SHADER_DEBUG_FLAG}
            DEPENDS ${SHADER}
            ${DEPFILE_OPT}
            COMMENT "Compiling shader: ${REL_PATH_NO_EXT}"
            VERBATIM
        )

        list(APPEND SHADER_OUTPUTS ${OUTPUT_SPV})
    endforeach()

    # Group all shaders into a custom target
    add_custom_target(shaders ALL
        DEPENDS ${SHADER_OUTPUTS}
    )

    add_dependencies(thermite-engine shaders)
endfunction()