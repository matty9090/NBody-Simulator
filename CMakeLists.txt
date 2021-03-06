cmake_minimum_required(VERSION 3.5)
project(nbody)

set(CMAKE_CXX_STANDARD 14)

option(ENABLE_TESTING "Turns on testing" OFF)

# CxxOpts
include_directories(SYSTEM lib/CxxOpts/include)

# DirectXTK
add_subdirectory(lib/DirectXTK)
include_directories(SYSTEM lib/DirectXTK/Inc)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    link_directories(${CMAKE_SOURCE_DIR}/bin/CMake)
else()
    link_directories(${CMAKE_SOURCE_DIR}/bin/CMake/Release)
endif()

# ImGui
include_directories(SYSTEM lib/imgui)

add_library(imgui STATIC
	lib/imgui/imgui.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imconfig.h
    lib/imgui/imgui.h
    lib/imgui/imgui_internal.h
    lib/imgui/imstb_rectpack.h
    lib/imgui/imstb_textedit.h
    lib/imgui/imstb_truetype.h)

# Assimp
add_subdirectory(lib/assimp)
include_directories(SYSTEM lib/assimp/include)

# NBody
add_definitions(-DUNICODE)
include_directories(src)

file(GLOB_RECURSE SRC_FILES src/*.cpp)
list(REMOVE_ITEM SRC_FILES ${CMAKE_SOURCE_DIR}/src/App/NBody.cpp)
list(REMOVE_ITEM SRC_FILES ${CMAKE_SOURCE_DIR}/src/Render/Planet/Components/TerrainNode.cpp)
list(REMOVE_ITEM SRC_FILES ${CMAKE_SOURCE_DIR}/src/Render/Planet/Components/TerrainComponent.cpp)
list(REMOVE_ITEM SRC_FILES ${CMAKE_SOURCE_DIR}/src/Sim/GalaxySeeder.cpp)
list(REMOVE_ITEM SRC_FILES ${CMAKE_SOURCE_DIR}/src/Sim/RandomSeeder.cpp)
list(REMOVE_ITEM SRC_FILES ${CMAKE_SOURCE_DIR}/src/Sim/StarSystemSeeder.cpp)
message(${SRC_FILES})

add_library(nbody_lib STATIC ${SRC_FILES})
target_compile_options(nbody_lib PRIVATE /WX)
target_link_libraries(nbody_lib d3d11 dxgi imgui assimp)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_link_libraries(nbody_lib directxtkd)
else()
    target_link_libraries(nbody_lib directxtk)
endif()

add_executable(nbody WIN32 src/App/NBody.cpp resources/Resource.rc)
target_link_libraries(nbody nbody_lib)
target_compile_options(nbody PRIVATE /WX)

add_custom_command(TARGET nbody POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/shaders/ $<TARGET_FILE_DIR:nbody>/shaders)

add_custom_command(TARGET nbody POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/assets/ $<TARGET_FILE_DIR:nbody>/assets)


# Tests
if (ENABLE_TESTING)
    enable_testing()

    add_subdirectory(lib/googletest)
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp)
    add_executable(nbody_tests ${TEST_SRC_FILES})

    target_link_libraries(nbody_tests gtest gtest_main nbody_lib)

    add_test(UnitTests nbody_tests)
endif ()
