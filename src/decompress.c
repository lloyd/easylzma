/*
 * Copyright 2009, Lloyd Hilaiel.
 *
 * License
 * 
 * All the cruft you find here is public domain.  You don't have to credit
 * anyone to use this code, but my personal request is that you mention
 * Igor Pavlov for his hard, high quality work.
 */

#include "easylzma/decompress.h"
#include "pavlov/LzmaDec.h"
#include "common_internal.h"
#include "lzma_header.h"

#include <string.h>
#include <assert.h>

#define ELZMA_DECOMPRESS_INPUT_BUFSIZE (1024 * 64)
#define ELZMA_DECOMPRESS_OUTPUT_BUFSIZE (1024 * 256)

/** an opaque handle to an lzma decompressor */
struct _elzma_decompress_handle {
    char inbuf[ELZMA_DECOMPRESS_INPUT_BUFSIZE];
    char outbuf[ELZMA_DECOMPRESS_OUTPUT_BUFSIZE];    
    struct elzma_alloc_struct allocStruct;
};

elzma_decompress_handle
elzma_decompress_alloc()
{
    elzma_decompress_handle hand =
        malloc(sizeof(struct _elzma_decompress_handle));
    memset((void *) hand, 0, sizeof(struct _elzma_decompress_handle));
    init_alloc_struct(&(hand->allocStruct), NULL, NULL, NULL, NULL);
    return hand;
}

void elzma_decompress_set_allocation_callbacks(
    elzma_decompress_handle hand,
    elzma_malloc mallocFunc, void * mallocFuncContext,
    elzma_free freeFunc, void * freeFuncContext)
{
    if (hand) {
        init_alloc_struct(&(hand->allocStruct),
                          mallocFunc, mallocFuncContext,
                          freeFunc, freeFuncContext);
    }
}


void
elzma_decompress_free(elzma_decompress_handle * hand)
{
    if (*hand) free(*hand);
    *hand = NULL;
}

int
elzma_decompress_run(elzma_decompress_handle hand,
                     elzma_read_callback inputStream, void * inputContext,
                     elzma_write_callback outputStream, void * outputContext)
{
    CLzmaDec dec;
    unsigned int errorCode = ELZMA_E_OK;

    /* initialize decoder memory */
    memset((void *) &dec, 0, sizeof(dec));
    LzmaDec_Init(&dec);

    /* decode the LZMA-Alone header. */
    {
        unsigned char hdr[ELZMA_LZMA_HEADER_SIZE];
        struct elzma_lzma_header h;
        size_t sz = ELZMA_LZMA_HEADER_SIZE;

        initLzmaHeader(&h);        

        if (inputStream(inputContext, hdr, &sz) != 0 ||
            sz != ELZMA_LZMA_HEADER_SIZE)
        {
            return ELZMA_E_INPUT_ERROR;
        }

        if (0 != parseLzmaHeader(hdr, &h)) {
            return ELZMA_E_CORRUPT_HEADER;
        }
        
        /* now we're ready to allocate the decoder */
        LzmaDec_Allocate(&dec, hdr, ELZMA_LZMA_HEADER_SIZE,
                         (ISzAlloc *) &(hand->allocStruct));
    }

    /* perform the decoding */
    for (;;)
    {
        size_t dstLen = ELZMA_DECOMPRESS_OUTPUT_BUFSIZE;
        size_t srcLen = ELZMA_DECOMPRESS_INPUT_BUFSIZE;
        size_t amt = 0;
        size_t bufOff = 0;
		ELzmaStatus stat;

        if (0 != inputStream(inputContext, hand->inbuf, &srcLen))
        {
            errorCode = ELZMA_E_INPUT_ERROR;
            goto decompressEnd;                    
        }

        /* handle the case where the input prematurely finishes */
        if (srcLen == 0) {
            errorCode = ELZMA_E_INSUFFICIENT_INPUT;
            goto decompressEnd;
        }
        
        amt = srcLen;

        /* handle the case where a single read buffer of compressed bytes
         * will translate into multiple buffers of uncompressed bytes,
         * with this inner loop */
        stat = LZMA_STATUS_NOT_SPECIFIED;

        while (bufOff < srcLen) {
            SRes r = LzmaDec_DecodeToBuf(&dec, (Byte *) hand->outbuf, &dstLen,
                                         ((Byte *) hand->inbuf + bufOff), &amt,
                                         LZMA_FINISH_ANY, &stat);

            /* XXX deal with result code more granularly*/
            if (r != SZ_OK) {
                errorCode = ELZMA_E_DECOMPRESS_ERROR;
                goto decompressEnd;
            }
            
            /* write what we've read */
            {
                size_t wt = outputStream(outputContext, hand->outbuf, dstLen);
                if (wt != dstLen) {
                    errorCode = ELZMA_E_OUTPUT_ERROR;
                    goto decompressEnd;                    
                }

            }
            
            /* do we have more data on the input buffer? */
            bufOff += amt;
            assert( bufOff <= srcLen );
            if (bufOff >= srcLen) break;
            amt = srcLen - bufOff;
        }

        /* now check status */
        if (stat == LZMA_STATUS_FINISHED_WITH_MARK) {
            break;
        }
        /* XXX: other status codes!? */
    }
    
  decompressEnd:
    LzmaDec_Free(&dec, (ISzAlloc *) &(hand->allocStruct));

    return errorCode;
}
