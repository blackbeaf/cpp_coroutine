cmake_minimum_required(VERSION 2.8)

aux_source_directory(./ SRC)

add_executable(example1 ${SRC}) 

target_link_libraries(example1 eventmgr coroutine pb)
