cmake_minimum_required(VERSION 3.21)

# Basic Project Setup
project(ImageImporter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# 1. Handle Geode SDK Path (Priority: CMake Var > Env Var)
if (NOT DEFINED GEODE_SDK)
    if (DEFINED ENV{GEODE_SDK})
        set(GEODE_SDK $ENV{GEODE_SDK})
    else()
        message(FATAL_ERROR "GEODE_SDK is not defined! Set it in your build script or environment.")
    endif()
endif()

message(STATUS "Using Geode SDK: ${GEODE_SDK}")

# 2. Add Geode subdirectory
add_subdirectory(${GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# 3. Create Library
add_library(${PROJECT_NAME} SHARED
    main.cpp
)

# 4. Link Geode (This macro handles the heavy lifting)
setup_geode_mod(${PROJECT_NAME})