
# ads1115 target instructions

#####################################
# Define Target Information
#####################################
set(TARGET_NAME tcpserver)
set(TARGET_INCLUDE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TARGET_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/tcpserver.cpp)

#####################################
# Support IDE Visualization
#####################################
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TARGET_SOURCES})

#####################################
# Define Target according to options
#####################################

add_library(${TARGET_NAME} STATIC ${TARGET_SOURCES})


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
    ${Boost_INCLUDE_DIRS}
    ${BOOST_CIRCULAR_BUFFER_INCLUDE_DIRS}
    ${BOOST_ASIO_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../ppg/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecg/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ads1115/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../max30102/include
    # Any other extra include required
)


# add_executable(
#   ecg_test
#   test/ecg_test.cpp
# )
# target_link_libraries(
#   ecg_test
#   GTest::gtest_main
# )

# include(GoogleTest)
# gtest_discover_tests(ecg_test)
