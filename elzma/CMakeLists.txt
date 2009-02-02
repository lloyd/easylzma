SET (SRCS elzma.c)
SET (HDRS )

# use the library we built
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../src/dist/include)
LINK_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../src/dist/lib)

ADD_EXECUTABLE(elzma ${SRCS})

TARGET_LINK_LIBRARIES(elzma easylzma_s)
