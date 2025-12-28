cmake_minimum_required(VERSION 3.21)

project(ImageImporter VERSION 1.0.0 LANGUAGES CXX)

if (APPLE)
    if (IOS)
        set(CMAKE_OSX_ARCHITECTURES arm64)
    endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if (NOT DEFINED GEODE_SDK)
    if (DEFINED ENV{GEODE_SDK})
        set(GEODE_SDK $ENV{GEODE_SDK})
    else()
        message(FATAL_ERROR "GEODE_SDK is not defined!")
    endif()
endif()

add_subdirectory(${GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC src)

setup_geode_mod(${PROJECT_NAME})
