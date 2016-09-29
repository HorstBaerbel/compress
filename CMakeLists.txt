cmake_minimum_required (VERSION 2.8.9)
 
project(cmp5)

#-------------------------------------------------------------------------------
#set up compiler flags and executable names
#set(CMAKE_CXX_FLAGS "-fPIC") #or -fPIE

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") #multi-processor compilation
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP") #multi-processor compilation
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    #check for G++ 4.7+
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE G++_VERSION)
    if (G++_VERSION VERSION_LESS 4.7)
            message(SEND_ERROR "You need at least G++ 4.7 to compile cmp5!")
    endif()
    #set up compiler flags for GCC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2") #support C++11 for std::, optimize
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s") #strip binary
endif()

#-------------------------------------------------------------------------------
#define basic sources and headers

set(TARGET_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/src/bwt_codec.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/codec.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/compressor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/delta_codec.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/huffman_codec.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/huffman_codes.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/lzss_codec.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/mtf1_codec.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/rgb2planes_codec.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/rle0_codec.h
	${CMAKE_CURRENT_SOURCE_DIR}/sais/sais.hxx
	${CMAKE_CURRENT_SOURCE_DIR}/src/tools.h
)

set(TARGET_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/bwt_codec.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/cmp5.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/codec.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/compressor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/delta_codec.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/huffman_codec.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/huffman_codes.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/lzss_codec.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/mtf1_codec.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/rgb2planes_codec.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/rle0_codec.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/tools.cpp
)

#-------------------------------------------------------------------------------
#set up build directories

set(dir ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)

#-------------------------------------------------------------------------------
#define target

add_executable(cmp5 ${TARGET_SOURCES} ${TARGET_HEADERS})
