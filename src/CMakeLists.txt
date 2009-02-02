cmake_minimum_required(VERSION 2.6)

FILE(GLOB SRCS pavlov/*.c *.c)
FILE(GLOB HDRS *.h pavlov/*.h easylzma/*.h)
FILE(GLOB PUB_HDRS easylzma/*.h)

# (XXX temporarily include Igor's headers in output dir)
FILE(GLOB TMP_HDRS
     pavlov/LzmaLib.h pavlov/Types.h pavlov/LzmaDec.h pavlov/LzmaEnc.h)
SET(PUB_HDRS ${PUB_HDRS} ${TMP_HDRS})

# set up some paths for outputing the usable binaries
SET (libDir ${CMAKE_CURRENT_BINARY_DIR}/dist/lib)
SET (incDir ${CMAKE_CURRENT_BINARY_DIR}/dist/include/easylzma)

# an include directory to allow easylzma implementation to find public
# headers
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# create these output directories
FILE(MAKE_DIRECTORY ${libDir})
FILE(MAKE_DIRECTORY ${incDir})

# get the built libs into the correct place
SET(LIBRARY_OUTPUT_PATH ${libDir})

ADD_LIBRARY(easylzma_s STATIC ${SRCS} ${HDRS})

### copy the two required headers into our output dir as a post build step
# copy public headers to output directory
FOREACH (header ${PUB_HDRS})
  # preserve relative pathing		
  ADD_CUSTOM_COMMAND(TARGET easylzma_s POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${header} ${incDir})
ENDFOREACH (header ${PUB_HDRS})
