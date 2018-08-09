cmake_minimum_required(VERSION 2.8)

FILE(GLOB PROTOS "*.proto")

FOREACH(file ${PROTOS})
    GET_FILENAME_COMPONENT(FILE_NAME ${file} NAME)

    message(STATUS "===protoc  ${FILE_NAME}===")

    PROTOBUF_GENERATE_CPP(psrc phdr ${FILE_NAME})
    message(STATUS "src ${psrc} ${phdr}")
    LIST(APPEND SRC ${psrc} )
ENDFOREACH()

aux_source_directory(./ SRC)

ADD_LIBRARY(pb STATIC ${SRC} )
target_link_libraries(pb pbs)