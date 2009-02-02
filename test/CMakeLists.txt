SET (SRCS main.cpp header.cpp)
SET (HDRS header.h)

# use the library we built
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../src/dist/include/easylzma)
LINK_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../src/dist/lib)

ADD_EXECUTABLE(lzma_test ${SRCS})

TARGET_LINK_LIBRARIES(lzma_test easylzma_s)
