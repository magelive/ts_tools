#ifndef TS_PARSE_H
#define TS_PARSE_H

#include <stdio.h>
#include <stdlib.h>

#include "ts_header.h"
#include "ts_pes.h"

typedef struct TS_stream {
    uint32_t id;
    uint32_t len;
    uint8_t *data;
    struct TS_stream *next;
}TS_stream_t;

typedef struct TS {
    TS_PAT_t pat;
    TS_PMT_t pmt;
    TS_CAT_t cat;
    TS_stream_t *stream_header;
}TS_t;

#endif