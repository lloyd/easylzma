#include "easylzma/compress.h"


struct _elzma_compress_handle {

};

elzma_compress_handle
elzma_compress_alloc()
{
    
}

void
elzma_compress_free(elzma_compress_handle * hand)
{

}

int
elzma_compress_config(elzma_compress_handle hand,
                      unsigned char lc,
                      unsigned char lp,
                      unsigned char pb,
                      unsigned char level,
                      unsigned int dictionarySize,
                      elzma_file_format format,
                      unsigned long long uncompressedSize)
{

}

int
elzma_compress_run(elzma_compress_handle hand,
                   elzma_read_callback inputStream, void * inputContext,
                   elzma_write_callback outputStream, void * outputContext)
{
}
