project(onexCLI)

file(GLOB_RECURSE SRC_FILES RELATIVE ${PROJECT_SOURCE_DIR} *.cpp)

add_executable(onex ${SRC_FILES})

target_link_libraries(onex ${Readline_LIBRARY} onexLib)

set_target_properties(onex PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})