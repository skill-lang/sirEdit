cmake_minimum_required(VERSION 3.13)
project(sirEdit)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0)

add_subdirectory(src)

file(GLOB SIR ${CMAKE_CURRENT_SOURCE_DIR}/sir/*.cpp)
add_subdirectory(cppCommon)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

add_executable(sirEdit ${SOURCES} ${SIR})
target_include_directories(sirEdit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cppCommon ${CMAKE_CURRENT_SOURCE_DIR}/sir)
target_link_directories(sirEdit PRIVATE ${GTKMM_LIBRARY_DIRS})
target_include_directories(sirEdit PRIVATE ${GTKMM_INCLUDE_DIRS})
target_link_libraries(sirEdit skill.cpp.common ${GTKMM_LIBRARIES})
target_include_directories(sirEdit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
