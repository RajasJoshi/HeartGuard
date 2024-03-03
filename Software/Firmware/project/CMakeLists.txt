
# Project-related instructions

#####################################
# Define Targets
#####################################
add_subdirectory(heartguard)
add_subdirectory(ads1115)
add_subdirectory(max30102)

#####################################
# Define Install Targets
#####################################
install(TARGETS
  heartguard
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include
)

install(
  FILES 
    ${CMAKE_SOURCE_DIR}/project/ads1115/include/ads1115.hpp
    ${CMAKE_BINARY_DIR}/exports/ads1115_export.hpp
  DESTINATION 
    include/ads1115
)

install(
  FILES 
    ${CMAKE_SOURCE_DIR}/project/max30102/include/max30102.hpp
    ${CMAKE_BINARY_DIR}/exports/max30102_export.hpp
  DESTINATION 
    include/max30102
)
