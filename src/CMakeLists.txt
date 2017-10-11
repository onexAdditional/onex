project(onexLib)

file(GLOB_RECURSE SRC_FILES RELATIVE ${PROJECT_SOURCE_DIR} *.cpp)

add_library(onexLib ${SRC_FILES})
