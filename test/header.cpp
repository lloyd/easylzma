// code showing us how to parse "proper" lzma file headers.
// we need to augment with stuff to read and write these 13 byte
// headers simply.

#include "header.h"

#include <string.h>
#include <assert.h>
#include <iostream>

/****************
  Header parsing
 ****************/

#define int_fast8_t char
#define uint_fast8_t unsigned char
#define uint8_t unsigned char
#define uint_fast32_t unsigned int
#define uint_fast64_t unsigned long long
#define UINT64_MAX ((unsigned long long) -1)

/* Parse the properties byte */
static int_fast8_t
lzmadec_header_properties (
	uint_fast8_t *pb, uint_fast8_t *lp, uint_fast8_t *lc, const uint8_t c)
{
//	 pb, lp and lc are encoded into a single byte. 
	if (c > (9 * 5 * 5))
		return -1;
	*pb = c / (9 * 5);        // 0 <= pb <= 4 
    *lp = (c % (9 * 5)) / 9;  // 0 <= lp <= 4 
    *lc = c % 9;              // 0 <= lc <= 8 
    
	assert (*pb < 5 && *lp < 5 && *lc < 9);
	return 0;
}

// Parse the dictionary size (4 bytes, little endian) 
static int_fast8_t
lzmadec_header_dictionary (uint_fast32_t *size, const uint8_t *buffer)
{
	uint_fast32_t i;
	*size = 0;
	for (i = 0; i < 4; i++)
		*size += (uint_fast32_t)(*buffer++) << (i * 8);
	// The dictionary size is limited to 256 MiB (checked from
	// LZMA SDK 4.30)
	if (*size > (1 << 28))
		return -1;
	return 0;
}

// Parse the uncompressed size field (8 bytes, little endian) 
static void
lzmadec_header_uncompressed (uint_fast64_t *size, uint_fast8_t *is_streamed,
	const uint8_t *buffer)
{
	uint_fast32_t i;

//	 Streamed files have all 64 bits set in the size field.
//	   We don't know the uncompressed size beforehand. 
	*is_streamed = 1;  //Assume streamed. 
	*size = 0;
	for (i = 0; i < 8; i++) {
		*size += (uint_fast64_t)buffer[i] << (i * 8);
		if (buffer[i] != 255)
			*is_streamed = 0;
	}
	assert ((*is_streamed == 1 && *size == UINT64_MAX)
			|| (*is_streamed == 0 && *size < UINT64_MAX));
}


void
initLzmaHeader(my_lzma_header * hdr)
{
    memset((void *) hdr, 0, sizeof(my_lzma_header));
}

bool
parseLzmaHeader(const unsigned char * hdrBuf, my_lzma_header * hdr)
{
    if (lzmadec_header_properties(&(hdr->pb), &(hdr->lp), &(hdr->lc),
                                  *hdrBuf) ||
        lzmadec_header_dictionary(&(hdr->dictSize), hdrBuf + 1))
    {
        return false;
    }
    lzmadec_header_uncompressed(&(hdr->uncompressedSize),
                                &(hdr->isStreamed),
                                hdrBuf + 5);
    
//     std::cout << "read lzma header" << std::endl;
//     std::cout << "lc: " << (int) hdr->lc << std::endl;
//     std::cout << "pb: " << (int) hdr->pb << std::endl;
//     std::cout << "lp: " << (int) hdr->lp << std::endl;
//     std::cout << "dictSize: " << hdr->dictSize << std::endl;
//     std::cout << "isStreamed: " << (hdr->isStreamed ? "yes" : "no")
//               << std::endl;
//     std::cout << "uncompressed: " << hdr->uncompressedSize << std::endl;

    return true;
}

bool serializeLzmaHeader(unsigned char * hdrBuf, const my_lzma_header * hdr)
{
    memset((void *) hdrBuf, 0, MY_LZMA_HEADER_SIZE); 
//     std::cout << "writing lzma header" << std::endl;
//     std::cout << "lc: " << (int) hdr->lc << std::endl;
//     std::cout << "pb: " << (int) hdr->pb << std::endl;
//     std::cout << "lp: " << (int) hdr->lp << std::endl;

    // encode lc, pb, and lp
    *hdrBuf++ = hdr->lc + (hdr->pb * 45) + (hdr->lp * 45 * 9);

    // encode dictionary size
    for (unsigned int i = 0; i < 4; i++) {
        *(hdrBuf++) = (unsigned char) (hdr->dictSize >> (i * 8)); 
//         printf("hdrbuf %d: %x\n", i, *(hdrBuf - 1));
    }

//     std::cout << "dictSize: " << hdr->dictSize << std::endl;

    // encode uncompressed size
    for (unsigned int i = 0; i < 8; i++) {
        if (hdr->isStreamed) {
            *(hdrBuf++) = 0xff; 
        } else {
            *(hdrBuf++) = (unsigned char) (hdr->uncompressedSize << (i * 8)); 
        }
    }

//     std::cout << "isStreamed: " << (hdr->isStreamed ? "yes" : "no")
//               << std::endl;
//     std::cout << "uncompressed: " << hdr->uncompressedSize << std::endl;

    return true;
}
