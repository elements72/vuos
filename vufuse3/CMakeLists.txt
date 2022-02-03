cmake_minimum_required(VERSION 2.6)



execute_process(COMMAND bash "-c" "gcc -Wall ${CMAKE_CURRENT_SOURCE_DIR}/fuse_version.c `pkg-config fuse3 --cflags --libs` -o fuse_version && ./fuse_version" OUTPUT_VARIABLE FUSE_VERSION)

# FUSE_USE_VERSION should be ${FUSE_VERSION} but some modules (like sshfs) use version 31
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=31")

include_directories(${VU_HEADERS} ${CMAKE_CURRENT_SOURCE_DIR})

set(VU_MOD_TARGET vufuse3)
file(GLOB_RECURSE VUFUSE3_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(${VU_MOD_TARGET} SHARED ${VUFUSE3_SOURCES})
set_target_properties(${VU_MOD_TARGET} PROPERTIES PREFIX "")
target_link_libraries(${VU_MOD_TARGET} stropt volatilestream execs)

install(TARGETS ${VU_MOD_TARGET} LIBRARY DESTINATION ${MODULES_INSTALL_PATH})
