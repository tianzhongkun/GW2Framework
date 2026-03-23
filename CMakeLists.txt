cmake_minimum_required(VERSION 3.20)
project(GW2Framework VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

if (MSVC)
    add_compile_options(/utf-8 /W3)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# MinHook Sources
set(MINHOOK_SOURCES
    ${CMAKE_SOURCE_DIR}/vendor/minhook/src/hde/hde32.c
    ${CMAKE_SOURCE_DIR}/vendor/minhook/src/hde/hde64.c
    ${CMAKE_SOURCE_DIR}/vendor/minhook/src/buffer.c
    ${CMAKE_SOURCE_DIR}/vendor/minhook/src/hook.c
    ${CMAKE_SOURCE_DIR}/vendor/minhook/src/trampoline.c
)

# ImGui Sources
file(GLOB IMGUI_SOURCES 
    "${CMAKE_SOURCE_DIR}/vendor/imgui/*.cpp"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_dx11.cpp"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_win32.cpp"
)

# All DLL Sources
set(DLL_SOURCES
    ${CMAKE_SOURCE_DIR}/src/ConfigManager.cpp
    ${CMAKE_SOURCE_DIR}/src/GW2API.cpp
    ${CMAKE_SOURCE_DIR}/src/MumbleLink.cpp
    ${CMAKE_SOURCE_DIR}/src/MemoryReader.cpp
    ${CMAKE_SOURCE_DIR}/src/DX11Hook.cpp
    ${CMAKE_SOURCE_DIR}/src/ImGuiImpl.cpp
    ${CMAKE_SOURCE_DIR}/src/PluginManager.cpp
    ${CMAKE_SOURCE_DIR}/src/plugins/CombatPlugin.cpp
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${IMGUI_SOURCES}
    ${MINHOOK_SOURCES}
)

# Core DLL (GW2Framework)
add_library(GW2Framework SHARED ${DLL_SOURCES})
target_include_directories(GW2Framework PRIVATE 
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/vendor/minhook/include
    ${CMAKE_SOURCE_DIR}/vendor/minhook/src
    ${CMAKE_SOURCE_DIR}/vendor/json
)
target_link_libraries(GW2Framework d3d11.lib dxgi.lib user32.lib psapi.lib)

# Console Injector
add_executable(Injector ${CMAKE_SOURCE_DIR}/src/Injector.cpp)
target_link_libraries(Injector user32.lib)

# GUI Injector
add_executable(GUIInjector WIN32 ${CMAKE_SOURCE_DIR}/src/GUIInjector.cpp)
target_link_libraries(GUIInjector user32.lib psapi.lib comdlg32.lib)

# Output to same directory
set_target_properties(GW2Framework Injector GUIInjector PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy JSON header to build
add_custom_command(TARGET GW2Framework POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/vendor/json/json.hpp"
    ${CMAKE_BINARY_DIR}/bin/
)
