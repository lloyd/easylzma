#ifndef __MY_LZMA_HEADER__
#define __MY_LZMA_HEADER__

// header format gleaned from reading Igor's code

#define MY_LZMA_HEADER_SIZE 13
#define MY_LZMA_PROPSBUF_SIZE 5

struct my_lzma_header {
    unsigned char pb;
    unsigned char lp;    
    unsigned char lc;    
    unsigned char isStreamed;    
    long long unsigned int uncompressedSize;
    unsigned int dictSize;
};

void initLzmaHeader(my_lzma_header * hdr);
bool parseLzmaHeader(const unsigned char * hdrBuf, my_lzma_header * hdr);
bool serializeLzmaHeader(unsigned char * hdrBuf,
                         const my_lzma_header * hdr);

#endif
