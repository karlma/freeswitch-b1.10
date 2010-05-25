#ifndef __SMA_BUFFER_H__
#define __SMA_BUFFER_H__
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "buffer.h"

typedef struct {
    size_t len;
    BUFF_TYPE *data;
    BUFF_TYPE sma;
    size_t pos;
    size_t lpos;
} sma_buffer_t;

#define INIT_SMA_BUFFER(b, l, s) \
    do{ \
	(void)memset((b), 0, sizeof(sma_buffer_t)); \
	(b)->len = (l); \
	(b)->data = (BUFF_TYPE *)switch_core_session_alloc((s), sizeof(BUFF_TYPE) * (l)); \
	assert((b)->data != NULL); \
	(void)memset((b)->data, 0, sizeof(BUFF_TYPE) * (l)); \
	(b)->sma = 0.0; \
	(b)->pos = 0; \
	(b)->lpos = 0; \
    }while(0)

#define GET_SMA_SAMPLE(b, p) ((b)->data[(p) % (b)->len])
#define SET_SMA_SAMPLE(b, p, v) ((b)->data[(p) % (b)->len] = (v))
#define GET_CURRENT_SMA_POS(b) ((b)->lpos)

#define INC_SMA_POS(b) \
    do { \
	(b)->lpos++; \
	(b)->pos = (b)->lpos % (b)->len; \
    }while(0)

#define APPEND_SMA_VAL(b, v) \
    do{ \
	INC_SMA_POS(b); \
	(b)->sma -= ((b)->data[(b)->pos] / (BUFF_TYPE)(b)->len); \
	(b)->data[(b)->pos] = (v); \
	(b)->sma += ((b)->data[(b)->pos] / (BUFF_TYPE)(b)->len); \
    }while(0)

#define RESET_SMA_BUFFER(b) \
    do{ \
	(b)->sma = 0.0; \
	(void)memset((b)->data, 0, sizeof(BUFF_TYPE) * (b)->len); \
    }while(0)

/*
#define DESTROY_SMA_BUFFER(b) \
    do{ \
	free((b)->data); \
    }while(0);

*/
#endif
/*

int main(void)
{
    int i;
    sma_buffer_t b;

    INIT_SMA_BUFFER(&b, 100);

    for(i = 0; i < 20; i++){
	APPEND_SMA_VAL(&b, 100.0);
	printf("SMA = %lf\n", b.sma);
    }

    DESTROY_SMA_BUFFER(&b);

    return EXIT_SUCCESS;
}

*/

