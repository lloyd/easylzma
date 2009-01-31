extern "C" {
#include "LzmaEnc.h"
#include "LzmaDec.h"
};

#include <stdlib.h>

// allocation routines
static void *SzAlloc(void *p, size_t size) { p = p; return malloc(size); }
static void SzFree(void *p, void *address) { p = p; free(address); }
static ISzAlloc myAlloc = { SzAlloc, SzFree };

#include <string>
#include <iostream>
#include <fstream>

static char s_inBuffer[1024 * 64];
static char s_outBuffer[1024 * 64];

// we use the stream compression hooks.  this gunk makes it all go
struct FStreamInStream
{
    SRes Read(void *p, void *buf, size_t *size)
    {
        FStreamInStream * is = (FStreamInStream *) p;
        if (is->fileStream->eof()) {
            *size = 0;
        } else {
            is->fileStream->read((char *) buf, *size);
            *size = is->fileStream->gcount();
        }
        return SZ_OK;
    }
    std::ifstream * fileStream;
};

struct FStreamOutStream
{
    size_t Write(void *p, const void *buf, size_t size)
    {
        FStreamOutStream * os = (FStreamOutStream *) p;
        if (os->fileStream->eof() || os->fileStream->fail()) {
            return -1;
        }
        unsigned int pos = os->fileStream->tellp();
        os->fileStream->write((const char *) buf, size);
        return ((unsigned int) os->fileStream->tellp() - pos);
    }
    std::ofstream * fileStream;
};
    

static int
doCompress(std::ifstream & inFile, std::ofstream & outFile)
{
    // allocate the compressor
    CLzmaEncProps props;
    LzmaEncProps_Init(&props);
    props.lc = 3;
    props.lp = 0;    
    props.pb = 2;    
    props.level = 5;
    props.algo = 1;
    props.fb = 32;
    props.dictSize = 1 << 24;
    props.btMode = 1;
    props.numHashBytes = 4;
    props.mc = 32;
    props.numThreads = 1;
    props.writeEndMark = 1;
    CLzmaEncHandle hand = LzmaEnc_Create(&myAlloc);
    LzmaEnc_SetProps(hand, &props);

    // properties!  write them as the first 5 bytes of the stream
    {
        SizeT s = LZMA_PROPS_SIZE;
        Byte props[LZMA_PROPS_SIZE];
        LzmaEnc_WriteProperties(hand, props, &s);
        std::cout << "wrote props to byte stream: "
                  << s << " bytes" << std::endl;
        outFile.write((char *) props, LZMA_PROPS_SIZE);
    }
    
    // now let's do the compression!
    for (;;)
    {
        inFile.read((char *) s_inBuffer, sizeof(s_inBuffer));
        std::cout << "read: " << inFile.gcount() << " uncompressed bytes"
                  << std::endl;

        size_t dstLen = sizeof(s_outBuffer);
        // XXX: run bytes through compression
        LzmaEnc_MemEncode(hand, (Byte *) s_outBuffer, &dstLen,
                          (Byte *) s_inBuffer, inFile.gcount(),
                          0 /*(inFile.eof() ? 1 : 0) */, NULL,
                          &myAlloc, &myAlloc);        
        std::cout << "writing: " << dstLen << " compressed bytes"
                  << std::endl;

        outFile.write(s_outBuffer, dstLen);

        if (inFile.eof()) break;

        if (inFile.fail()) {
            std::cerr << "read failure" << std::endl;
            break;
        }
    }

    LzmaEnc_Destroy(hand, &myAlloc, &myAlloc);

    return 0;
}

static int
doUncompress(std::ifstream & inFile, std::ofstream & outFile)
{
    // return codes from lzma library
    SRes r;

    // allocate the decompressor
    CLzmaDec dec;
    memset(&dec, 0, sizeof(dec));
    LzmaDec_Init(&dec);

    // decode the properties, these are encoded into the first LZMA_PROPS_SIZE
    // bytes of the input stream
    {
        Byte propsBuf[LZMA_PROPS_SIZE];
        inFile.read((char *) propsBuf, LZMA_PROPS_SIZE);
        if (inFile.gcount() != LZMA_PROPS_SIZE) {
            std::cerr << "couldn't read lzma stream properties!" << std::endl;
            return 1;
        }
        CLzmaProps props;
        r = LzmaProps_Decode(&props, propsBuf, LZMA_PROPS_SIZE);        
        
        if (r != SZ_OK) {
            std::cerr << "couldn't parse lzma stream properties!" << std::endl;
            return 1;
        }
        
        // now we're ready to allocate the decoder
        LzmaDec_Allocate(&dec, propsBuf, LZMA_PROPS_SIZE, &myAlloc);
    }
    
    // now let's do the decompression!
    for (;;)
    {
        inFile.read((char *) s_inBuffer, sizeof(s_inBuffer));

        size_t dstLen = sizeof(s_outBuffer);
        size_t srcLen = inFile.gcount();
        size_t amt = inFile.gcount();
        size_t bufOff = 0;

        std::cout << "read: " << srcLen << " compressed bytes"
                  << std::endl;

        // because we're _decompressing_ here, likely a single read buffer
        // of compressed bytes will translate into multiple buffers of
        // uncompressed bytes
        ELzmaStatus stat = LZMA_STATUS_NOT_SPECIFIED;

        while (bufOff < srcLen) {
            r = LzmaDec_DecodeToBuf(&dec, (Byte *) s_outBuffer, &dstLen,
                                    ((Byte *) s_inBuffer + bufOff), &amt,
                                    LZMA_FINISH_ANY, &stat);

            // now write what we've decoded
            std::cout << "finished decode of " << amt << " bytes into "
                      << dstLen << std::endl;

            if (r != SZ_OK) {
                std::cerr << "decode error" << std::endl;
                goto decompressEnd;
            }
            
            // write what we've read
            std::cout << "writing: " << dstLen << " uncompressed bytes"
                      << std::endl;
            outFile.write(s_outBuffer, dstLen);
            if (outFile.fail()) {
                std::cerr << "error writing!" << std::endl;
                goto decompressEnd;
            }
            
            // do we have more data on the input buffer?
            bufOff += amt;
            assert( bufOff <= srcLen );
            if (bufOff >= srcLen) break;
            amt = srcLen - bufOff;

            std::cout << amt << " bytes left" << std::endl;
        }

        if (inFile.eof()) break;

        if (inFile.fail()) {
            std::cerr << "read failure" << std::endl;
            break;
        }
    }

decompressEnd:
    LzmaDec_Free(&dec, &myAlloc);

    return 0;
}

int
main(int argc, char * argv[])
{
    if (argc != 4 || (argv[1][0] != 'x' && argv[1][0] != 'c'))
    {
        std::cerr << "usage: " << argv[0] << " [c|x] <infile> <outfile>"
                  << std::endl;
        return 1;
    }
    
    // open the in & out files
    std::ifstream inFile;
    std::ofstream outFile;
    inFile.open(argv[2], std::ios::binary | std::ios::in);
    outFile.open(argv[3], std::ios::binary | std::ios::trunc | std::ios::out);

    if (!inFile.is_open()) {
        std::cerr << "couldn't open file for reading: " << argv[1]
                  << std::endl;
        return 1;
    }

    if (!outFile.is_open()) {
        std::cerr << "couldn't open file for writing: " << argv[2]
                  << std::endl;
        return 1;
    }

    int rv = 1;
    
    if (argv[1][0] == 'c') {
        rv = doCompress(inFile, outFile);
    } else if (argv[1][0] == 'x') {
        rv = doUncompress(inFile, outFile);        
    }
    

    inFile.close();
    outFile.close();    

    return 0;
}

