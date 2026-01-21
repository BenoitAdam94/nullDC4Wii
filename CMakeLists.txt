##
##	NullDCe cmake script, top level
#

PROJECT( NDCE )
cmake_minimum_required(VERSION 2.6)






##	Build Options	##

# Target Specifics #

OPTION(_X86  "Build for CPU:X86 PC" OFF)
OPTION(_X64  "Build for CPU:X64 PC" OFF)
OPTION(_ARM  "Build for CPU:ARM" OFF)
OPTION(_PPC  "Build for CPU:PPC" OFF)
OPTION(_MIPS "Build for CPU:MIPS" OFF)

if(_X86)
  set(HOST_CPU _X86)
endif(_X86)

if(_X64)
  set(HOST_CPU _X64)
endif(_X64)

if(_ARM)
  set(HOST_CPU _ARM)
endif(_ARM)

if(_PPC)
  set(HOST_CPU _PPC)
endif(_PPC)

if(_MIPS)
  set(HOST_CPU _MIPS)
endif(_MIPS)




OPTION(_LINUX   "Build for OS:Linux" OFF)
OPTION(_DARWIN  "Build for OS:Darwin" OFF)
OPTION(_WINDOWS "Build for OS:Windows" OFF)
OPTION(_WII     "Build for OS:WII (Requires _PPC)" OFF)
OPTION(_PSP     "Build for OS:PSP (Requires _MIPS)" OFF)

if(_LINUX)
  set(HOST_OS _LINUX)
endif(_LINUX)

if(_DARWIN)
  set(HOST_OS _DARWIN)
endif(_DARWIN)

if(_WINDOWS)
  set(HOST_OS _WINDOWS)
endif(_WINDOWS)

if(_WII)
  set(HOST_OS _WII)
endif(_WII)

if(_PSP)
  set(HOST_OS _PSP)
endif(_PSP)


if(NOT DEFINED HOST_CPU)
  set(HOST_CPU _X86)
endif(NOT DEFINED HOST_CPU)

if(NOT DEFINED HOST_OS)
  set(HOST_OS _WINDOWS)
endif(NOT DEFINED HOST_OS)























MACRO(SRC_DIR SDIR)
FILE(GLOB_RECURSE inFiles RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}/${SDIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/${SDIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/${SDIR}/*.cpp"
)
FOREACH(infileName ${inFiles})
#    MESSAGE(STATUS "Process file: ${SDIR}/${infileName}")
    SET(NDCE_SRCS ${NDCE_SRCS} "${SDIR}/${infileName}")
ENDFOREACH(infileName)
ENDMACRO(SRC_DIR SDIR)




# Non-Recurse, we want to be able to separate so we'll use add_subdir (or not)
FILE(GLOB inFiles RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}" 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
SET(NDCE_SRCS ${inFiles})







# Platform independant base #

SRC_DIR(dc)
SRC_DIR(naomi)

SRC_DIR(cl)
SRC_DIR(gui)
SRC_DIR(config)
SRC_DIR(plugins)
SRC_DIR(profiler)
SRC_DIR(serial_ipc)




# TODO Change to MAKE_LIBRARY macro for platforms
SRC_DIR(plugs)






IF(_PSP)
    SRC_DIR(psp)
    SRC_DIR(dvemgr)
ENDIF(_PSP)


IF(_LINUX)
    SRC_DIR(linux)
ENDIF(_LINUX)


IF(_ARM)
    SRC_DIR(rec-ARM)
    #ADD_LIBRARY(rec-ARM)
ENDIF(_ARM)


IF(_X86)
    SRC_DIR(win-x86)
    #ADD_LIBRARY(win-x86)
ENDIF(_X86)











# The actual host generation #


set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-D${HOST_OS} -D${HOST_CPU})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})




MESSAGE(STATUS "Building NullDCe for: ${HOST_OS}-${HOST_CPU}")

ADD_EXECUTABLE(ndce ${NDCE_SRCS})


