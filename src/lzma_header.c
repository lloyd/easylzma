/* code showing us how to parse "proper" lzma file headers.
 * we need to augment with stuff to read and write these 13 byte
 * headers simply. */

#include "lzma_header.h"

#include <string.h>
#include <assert.h>

/****************
  Header parsing
 ****************/

#define UINT64_MAX ((unsigned long long) -1)

/* Parse the properties byte */
static char
lzmadec_header_properties (
	unsigned char *pb, unsigned char *lp, unsigned char *lc, const unsigned char c)
{
/*	 pb, lp and lc are encoded into a single byte.  */
	if (c > (9 * 5 * 5))
		return -1;
	*pb = c / (9 * 5);        /* 0 <= pb <= 4 */
    *lp = (c % (9 * 5)) / 9;  /* 0 <= lp <= 4 */
    *lc = c % 9;              /* 0 <= lc <= 8 */
    
	assert (*pb < 5 && *lp < 5 && *lc < 9);
	return 0;
}

/* Parse the dictionary size (4 bytes, little endian) */
static char
lzmadec_header_dictionary (unsigned int *size, const unsigned char *buffer)
{
	unsigned int i;
	*size = 0;
	for (i = 0; i < 4; i++)
		*size += (unsigned int)(*buffer++) << (i * 8);
	/* The dictionary size is limited to 256 MiB (checked from
	 * LZMA SDK 4.30) */
	if (*size > (1 << 28))
		return -1;
	return 0;
}

/* Parse the uncompressed size field (8 bytes, little endian) */
static void
lzmadec_header_uncompressed (unsigned long long *size, unsigned char *is_streamed,
	const unsigned char *buffer)
{
	unsigned int i;

    /* Streamed files have all 64 bits set in the size field.
     * We don't know the uncompressed size beforehand. */
	*is_streamed = 1;  /* Assume streamed. */
	*size = 0;
	for (i = 0; i < 8; i++) {
		*size += (unsigned long long)buffer[i] << (i * 8);
		if (buffer[i] != 255)
			*is_streamed = 0;
	}
	assert ((*is_streamed == 1 && *size == UINT64_MAX)
			|| (*is_streamed == 0 && *size < UINT64_MAX));
}

void
initLzmaHeader(struct elzma_lzma_header * hdr)
{
    memset((void *) hdr, 0, sizeof(struct elzma_lzma_header));
}

int
parseLzmaHeader(const unsigned char * hdrBuf,
                struct elzma_lzma_header * hdr)
{
    if (lzmadec_header_properties(&(hdr->pb), &(hdr->lp), &(hdr->lc),
                                  *hdrBuf) ||
        lzmadec_header_dictionary(&(hdr->dictSize), hdrBuf + 1))
    {
        return 1;
    }
    lzmadec_header_uncompressed(&(hdr->uncompressedSize),
                                &(hdr->isStreamed),
                                hdrBuf + 5);
    
    return 0;
}

int
serializeLzmaHeader(unsigned char * hdrBuf,
                    const struct elzma_lzma_header * hdr)
{
    unsigned int i;
    
    memset((void *) hdrBuf, 0, ELZMA_LZMA_HEADER_SIZE); 

    /* encode lc, pb, and lp */
    *hdrBuf++ = hdr->lc + (hdr->pb * 45) + (hdr->lp * 45 * 9);

    /* encode dictionary size */
    for (i = 0; i < 4; i++) {
        *(hdrBuf++) = (unsigned char) (hdr->dictSize >> (i * 8)); 
    }

    /* encode uncompressed size */
    for (i = 0; i < 8; i++) {
        if (hdr->isStreamed) {
            *(hdrBuf++) = 0xff; 
        } else {
            *(hdrBuf++) = (unsigned char) (hdr->uncompressedSize >> (i * 8)); 
        }
    }

    return 0;
}
