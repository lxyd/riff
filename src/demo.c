#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include "vector.h"
#include "riff.h"

VEC_DECLARE_TYPE(fourcc_t, fourccv_t);
fourccv_t skipv;
fourccv_t readv;
fourccv_t stopv;

riff_cbresult_t chunk_cb (const riff_chunkv_t * chunk_path_ptr, FILE * f, uint64_t position, void * cookie) {
    riff_chunk_t cur;
    char buf[5];
    int i;

    cur = VEC_TOP(*chunk_path_ptr);
    fourcc_to_string(cur.fourcc, buf);

    assert(cur.pos_content == position);

    for(i=0; i<VEC_SIZE(skipv); i++) {
        if(VEC_GET(skipv, i) == VEC_TOP(*chunk_path_ptr).fourcc) {
            printf("Skipping: %s\n", buf);
            return RIFF_SKIP_CHUNK;
        }
    }
    for(i=0; i<VEC_SIZE(stopv); i++) {
        if(VEC_GET(stopv, i) == VEC_TOP(*chunk_path_ptr).fourcc) {
            printf("Stoping: %s\n", buf);
            return RIFF_STOP;
        }
    }
    for(i=0; i<VEC_SIZE(readv); i++) {
        if(VEC_GET(readv, i) == VEC_TOP(*chunk_path_ptr).fourcc) {
            printf("Reading: %s\n", buf);
            for(; position < cur.pos_end; position++) {
                fgetc(f);
            }
            return RIFF_CHUNK_READ;
        }
    }
    for(i=0;i<VEC_SIZE(*chunk_path_ptr);i++) {
        cur = VEC_GET(*chunk_path_ptr, i);
        fourcc_to_string(cur.fourcc, buf);
        printf("%s:", buf);
    }
    cur = VEC_TOP(*chunk_path_ptr);
    printf(" %" PRIu64 " - %" PRIu64 "\n", cur.pos_head, cur.pos_end);
    return RIFF_CONTINUE; /* go print subchunks */
}

int main(int argc, char * argv[]){
    riff_result_t res;
    int argi = 1;
    VEC_INIT(skipv);
    VEC_INIT(readv);
    VEC_INIT(stopv);

    while(argc > argi) {
        if(0 == strcmp(argv[argi], "-h") || 0 == strcmp(argv[argi], "--help")) {
            printf(
                "USAGE: %s [FLAGS]\n"
                "Reads RIFF file from standard input\n"
                "FLAGS:\n"
                "  -h (--help)         Print this message\n"
                "  -s (--skip) fourcc  Skip chunk with this fourcc\n"
                "  -r (--read) fourcc  Read chunk with this fourcc\n"
                "  -p (--stop) fourcc  Stop on chunk with this fourcc\n"
                , argv[0]);
            return 0;
        } else if(0 == strcmp(argv[argi], "-s") || 0 == strcmp(argv[argi], "--skip")) {
            argi++;
            VEC_PUSH(skipv, (*(fourcc_t*)argv[argi]));
            argi++;
        } else if(0 == strcmp(argv[argi], "-r") || 0 == strcmp(argv[argi], "--read")) {
            argi++;
            VEC_PUSH(readv, (*(fourcc_t*)argv[argi]));
            argi++;
        } else if(0 == strcmp(argv[argi], "-p") || 0 == strcmp(argv[argi], "--stop")) {
            argi++;
            VEC_PUSH(stopv, (*(fourcc_t*)argv[argi]));
            argi++;
        }
    }

    res = riff_readfile(stdin, chunk_cb, NULL, NULL);
    printf("%d\n", res);
    return 0;
}
