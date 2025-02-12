cmake_minimum_required(VERSION 3.8)

project(vpk_shell_extensions VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#ADD_DEFINITIONS(-Wall -DWIN32_LEAN_AND_MEAN -DUNICODE -D_CRT_SECURE_NO_WARNINGS)
add_definitions(-DUNICODE)

add_subdirectory("sourcepp")

set(MY_VERSIONINFO_RC "${CMAKE_BINARY_DIR}/VersionInfo.rc")
configure_file("${CMAKE_SOURCE_DIR}/VersionInfo.rc.in"
                "${MY_VERSIONINFO_RC}")

add_library("VPKShellExtensions" SHARED  "src/pch.cpp"
                                    "src/ThumbnailProvider.cpp"
                                    
                                    "src/Main.cpp"
                                    "src/Module.def" 
                                    "src/MetadataProvider.cpp"
                                    "src/PropVariantSafe.h"
                                    "src/VPKReader.cpp"
                                    "src/Utils.cpp"
                                    ${MY_VERSIONINFO_RC})

target_include_directories("VPKShellExtensions" INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features("VPKShellExtensions" INTERFACE cxx_std_20)
set_target_properties("VPKShellExtensions" PROPERTIES OUTPUT_NAME "VPKShellExtensions")



add_dependencies("VPKShellExtensions" vpkpp)
target_link_libraries("VPKShellExtensions" PUBLIC shlwapi.lib comctl32.lib propsys.lib gdiplus.lib vpkpp vtfpp)

add_executable("tester" "src/test.cpp")
target_link_libraries("tester" PUBLIC vpkpp vtfpp)