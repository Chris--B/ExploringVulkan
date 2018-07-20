cmake_minimum_required(VERSION 3.7)

string(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(DEMO_NAME Demo)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(EXTERNAL_IDE_FOLDER "External")
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/External")
set(VULKAN_SDK "$ENV{VULKAN_SDK}")

project("${DEMO_NAME}" LANGUAGES CXX C)

# Demo
add_executable(${DEMO_NAME}
    include/00-Prelude.hpp
    include/00-Prelude/Debug.hpp
    include/00-Prelude/Utils.hpp
    include/Renderer.hpp

    source/Main.cpp
    source/Debug.cpp
    source/Utils.cpp
    source/Renderer.cpp
)

target_compile_definitions(${DEMO_NAME}
    PRIVATE
        "-DDEMO_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\""
        "-DWE_HAVE_PRELUDE=0"
)
target_include_directories(${DEMO_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory("${EXTERNAL_DIR}/glfw")
set_target_properties(glfw PROPERTIES FOLDER "${EXTERNAL_IDE_FOLDER}")
target_include_directories(${DEMO_NAME} PRIVATE "${EXTERNAL_DIR}/glfw/include")

# Vulkan
message(STATUS "VULKAN_LIBRARY     ${VULKAN_LIBRARY}")
message(STATUS "VULKAN_INCLUDE_DIR ${VULKAN_INCLUDE_DIR}")
target_include_directories(${DEMO_NAME} PRIVATE ${VULKAN_INCLUDE_DIR})
target_link_libraries(${DEMO_NAME} ${VULKAN_LIBRARY})

if (WIN32) # TODO: Make a proper WIN64 check somewhere
    target_compile_definitions(${DEMO_NAME} PRIVATE "-D_CRT_SECURE_NO_WARNINGS")
    message(STATUS "_CRT_SECURE_NO_WARNINGS")

    target_compile_definitions(${DEMO_NAME} PRIVATE "-D_CRT_NONSTDC_NO_DEPRECATE")
    message(STATUS "_CRT_NONSTDC_NO_DEPRECATE")

    # Vulkan Swapchain things - Do we need these?
    target_compile_definitions(${DEMO_NAME} PRIVATE "-DVK_USE_PLATFORM_WIN32_KHR")
    message(STATUS "VK_USE_PLATFORM_WIN32_KHR")
elseif(APPLE)
    # Vulkan Swapchain things - Do we need these?
    target_compile_definitions(${DEMO_NAME} PRIVATE "-DVK_USE_PLATFORM_MACOS_MVK")
    message(STATUS "VK_USE_PLATFORM_MACOS_MVK")

    target_compile_options(${DEMO_NAME} PRIVATE "-Wno-format-security")
else()
    # TODO: Add appropriate add_definitions here for non-Windows platforms
endif()

# glm
add_subdirectory("${EXTERNAL_DIR}/glm")
target_include_directories(${DEMO_NAME} PRIVATE "${EXTERNAL_DIR}/glm")
set_target_properties(glm_dummy PROPERTIES FOLDER "${EXTERNAL_IDE_FOLDER}")

# stb
add_subdirectory("${EXTERNAL_DIR}/stb")
target_include_directories(${DEMO_NAME} PRIVATE "${EXTERNAL_DIR}/stb")
set_target_properties(stb PROPERTIES FOLDER "${EXTERNAL_IDE_FOLDER}")

# tinyobj
add_subdirectory("${EXTERNAL_DIR}/tinyobjloader")
target_include_directories(${DEMO_NAME} PRIVATE "${EXTERNAL_DIR}/tinyobjloader")
set_target_properties(tinyobjloader PROPERTIES FOLDER "${EXTERNAL_IDE_FOLDER}")

target_link_libraries("${DEMO_NAME}"
    ${Vulkan_LIBRARY}
    glfw
    stb
    tinyobjloader
)

## Generate SPIRV Compilation Commands when CMake is initialized.
find_program(GLSLC glslc
             PATHS         $ENV{HOME} ${VULKAN_SDK}
             PATH_SUFFIXES bin macOS/bin
             DOC           "Compile GLSL code into SPIR-V"
)

if(NOT GLSLC)
    message(FATAL_ERROR "Cannot find glslc executable")
else()
    message(STATUS "Using GLSLC = ${GLSLC}")
endif()

message(STATUS "Finding GLSL files and generating compile rules")
file(GLOB_RECURSE GLSL_SOURCE_FILES
    # Other stages have their own extensions, but we only use these two atm.
    "Glsl/*.frag"
    "Glsl/*.vert"
)
file(GLOB_RECURSE GLSL_INCLUDE_FILES
    "Glsl/*.glsl"
)
set(GLSL_INCLUDE   "${CMAKE_SOURCE_DIR}/Glsl/include")
set(SHADER_BIN_DIR "${PROJECT_BINARY_DIR}/shader/")
set(GLSL_FLAGS     -g -O0 -I${GLSL_INCLUDE})

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV     "${SHADER_BIN_DIR}/${FILE_NAME}.spv")
    set(SPIRV_ASM "${SHADER_BIN_DIR}/${FILE_NAME}.spv.s")
    file(RELATIVE_PATH GLSL_R  ${CMAKE_SOURCE_DIR}/.. ${GLSL})
    file(RELATIVE_PATH SPIRV_R ${CMAKE_SOURCE_DIR}/.. ${SPIRV})

    # Build shader, ready for Vulkan consumption
    message(STATUS "    ${GLSL_R} -> ${SPIRV_R}")
    add_custom_command(
        OUTPUT  ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BIN_DIR}"
        COMMAND ${GLSLC} ${GLSL_FLAGS} ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}  ${GLSL_INCLUDE_FILES}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})

    # Compile shader into a human readable format
    message(STATUS "    ${GLSL_R} -> ${SPIRV_R}.s")
    add_custom_command(
        OUTPUT  ${SPIRV_ASM}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BIN_DIR}"
        COMMAND ${GLSLC} ${GLSL_FLAGS} -S ${GLSL} -o ${SPIRV_ASM}
        DEPENDS ${GLSL}  ${GLSL_INCLUDE_FILES}
    )
    list(APPEND SPIRV_ASM_FILES ${SPIRV_ASM})
endforeach(GLSL)

# Build Shaders-Asm before Demo, but...
add_custom_target(Shaders-Asm
    DEPENDS ${SPIRV_ASM_FILES}
    SOURCES ${GLSL_INCLUDE_FILES}
            ${GLSL_SOURCE_FILES}
)
add_dependencies("${DEMO_NAME}" Shaders-Asm)

# ...build Shaders before Shaders-Asm! This, if Shaders fails to build, we don't
# get double error messages. (because we compile everything twice.)
# If shaders start taking a long time to compile, we shouldn't do this.
# For now, we have a grand total of two... so let's go!
add_custom_target(Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
    SOURCES ${GLSL_INCLUDE_FILES}
            ${GLSL_SOURCE_FILES}
)
add_dependencies(Shaders-Asm Shaders)

add_custom_command(TARGET ${DEMO_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                                    "$<TARGET_FILE_DIR:${DEMO_NAME}>/shaders/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                                    "${SHADER_BIN_DIR}"
                                    "$<TARGET_FILE_DIR:${DEMO_NAME}>/shaders/"
)
