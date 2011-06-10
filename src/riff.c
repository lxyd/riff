#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "vector.h"
#include "riff.h"

void fourcc_to_string(fourcc_t fourcc, char * buf) {
    *((fourcc_t*)buf) = fourcc;
    buf[4] = '\0';
}

int fourcc_is_group(fourcc_t fourcc) {
    return fourcc == FCC_RIFF || fourcc == FCC_LIST 
        || fourcc == FCC_FORM || fourcc == FCC_CAT;
}

/* ensure file offset is even 
   because chunks can start from even byte only */
#define ALIGN_EVEN(f, offset)         \
    if ((offset) % 2) {               \
        (offset)++;                   \
        (void)getc(f);                \
    }

#define CLEAR_CHUNK(c)                \
    (c).pos_head = (c).pos_end =      \
    (c).pos_content      = 0;         \
    (c).fourcc           = FCC_NULL;  \
    (c).data.riff.format = FCC_NULL;  \
    (c).data.list.type   = FCC_NULL;

static int _riff_fskip(FILE * f, uint64_t skip) {
    int res = 0;
    fpos_t cur_pos;
    int step_skip;
    (void)fgetpos(f, &cur_pos);
    while(skip > 0 && res == 0) {
        step_skip = skip > INT32_MAX ? INT32_MAX : skip;
        res = fseek(f, step_skip, SEEK_CUR);
        skip -= step_skip;
    }
    /* go back if skip failed */
    if(res != 0)
        (void)fsetpos(f, &cur_pos);
    return res;
}

static riff_result_t _riff_read_chunk_content (FILE * f,
                                        riff_chunkv_t * path_ptr,
                                        uint64_t * foffset_ptr,
                                        riff_chunkcb chunk_cb,
                                        void * cookie) {
    uint32_t skip_bytes_left = VEC_TOP(*path_ptr).pos_end - *foffset_ptr;
    riff_chunk_t next_subchunk, cur_chunk;
    fourcc_t fourcc;
    uint32_t size;
    riff_result_t subchunk_res;
    enum {
        A_READ_SUBCHUNKS, /* when chunk is a group AND callback returned R_CONTINUE */
        A_SKIP_CHUNK,     /* when chunk is NOT a group OR callback returned R_SKIP */
        A_UPDATE_OFFSET,  /* when callback returned R_CHUNK_IS_READ */
    } next_action;

    if(VEC_TOP(*path_ptr).fourcc == FCC_RIFF) {
        *foffset_ptr += sizeof(fourcc_t);
        skip_bytes_left -= sizeof(fourcc_t);
        /* advance pos_content after RIFF format fourcc code */
        VEC_TOP(*path_ptr).pos_content += sizeof(fourcc_t);
        if(1 != fread(&VEC_TOP(*path_ptr).data.riff.format, sizeof(fourcc_t), 1, f)) {
            return ferror(f) ? RIFF_READFAILED : RIFF_CHUNKINCOMPLETE;
        }
    } else if(VEC_TOP(*path_ptr).fourcc == FCC_LIST) {
        *foffset_ptr += sizeof(fourcc_t);
        skip_bytes_left -= sizeof(fourcc_t);
        /* advance pos_content after LIST type fourcc code */
        VEC_TOP(*path_ptr).pos_content += sizeof(fourcc_t);
        if(1 != fread(&VEC_TOP(*path_ptr).data.list.type, sizeof(fourcc_t), 1, f)) {
            return ferror(f) ? RIFF_READFAILED : RIFF_CHUNKINCOMPLETE;
        }
    }
    
    cur_chunk = VEC_TOP(*path_ptr);

    /* all chunk data is read
       subchunks left */
    next_action = fourcc_is_group(cur_chunk.fourcc) ? A_READ_SUBCHUNKS : A_SKIP_CHUNK;
    if(chunk_cb != NULL) {
        /* call callback function */
        switch(chunk_cb(path_ptr, f, *foffset_ptr, cookie)) {
            case RIFF_CONTINUE:
                /* do nothing: next_action is choosen right */
                break;
            case RIFF_SKIP_CHUNK:
                next_action = A_SKIP_CHUNK;
                break;
            case RIFF_CHUNK_READ:
                next_action = A_UPDATE_OFFSET;
                break;
            case RIFF_STOP:
                return RIFF_STOPPED;
                break;
            default:
                assert(0);
                break;
        }
    }

    switch(next_action) {

        case A_READ_SUBCHUNKS:

            ALIGN_EVEN(f, *foffset_ptr);
            while(*foffset_ptr < cur_chunk.pos_end && 1 == fread(&fourcc, sizeof(fourcc_t), 1, f)) {
                CLEAR_CHUNK(next_subchunk);
                next_subchunk.fourcc = fourcc;
                next_subchunk.pos_head = *foffset_ptr; /* before FourCC */
                *foffset_ptr += sizeof(fourcc_t);

                if(1 != fread(&size, sizeof(uint32_t), 1, f)) {
                    return ferror(f) ? RIFF_READFAILED : RIFF_CHUNKINCOMPLETE;
                }
                *foffset_ptr += sizeof(uint32_t);
                
                next_subchunk.pos_content = *foffset_ptr; /* after FourCC and size */
                next_subchunk.pos_end   = next_subchunk.pos_content + size;

                if(next_subchunk.pos_end > cur_chunk.pos_end) {
                    return RIFF_SUBCHUNKTOOLONG;
                }

                /* push incomplete chunk so that _riff_read_chunk_content could finish it */
                VEC_PUSH(*path_ptr, next_subchunk);
                if(RIFF_SUCCESS != (subchunk_res = _riff_read_chunk_content(f, path_ptr, foffset_ptr, chunk_cb, cookie))) {
                    return subchunk_res;
                }
                (void)VEC_POP(*path_ptr);
                
                ALIGN_EVEN(f, *foffset_ptr);
            }
            
            /* check if we haven't read whole chunk */
            if(*foffset_ptr < cur_chunk.pos_end) {
                return ferror(f) ? RIFF_READFAILED : RIFF_CHUNKINCOMPLETE;
            }
            break;

        case A_SKIP_CHUNK:

            if(0 != _riff_fskip(f, skip_bytes_left)) {
                /* tried to seek after EOF */
                return RIFF_CHUNKINCOMPLETE;
            }
            *foffset_ptr += skip_bytes_left;
            break;

        case A_UPDATE_OFFSET:

            *foffset_ptr += skip_bytes_left;
            break;

        default:

            assert(0);
            break;
    }
    
    return RIFF_SUCCESS;
}

riff_result_t riff_readfile (FILE * f, 
                             riff_chunkcb chunk_cb,
                             riff_errorcb err_cb,
                             void * cookie) {
    uint64_t foffset;    /* offset in the stream */
    riff_result_t res = RIFF_SUCCESS;
    riff_chunk_t next_chunk;
    fourcc_t fourcc;     /* FourCC code of the next chunk */
    uint32_t size;

    riff_chunkv_t path;       /* Path from root to the current chunk */
    VEC_INIT(path);
    
    foffset = 0;
    while(1 == fread(&fourcc, sizeof(fourcc_t), 1, f)) {
        CLEAR_CHUNK(next_chunk);
        next_chunk.fourcc = fourcc;
        next_chunk.pos_head = foffset; /* before FourCC */
        foffset += sizeof(fourcc_t);

        if(next_chunk.fourcc != FCC_RIFF) {
            res = RIFF_WRONGFCC;
            break;
        }

        if(1 != fread(&size, sizeof(uint32_t), 1, f)) {
            res = ferror(f) ? RIFF_READFAILED : RIFF_CHUNKINCOMPLETE;
            break;
        }
        foffset += sizeof(uint32_t);

        next_chunk.pos_content = foffset; /* after FourCC and size */
        next_chunk.pos_end     = next_chunk.pos_content + size;

        /* push incomplete chunk so that _riff_read_chunk_content could finish it */
        VEC_PUSH(path, next_chunk);
        if(RIFF_SUCCESS != (res = _riff_read_chunk_content(f, &path, &foffset, chunk_cb, cookie))) {
            break;
        }
        (void)VEC_POP(path);

        ALIGN_EVEN(f, foffset);
    }
    
    if(res == RIFF_SUCCESS) {
        /* if there was not any error in the body of the loop */
        if(ferror(f)) {
            /* if fread returned 0 because of file error */
            res = RIFF_READFAILED;
        }
    }

    if(res != RIFF_SUCCESS && res != RIFF_STOPPED && err_cb != NULL) {
        err_cb(&path, f, foffset, res, cookie);
    }
    VEC_DESTROY(path);
    return res;
}
