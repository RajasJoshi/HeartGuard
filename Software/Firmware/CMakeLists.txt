
# Main CMake Project file

#####################################
# Define Project-Wide Settings
#####################################
cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

# Enables the Visibility Property on all target types
cmake_policy(SET CMP0063 NEW)
# Enables the MSVC_RUNTIME_LIBRARY property on targets
cmake_policy(SET CMP0091 NEW)
# Enables the CMP0135 policy to allow setting the CXX_STANDARD property on targets
# cmake_policy(SET CMP0135 NEW)
# Define the Project Name and Description
project(moderncpp-project LANGUAGES CXX DESCRIPTION "Codebase for Heartguard Project")

# Set Project version (used for library versioning and for CPack)
set(CMAKE_PROJECT_VERSION_MAJOR 1)
set(CMAKE_PROJECT_VERSION_MINOR 0)
set(CMAKE_PROJECT_VERSION_PATCH 0)
set(CMAKE_PROJECT_VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH})

# Set required C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
# Generate the compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
# Set the Relative Path Configurations
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# Configure the visibility of symbols in targets
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
# Define installation folder and the module path (this one required by conan)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})

#####################################
# Define Options
#####################################
option(BUILD_SHARED_LIBS "Build shared libraries" FALSE)

#####################################
# Define CMake Module Imports
#####################################
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(InstallRequiredSystemLibraries)
include(GenerateExportHeader)
include(${CMAKE_SOURCE_DIR}/cmake/compiler_options.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/cpack.cmake)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# FetchContent_Declare(
#   iir
#   URL https://github.com/berndporr/iir1/archive/refs/tags/1.9.4.zip
# )
# FetchContent_MakeAvailable(iir)
  
# Enable Testing
enable_testing()

#####################################
# Define Targets
#####################################
add_subdirectory(project)
