
# ads1115 target instructions

#####################################
# Define Target Information
#####################################
set(TARGET_NAME ads1115)
set(TARGET_INCLUDE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TARGET_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ads1115.cpp)

#####################################
# Support IDE Visualization
#####################################
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TARGET_SOURCES})

#####################################
# Define Target according to options
#####################################

add_library(${TARGET_NAME} STATIC ${TARGET_SOURCES})
# Find pigpio library
find_library(PIGPIO_LIBRARY pigpio)

# Check if pigpio library is found
if(NOT PIGPIO_LIBRARY)
    message(FATAL_ERROR "pigpio library not found")
endif()

#####################################
# Define Target Properties and Optional OpenSSL Dependency
#####################################

# generate an export header to expose symbols when using the library
generate_export_header(${TARGET_NAME} EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/${TARGET_NAME}_export.hpp)

# add target properties such as debug postfix, MSVC Runtime and library version
set_target_properties(${TARGET_NAME} PROPERTIES 
                                      DEBUG_POSTFIX "-d" 
                                      VERSION ${CMAKE_PROJECT_VERSION}
                                      SOVERSION ${CMAKE_PROJECT_VERSION})

# set target compile options as defined in the cmake/compiler_options.cmake Module
target_compile_options(${TARGET_NAME} PRIVATE ${compiler_options})

# set target link options as defined in the cmake/compiler_options.cmake Module
target_link_options(${TARGET_NAME} PRIVATE ${linker_flags})

# include all the necessary directories
target_include_directories(${TARGET_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/exports>
  PRIVATE 
    ${TARGET_INCLUDE_FOLDER}
    # Any other extra include required
)

target_link_libraries(${TARGET_NAME} PUBLIC ${PIGPIO_LIBRARY})


add_executable(
  ads1115_test
  test/ads1115_test.cpp
)
target_link_libraries(
  ads1115_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ads1115_test)
