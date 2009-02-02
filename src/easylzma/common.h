#ifndef __EASYLZMACOMMON_H__ 
#define __EASYLZMACOMMON_H__ 

/** error codes */

/** no error */
#define ELZMA_E_OK                               0
/** bad parameters passed to an ELZMA function */
#define ELZMA_E_BAD_PARAMS                      10
/** could not initialize the encode with configured parameters. */
#define ELZMA_E_ENCODING_PROPERTIES_ERROR       11
/** an error occured during compression (XXX: be more specific) */
#define ELZMA_E_COMPRESS_ERROR                  12
/** currently unsupported lzma file format was specified*/
#define ELZMA_E_UNSUPPORTED_FORMAT              13
/** an error occured when writing output */
#define ELZMA_E_OUTPUT_ERROR                    14


#endif
