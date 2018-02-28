cmake_minimum_required(VERSION 3.2)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")
project(VIPWRENCH)
add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")
set(CMAKE_CXX_STANDARD 11)

# include directories for dependencies and WRENCH libraries
include_directories(include/ /usr/local/include /usr/local/include/wrench)

# source files
set(SOURCE_FILES
        include/VipPilotJobScheduler.hpp
        include/VipStandardJobScheduler.hpp
        include/VipWMS.hpp
        src/VipPilotJobScheduler.cpp
        src/VipStandardJobScheduler.cpp
        src/VipWMS.cpp
        src/VipWrench.cpp
)

# wrench library and dependencies
find_library(WRENCH_LIBRARY NAMES wrench)
find_library(SIMGRID_LIBRARY NAMES simgrid)
find_library(PUGIXML_LIBRARY NAMES pugixml)
find_library(LEMON_LIBRARY NAMES emon)
find_library(GTEST_LIBRARY NAMES gtest)

# generating the executable
add_executable(vip_wrench ${SOURCE_FILES})
target_link_libraries(vip_wrench 
                        ${WRENCH_LIBRARY} 
                        ${SIMGRID_LIBRARY} 
                        ${PUGIXML_LIBRARY} 
                        ${LEMON_LIBRARY}
                     )
install(TARGETS vip_wrench DESTINATION bin)
