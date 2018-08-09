cmake_minimum_required(VERSION 2.8)

aux_source_directory(./ SRC)

add_executable(example2 ${SRC}) 

target_link_libraries(example2 eventmgr coroutine pb)
  