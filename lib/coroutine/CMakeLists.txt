cmake_minimum_required(VERSION 2.8)

enable_language(ASM)
set_property(SOURCE switch.s PROPERTY LANGUAGE ASM)

set (CMAKE_VERBOSE_MAKEFILE false )

aux_source_directory(./ SRC)

ADD_LIBRARY(coroutine STATIC ${SRC} switch.s)