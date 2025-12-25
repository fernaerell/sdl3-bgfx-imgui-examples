cmake_minimum_required(VERSION 4.2)

project(
    "Example SDL3 + BGFX + IMGUI"
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the build type (Debug or Release)" FORCE)
endif()

set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=undefined -fsanitize-address-use-after-scope")

set(DEBUG_FLAGS "-g -O0 -Wall -Werror -Wextra -fno-omit-frame-pointer ${SANITIZER_FLAGS}")
set(RELEASE_FLAGS "-static -O3 -march=native -mtune=native -flto -funroll-loops -fomit-frame-pointer -ffast-math -finline-functions -fdata-sections -ffunction-sections -Os -DNDEBUG -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -g0")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${RELEASE_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${RELEASE_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s -Wl,--gc-sections -Wl,--strip-all -Wl,--as-needed")

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

find_package(SDL3 CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(
    "example_sdl3_bgfx"
    "${CMAKE_CURRENT_LIST_DIR}/src/main.cpp"
)

target_include_directories(
    "example_sdl3_bgfx"
    PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(
    "example_sdl3_bgfx"
    PRIVATE
    "SDL3::SDL3"
    "bgfx::bgfx"
    "imgui::imgui"
)