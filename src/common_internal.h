#ifndef __ELZMA_COMMON_INTERNAL_H__
#define __ELZMA_COMMON_INTERNAL_H__

#include "easylzma/common.h"

/** a structure which may be cast and passed into Igor's allocate
 *  routines */
struct elzma_alloc_struct {
    void *(*Alloc)(void *p, size_t size);
    void (*Free)(void *p, void *address); /* address can be 0 */

    elzma_malloc clientMallocFunc;
    void * clientMallocContext;

    elzma_free clientFreeFunc;
    void * clientFreeContext;
};

/* initialize an allocation structure, may be called safely multiple
 * times */
void init_alloc_struct(struct elzma_alloc_struct * allocStruct,
                       elzma_malloc clientMallocFunc,
                       void * clientMallocContext,
                       elzma_free clientFreeFunc,
                       void * clientFreeContext);



#endif
