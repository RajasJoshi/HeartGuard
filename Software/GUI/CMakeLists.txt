cmake_minimum_required(VERSION 3.7.0)

project(qwt-example-app VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


# Find required Qt5 components
find_package(Qt5 COMPONENTS Network Widgets REQUIRED)

# Set the include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(qwt-example
  tcpclient.cpp
  window.cpp
  main.cpp
)

target_link_libraries(qwt-example Qt5::Widgets)
target_link_libraries(qwt-example qwt-qt5)
target_link_libraries(qwt-example pthread)
target_link_libraries(qwt-example Qt5::Network)

# Install the executable
install(TARGETS qwt-example)
