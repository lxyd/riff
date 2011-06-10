#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include "vector.h"
#include "riff.h"

const int BUF_SIZE = 1024*1024;

riff_cbresult_t chunk_cb (const riff_chunkv_t * chunk_path_ptr, FILE * f_in, uint64_t position, void * cookie) {
    FILE * f_out = (FILE*)cookie;
    riff_chunk_t cur;

    uint8_t * buf;
    uint32_t read, read_total = 0;
    uint32_t size;

    cur = VEC_TOP(*chunk_path_ptr);

    if(cur.fourcc == FOURCC("data")) {
        buf = calloc(BUF_SIZE, sizeof(uint8_t));
        size = cur.pos_end - cur.pos_content;
        
        read_total = 0;
        do {
            read = fread(buf, sizeof(uint8_t), BUF_SIZE > (size - read_total) ? (size - read_total) : BUF_SIZE, f_in);
            fwrite(buf, sizeof(uint8_t), read, f_out);
            read_total += read;
        } while (read_total < size);

        free(buf);

        return RIFF_STOP; /* don't need to parse futher more */
    }
    return RIFF_CONTINUE;
}

int main(int argc, char * argv[]){
    /*riff_result_t res;*/
    int argi = 1;

    while(argc > argi) {
        if(0 == strcmp(argv[argi], "-h") || 0 == strcmp(argv[argi], "--help")) {
            printf(
                "USAGE: %s [FLAGS]\n"
                "Reads WAV file from standard input and writes PCM data to standart output\n"
                "Limitations: WAV files cannot be greater than 4G\n"
                "FLAGS:\n"
                "  -h (--help)         Print this message\n"
                , argv[0]);
            return 0;
        }
    }

    riff_readfile(stdin, chunk_cb, NULL, stdout);
    return 0;
}
