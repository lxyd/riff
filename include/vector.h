#ifndef __vector_h_
#define __vector_h_

#include <stdlib.h>

/*
    USAGE EXAMPLES:

    VEC_DECLARE_TYPE(long, lv_t) - let the type lv_t be a vector of long
    lvector_t v, *vptr;          - just declare var and pointer
    VEC_INIT(v)                  - initialize of normal vector
    VEC_INIT(*vptr)              - ... and this also works
    VEC_DESTROY(v)               - free the memory allocated to the v's data
    VEC_DESTROY(*vptr)           - ... same for the type 'lv_t *'
*/

#define VEC_MIN_CAPACITY 4

#define VEC_DECLARE_TYPE(ELEMENT_TYPE, VEC_TYPENAME)                           \
    typedef struct {                                                           \
        ELEMENT_TYPE * data;                                                   \
        size_t _cap;                                                           \
        size_t size;                                                           \
    } VEC_TYPENAME 

#define VEC_INIT(VAR)                                                          \
    (VAR).size = 0;                                                            \
    (VAR)._cap = VEC_MIN_CAPACITY;                                             \
    (VAR).data = malloc(sizeof(*(VAR).data) * (VAR)._cap) 

#define VEC_DESTROY(VAR)                                                       \
    free((VAR).data);                                                          \
    (VAR).data = NULL;                                                         \
    (VAR).size = -1;                                                           \
    (VAR)._cap = -1 

#define VEC_PUSH(VAR, VAL)                                                     \
    if((VAR)._cap <= (VAR).size) {                                             \
        (VAR)._cap *= 2;                                                       \
        (VAR).data = realloc((VAR).data, sizeof(*(VAR).data) * (VAR)._cap);    \
    }                                                                          \
    (VAR).data[(VAR).size] = (VAL);                                            \
    (VAR).size++ 

#define VEC_COMPACT(VAR)                                                       \
    if((VAR)._cap > (VEC_MIN_CAPACITY * 2) && (VAR)._cap > ((VAR).size * 2)) { \
		(VAR)._cap /= 2;                                                       \
		(VAR).data = realloc((VAR).data, sizeof(*(VAR).data) * (VAR)._cap);    \
	}

#define VEC_GET(VAR, IDX)      ((VAR).data[IDX])
#define VEC_INDEXOF(VAR, VAL)  (&(VAL) - (VAR).data)
#define VEC_SIZE(VAR)          ((VAR).size)
#define VEC_TOP(VAR)           ((VAR).data[(VAR).size-1])
#define VEC_POP(VAR)           ((VAR).data[--(VAR).size])

#endif
