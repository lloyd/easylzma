#include "easylzma/compress.h"
#include "lzma_header.h"
#include "common_internal.h"

#include "pavlov/Types.h"
#include "pavlov/LzmaEnc.h"

#include <string.h>

struct _elzma_compress_handle {
    CLzmaEncProps props;
    CLzmaEncHandle encHand;
    unsigned long long uncompressedSize;
    elzma_file_format format;
    struct elzma_alloc_struct allocStruct;
};

elzma_compress_handle
elzma_compress_alloc()
{
    elzma_compress_handle hand = malloc(sizeof(struct _elzma_compress_handle));
    memset((void *) hand, 0, sizeof(struct _elzma_compress_handle));

    /* "reasonable" defaults for props */
    LzmaEncProps_Init(&(hand->props));
    hand->props.lc = 3;
    hand->props.lp = 0;    
    hand->props.pb = 2;    
    hand->props.level = 5;
    hand->props.algo = 1;
    hand->props.fb = 32;
    hand->props.dictSize = 1 << 24;
    hand->props.btMode = 1;
    hand->props.numHashBytes = 4;
    hand->props.mc = 32;
    hand->props.numThreads = 1;
    hand->props.writeEndMark = 1;

    init_alloc_struct(&(hand->allocStruct), NULL, NULL, NULL, NULL);

    return hand;
}

void
elzma_compress_free(elzma_compress_handle * hand)
{
    if (hand && *hand) {
        if ((*hand)->encHand) {
            LzmaEnc_Destroy((*hand)->encHand,
                            (ISzAlloc *) &((*hand)->allocStruct),
                            (ISzAlloc *) &((*hand)->allocStruct));
        }
        
    }
    *hand = NULL;
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
    /* XXX: validate arguments are in valid ranges */

    hand->props.lc = lc;
    hand->props.lp = lp;    
    hand->props.pb = pb;
    hand->props.level = level;
    hand->props.dictSize = dictionarySize;
    hand->uncompressedSize = uncompressedSize;
    hand->format = format;

    return ELZMA_E_OK;
}

/* use Igor's stream hooks for compression. */
struct elzmaInStream
{
    SRes (*ReadPtr)(void *p, void *buf, size_t *size);
    elzma_read_callback inputStream;
    void * inputContext;
};
static SRes elzmaReadFunc(void *p, void *buf, size_t *size)
{
    struct elzmaInStream * is = (struct elzmaInStream *) p;
    return is->inputStream(is->inputContext, buf, size);
}

struct elzmaOutStream {
    size_t (*WritePtr)(void *p, const void *buf, size_t size);
    elzma_write_callback outputStream;
    void * outputContext;
};

static size_t elzmaWriteFunc(void *p, const void *buf, size_t size)
{
    struct elzmaOutStream * os = (struct elzmaOutStream *) p;
    return os->outputStream(os->outputContext, buf, size);
}

void elzma_compress_set_allocation_callbacks(
    elzma_compress_handle hand,
    elzma_malloc mallocFunc, void * mallocFuncContext,
    elzma_free freeFunc, void * freeFuncContext)
{
    if (hand) {
        init_alloc_struct(&(hand->allocStruct),
                          mallocFunc, mallocFuncContext,
                          freeFunc, freeFuncContext);
    }
}

int
elzma_compress_run(elzma_compress_handle hand,
                   elzma_read_callback inputStream, void * inputContext,
                   elzma_write_callback outputStream, void * outputContext)
{
    struct elzmaInStream inStreamStruct;
    struct elzmaOutStream outStreamStruct;    

    if (hand == NULL || inputStream == NULL) return ELZMA_E_BAD_PARAMS;

    /* initialize stream structrures */
    inStreamStruct.ReadPtr = elzmaReadFunc;
    inStreamStruct.inputStream = inputStream;    
    inStreamStruct.inputContext = inputContext;    

    outStreamStruct.WritePtr = elzmaWriteFunc;
    outStreamStruct.outputStream = outputStream;    
    outStreamStruct.outputContext = outputContext;    

    /* create an encoding object */
    hand->encHand = LzmaEnc_Create((ISzAlloc *) &(hand->allocStruct));

    if (hand->encHand == NULL) {
        return ELZMA_E_COMPRESS_ERROR;
    }

    /* inintialize with compression parameters */
    if (SZ_OK != LzmaEnc_SetProps(hand->encHand, &(hand->props)))
    {
        return ELZMA_E_BAD_PARAMS;
    }

    /* XXX: support lzip */
    if (ELZMA_lzma != hand->format) {
        return ELZMA_E_UNSUPPORTED_FORMAT;
    }

    /* now write the LZMA header */ 
    {
        unsigned char hdr[ELZMA_LZMA_HEADER_SIZE];
        struct elzma_lzma_header h;
        size_t wt;

        initLzmaHeader(&h);
        h.pb = hand->props.pb;
        h.lp = hand->props.lp;
        h.lc = hand->props.lc;
        h.dictSize = hand->props.dictSize;
        h.isStreamed = (hand->uncompressedSize == 0);
        h.uncompressedSize = hand->uncompressedSize;

        serializeLzmaHeader(hdr, &h);

        wt = outputStream(outputContext, (void *) hdr,
                          ELZMA_LZMA_HEADER_SIZE);
        if (wt != ELZMA_LZMA_HEADER_SIZE) {
            return ELZMA_E_OUTPUT_ERROR;
        }
    }
    
    /* begin LZMA encoding */
    /* XXX: expose encoding progress */
    SRes r = LzmaEnc_Encode(hand->encHand,
                            (ISeqOutStream *) &outStreamStruct,
                            (ISeqInStream *) &inStreamStruct, NULL,
                            (ISzAlloc *) &(hand->allocStruct),
                            (ISzAlloc *) &(hand->allocStruct));

    /* XXX: support a footer! (lzip) */

    if (r != SZ_OK) return ELZMA_E_COMPRESS_ERROR;
    
    return ELZMA_E_OK;
}
