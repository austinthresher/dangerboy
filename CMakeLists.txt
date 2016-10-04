cmake_minimum_required (VERSION 2.6)

set (PROJECT_NAME dangerboy)

project (${PROJECT_NAME})
set (CMAKE_C_FLAGS_RELEASE "-Wpedantic -std=c11 -O4")
set (CMAKE_C_FLAGS_DEBUG   "-g  -Wall  -std=c11")
file (GLOB SOURCE_FILES "src/*.c")
find_package(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR})
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${SDL_LIBRARY})
target_link_libraries(${PROJECT_NAME} m ncurses)
message(${CMAKE_BUILD_TYPE})
