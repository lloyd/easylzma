cmake_minimum_required(VERSION 2.6)

FILE(GLOB SRCS pavel/*.c)
FILE(GLOB HDRS pavel/*.h)
SET(PUB_HDRS LzmaLib.h Types.h LzmaDec.h LzmaEnc.h)

# set up some paths for outputing the usable binaries
SET (libDir ${CMAKE_CURRENT_BINARY_DIR}/dist/lib)
SET (incDir ${CMAKE_CURRENT_BINARY_DIR}/dist/include)

# create these output directories
FILE(MAKE_DIRECTORY ${libDir})
FILE(MAKE_DIRECTORY ${incDir})

# get the built libs into the correct place
SET(LIBRARY_OUTPUT_PATH ${libDir})

ADD_LIBRARY(easylzma_s STATIC ${SRCS} ${HDRS})

### copy the two required headers into our output dir as a post build step
# copy public headers to output directory
FOREACH (header ${PUB_HDRS})
  SET (header ${CMAKE_CURRENT_SOURCE_DIR}/pavel/${header})
  ADD_CUSTOM_COMMAND(TARGET easylzma_s POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${header} ${incDir})
ENDFOREACH (header ${PUB_HDRS})
