#ifndef __riff_h__
#define __riff_h__

#include <stdio.h>   /* for FILE */
#include <stdint.h>  /* for uint64_t, UINT32_MAX */
#include "vector.h"

/* FourCC is a 32bit integer containing 4 
   ascii codes of alphanumeric characters */
typedef uint32_t fourcc_t;

/* FourCC used as default */
#define FCC_NULL ((fourcc_t)0u)

/* Predefined FourCC codes for group chunks */
#define FCC_RIFF (*((fourcc_t*)"RIFF"))
#define FCC_LIST (*((fourcc_t*)"LIST"))
#define FCC_FORM (*((fourcc_t*)"FORM"))
#define FCC_CAT  (*((fourcc_t*)"CAT "))

#define MAX_CHUNK_LENGTH UINT32_MAX

typedef union {
	struct {
		fourcc_t format; /* Chunk format. E.g. 'WAVE' */
	} riff;
	struct {
		fourcc_t type;   /* List type. E.g. 'INFO' */
	} list;
	/* TODO: other chunk types */
} riff_chunkdata_t;

typedef struct {
    fourcc_t fourcc;       /* FourCC code: 'RIFF', 'LIST', etc */
    uint64_t pos_head;     /* Chunk's FourCC position */
    uint64_t pos_content;  /* First byte after chunk header
                              (header includes chunk FourCC, size and,
                              possibly, chunk format/type) */
    uint64_t pos_end;      /* Position after the last byte of the chunk */
    riff_chunkdata_t data; /* Data for particular chunk type */
} riff_chunk_t;

typedef	enum {
	RIFF_SUCCESS         = 0,    /* chunk was read successfully */
    RIFF_STOPPED         = 1,    /* riff parsing was stopped by callback */
	RIFF_READFAILED      = -100, /* fread() error */
	RIFF_CHUNKINCOMPLETE = -101, /* EOF before the end of chunk */
	RIFF_SUBCHUNKTOOLONG = -102, /* subchunk ends after the end of chunk */
	RIFF_WRONGFCC        = -103  /* unexpected FourCC */
} riff_result_t;

/* Chunk callback should return one of the following: */
typedef enum {
    RIFF_CONTINUE        = 0, /* riff parsing must be continued */
    RIFF_CHUNK_READ      = 1, /* callback have read the chunk: offset needs to be updated */
    RIFF_SKIP_CHUNK      = 2, /* current chunk must be skipped */
    RIFF_STOP            = 3  /* riff parsing must be stopped */
} riff_cbresult_t;

VEC_DECLARE_TYPE(riff_chunk_t, riff_chunkv_t);

/* Callback is called after the chunk's head is read */
typedef riff_cbresult_t (*riff_chunkcb) (const riff_chunkv_t * chunk_path_ptr,
                                         FILE * file,
                                         uint64_t position,
                                         void * cookie);

/* Callback is called whenever an error occurs */
typedef void (*riff_errorcb) (const riff_chunkv_t * chunk_path_ptr,
                              FILE * file,
                              uint64_t position,
                              riff_result_t error,
                              void * cookie);

/* functions */

void fourcc_to_string   (fourcc_t fourcc, char * buf);
int  fourcc_is_group    (fourcc_t fourcc);
riff_result_t riff_readfile (FILE * file, 
                             riff_chunkcb chunk_cb,
                             riff_errorcb err_cb,
                             void * cookie /* an arbitrary user data to be passed to callbacks */);

#endif
