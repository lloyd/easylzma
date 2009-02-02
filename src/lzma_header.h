#ifndef __EASYLZMA_LZMA_HEADER__
#define __EASYLZMA_LZMA_HEADER__

/* LZMA-Alone header format gleaned from reading Igor's code */

#define ELZMA_LZMA_HEADER_SIZE 13
#define ELZMA_LZMA_PROPSBUF_SIZE 5

struct elzma_lzma_header {
    unsigned char pb;
    unsigned char lp;    
    unsigned char lc;    
    unsigned char isStreamed;    
    long long unsigned int uncompressedSize;
    unsigned int dictSize;
};

void initLzmaHeader(struct elzma_lzma_header * hdr);

/* returns non-zero on failure */
int parseLzmaHeader(const unsigned char * hdrBuf,
                    struct elzma_lzma_header * hdr);

/* returns non-zero on failure */
int serializeLzmaHeader(unsigned char * hdrBuf,
                        const struct elzma_lzma_header * hdr);

#endif
