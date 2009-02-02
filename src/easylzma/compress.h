#ifndef __EASYLZMA_H__ 
#define __EASYLZMA_H__ 

/** an opaque handle to an lzma compressor */
typedef struct _elzma_compress_handle * elzma_compress_handle;

/** Supported file formats */
typedef enum {
    ELZMA_lzip, /**< the lzip format which includes a magic number and
                 *   CRC check */
    ELZMA_lzma  /**< the LZMA-Alone format, originally designed by
                 *   Igor Pavlov and in widespread use due to lzmautils,
                 *   lacking both aforementioned features of lzip */
/* XXX: future, potentially   ,
    ELZMA_xz 
*/
} elzma_file_format;

/**
 * Allocate a handle to an LZMA compressor object.
 */ 
elzma_compress_handle elzma_compress_alloc();

/**
 * set allocation routines (optional, if not called malloc & free will
 * be used) 
 */ 
typedef void * (*elzma_malloc)(void *ctx, unsigned int sz);
typedef void (*elzma_free)(void *ctx, void * ptr);
void elzma_compress_set_allocation_callbacks(
    elzma_compress_handle hand,
    elzma_malloc mallocFunc, void * mallocFuncContext,
    elzma_malloc freeFunc, void * freeFuncContext);

/**
 * Free all data associated with an LZMA compressor object.
 */ 
void elzma_compress_free(elzma_compress_handle hand);

/**
 * Set configuration paramters for a compression run.  If not called,
 * reasonable defaults will be used.
 */ 
int elzma_compress_config(elzma_compress_handle hand,
                          unsigned char lc,
                          unsigned char lp,
                          unsigned char pb,
                          unsigned char level,
                          unsigned int dictionarySize,
                          elzma_file_format format,
                          unsigned long long uncompressedSize);

typedef int (*elzma_write_callback)(void *ctx, const void *buf,
                                    unsigned int * size);

typedef int (*elzma_read_callback)(void *ctx, void *buf, size_t *size);

/**
 * Run compression
 */ 
int elzma_compress_run(
    elzma_compress_handle hand,
    elzma_read_callback inputStream, void * inputContext,
    elzma_write_callback outputStream, void * outputContext);


#endif
