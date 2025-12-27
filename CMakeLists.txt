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
<<<<<<< HEAD
# FIX: Point to the file inside the src directory
add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)

# 4. Include paths
# This ensures that #include "stb_image.h" works from inside your src folder
target_include_directories(${PROJECT_NAME} PUBLIC src)

# 5. Link Geode (This macro handles the heavy lifting)
=======
add_library(${PROJECT_NAME} SHARED
    main.cpp
)

# 4. Link Geode (This macro handles the heavy lifting)
>>>>>>> f9748aae7b869712d450ac448ae08cd3401f5ab9
setup_geode_mod(${PROJECT_NAME})