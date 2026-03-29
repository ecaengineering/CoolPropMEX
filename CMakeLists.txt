cmake_minimum_required(VERSION 3.15)
project(CoolPropMEX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# CoolProp shared library (built from submodule)
# ---------------------------------------------------------------------------
set(COOLPROP_SHARED_LIBRARY ON  CACHE BOOL "" FORCE)
set(COOLPROP_STATIC_LIBRARY OFF CACHE BOOL "" FORCE)
set(COOLPROP_OBJECT_LIBRARY OFF CACHE BOOL "" FORCE)
set(COOLPROP_NO_EXAMPLES    ON  CACHE BOOL "" FORCE)
set(BUILD_TESTING           OFF CACHE BOOL "" FORCE)

add_subdirectory(CoolProp EXCLUDE_FROM_ALL)

# ---------------------------------------------------------------------------
# MATLAB MEX support
# ---------------------------------------------------------------------------
find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY)

# ---------------------------------------------------------------------------
# Helper macro: create one MEX target
# ---------------------------------------------------------------------------
macro(add_mex_target MEX_NAME)
    matlab_add_mex(
        NAME    ${MEX_NAME}
        SRC     src/${MEX_NAME}.cpp
        LINK_TO CoolProp
    )

    # Compiler flags
    if(MSVC)
        target_compile_options(${MEX_NAME} PRIVATE /utf-8)
    else()
        target_compile_options(${MEX_NAME} PRIVATE -fPIC)
    endif()

    # Apple Silicon: resolve weak MATLAB symbols at link time
    if(APPLE)
        target_link_options(${MEX_NAME} PRIVATE
            "-Wl,-U,_mexCreateMexFunction"
            "-Wl,-U,_mexDestroyMexFunction"
            "-Wl,-U,_mexFunctionAdapter"
            "-Wl,-ld_classic"
        )
    endif()

    # Linux: embed RPATH so the .so is found next to the .mexa64
    if(UNIX AND NOT APPLE)
        set_target_properties(${MEX_NAME} PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH            "$ORIGIN"
        )
    endif()

    # Output all MEX files to Release/ at the repo root
    set_target_properties(${MEX_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_SOURCE_DIR}/Release"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_SOURCE_DIR}/Release"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_SOURCE_DIR}/Release"
        LIBRARY_OUTPUT_DIRECTORY                "${CMAKE_SOURCE_DIR}/Release"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_SOURCE_DIR}/Release"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_SOURCE_DIR}/Release"
    )

    # Post-build: copy CoolProp shared library next to the MEX file
    if(WIN32)
        add_custom_command(TARGET ${MEX_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:CoolProp>
                "${CMAKE_SOURCE_DIR}/Release/"
            COMMENT "Copying CoolProp.dll to Release/"
        )
    elseif(APPLE)
        add_custom_command(TARGET ${MEX_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:CoolProp>
                "${CMAKE_SOURCE_DIR}/Release/"
            COMMENT "Copying libCoolProp.dylib to Release/"
        )
    else()
        # Copy the versioned .so (e.g. libCoolProp.so.7.2.0) and create the
        # soname symlink (libCoolProp.so.7) beside it so that the dynamic
        # linker can resolve it when MATLAB loads the MEX file from that dir.
        add_custom_command(TARGET ${MEX_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:CoolProp>
                "${CMAKE_SOURCE_DIR}/Release/"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                $<TARGET_FILE_NAME:CoolProp>
                "${CMAKE_SOURCE_DIR}/Release/$<TARGET_SONAME_FILE_NAME:CoolProp>"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                $<TARGET_FILE_NAME:CoolProp>
                "${CMAKE_SOURCE_DIR}/Release/$<TARGET_LINKER_FILE_NAME:CoolProp>"
            COMMENT "Copying libCoolProp.so + soname/linker symlinks to Release/"
        )
    endif()
endmacro()

# ---------------------------------------------------------------------------
# MEX targets
# ---------------------------------------------------------------------------
add_mex_target(PropsSI)
add_mex_target(HAPropsSI)
add_mex_target(AbstractStateMex)

# Copy AbstractState.m to Release/ (only needs to happen once, tie to AbstractStateMex)
add_custom_command(TARGET AbstractStateMex POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/matlab/AbstractState.m"
        "${CMAKE_SOURCE_DIR}/Release/"
    COMMENT "Copying AbstractState.m to Release/"
)
