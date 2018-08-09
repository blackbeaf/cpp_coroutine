cmake_minimum_required(VERSION 2.8)

set (CMAKE_VERBOSE_MAKEFILE false )

aux_source_directory(./ SRC)

ADD_LIBRARY(eventmgr STATIC ${SRC})
