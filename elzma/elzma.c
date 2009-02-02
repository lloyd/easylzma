/**
 * command line elzma tool for lzma compression
 *
 * At time of writing, the primary purpose of this tool is to test the
 * easylzma library.
 *
 * TODO:
 *  - stdin/stdout support
 *  - multiple file support
 *  - much more
 */

#include "easylzma/compress.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define ELZMA_USAGE \
"Compress files using the LZMA algorithm (by default, in place).\n"\
"\n"\
"Usage: elzma [options] [file]\n"\
"  -1 .. -9       compression level, -1 is fast, -9 is best\n"\
"  -h, --help     output this message and exit\n"\
"  -v, --verbose  output verbose status information while compressing\n"\
"\n"\
"Advanced Options:\n"\
"  -s --set-dict   (advanced) specify LZMA dictionary size in bytes\n"

/* parse arguments populating output parameters, return nonzero on failure */
static int parseArgs(int argc, char ** argv, unsigned char * level,
                     char ** fname, unsigned int * dictSize,
                     unsigned int * verbose)
{
    unsigned int i;
    
    if (argc < 2) return 1;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            char * val = NULL;
            char * arg = &(argv[i][1]);
            if (arg[0] == '-') arg++;

            /* now see what argument this is */
            if (!strcmp(arg, "h") || !strcmp(arg, "help"))
            {
                return 1;
            }
            else if (!strcmp(arg, "s") || !strcmp(arg, "set-dict"))
            {
                unsigned int j = 0;
                val = argv[++i];
                
                /* validate argument is numeric */
                for (j = 0; j < strlen(val); j++) {
                    if (val[j] < '0' || val[j] > '9') return 1;
                }

                *dictSize = strtoul(val, (char **) NULL, 10);
            }
            else if (!strcmp(arg, "v") || !strcmp(arg, "verbose"))
            {
                *verbose = 1;
            }
            else if (strlen(arg) == 1 && arg[0] >= '1' && arg[0] <= '9')
            {
                *level = arg[0] - '0';
            }
            else
            {
                return 1;
            }
        }
        else 
        {
            *fname = argv[i];
            break;
        }
    }

    /* proper number of arguments? */
    if (i != argc - 1 || *fname == NULL) return 1;
    
    return 0;
}

/* callbacks for streamed input and output */
static size_t elzmaWriteFunc(void *ctx, const void *buf, size_t size)
{
    size_t wt;
    FILE * f = (FILE *) ctx;
    assert(f != NULL);

    wt = fwrite(buf, 1, size, f);
    printf("wrote %lu bytes\n", wt);
    
    return wt;
}


static int elzmaReadFunc(void *ctx, void *buf, size_t *size)
{
    FILE * f = (FILE *) ctx;
    assert(f != NULL);
    *size = fread(buf, 1, *size, f);
    printf("read %lu bytes\n", *size);
    return 0;
}

int
main(int argc, char ** argv)
{
    /* default compression parameters, some of which may be overridded by
     * command line arguments */
    unsigned char level = 5;
    unsigned char lc = ELZMA_LC_DEFAULT;
    unsigned char lp = ELZMA_LP_DEFAULT;
    unsigned char pb = ELZMA_PB_DEFAULT;
    unsigned int dictSize = ELZMA_DICT_SIZE_DEFAULT;
    elzma_file_format format = ELZMA_lzma;
    char * ext = ".lzma";
    char * ifname = NULL;
    char * ofname = NULL;
    unsigned int verbose = 0;
    FILE * inFile = NULL;
    FILE * outFile = NULL;
    elzma_compress_handle hand = NULL;
    /* XXX: large file support */
    unsigned int uncompressedSize = 0;

    if (0 != parseArgs(argc, argv, &level, &ifname, &dictSize, &verbose))
    {
        fprintf(stderr, ELZMA_USAGE);
        return 1;
    }

    /* XXX: type/extension switching */

    /* generate output file name */
    {
        ofname = malloc(strlen(ifname) + strlen(ext) + 1);
        ofname[0] = 0;
        strcat(ofname, ifname);
        strcat(ofname, ext);
    }
    
    /* now attempt to open input and ouput files */
    /* XXX: stdin/stdout support */
    {
        printf("opening '%s'\n", ifname);
        inFile = fopen(ifname, "rb");
        if (inFile == NULL) {
            fprintf(stderr, "couldn't open '%s' for reading\n", ifname);
            return 1;
        }
        
        /* set uncompressed size */
        if (0 != fseek(inFile, 0, SEEK_END) ||
            0 == (uncompressedSize = ftell(inFile)) ||
            0 != fseek(inFile, 0, SEEK_SET))
        {
            fprintf(stderr, "error seeking input file (%s) - zero length?\n",
                    ifname);
            return 1;
        }
        
        outFile = fopen(ofname, "wb");
        if (outFile == NULL) {
            fprintf(stderr, "couldn't open '%s' for writing\n", ofname);
            return 1;
        }
    }

    if (verbose) {
        printf("compressing '%s' to '%s'\n", ifname, ofname);
        printf("lc/lp/pb = %u/%u/%u | dictionary size = %u bytes\n",
               lc, lp, pb, dictSize);        
        printf("input file is %u bytes\n", uncompressedSize);
    }

    /* allocate a compression handle */
    hand = elzma_compress_alloc();
    if (hand == NULL) {
        fprintf(stderr, "couldn't allocate compression object\n");
        return 1;
    }
    
    if (ELZMA_E_OK != elzma_compress_config(hand, lc, lp, pb, level,
                                            dictSize, format,
                                            uncompressedSize))
    {
        fprintf(stderr, "couldn't configure compression with "
                "provided parameters\n");
        return 1;
    }
    
    if (ELZMA_E_OK != elzma_compress_run(
            hand, elzmaReadFunc, (void *) inFile,
            elzmaWriteFunc, (void *) outFile))
    {
        fprintf(stderr, "error compressing\n");
        return 1;
    }

    /* clean up */
    elzma_compress_free(&hand);
    fclose(inFile);
    fclose(outFile);
    free(ofname);

    return 0;
}
