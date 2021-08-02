
#ifndef TS_OUT_H
#define TS_OUT_H

#include "ts_header.h"
#include "ts_pes.h"

#ifndef PREFIX_PRINT
#define PREFIX_PRINT fprintf(stdout, "%s", print_prefix)
#endif

char *get_stream_description(int stream_id);

void TS_packet_print(TS_packet_t *packet, char *print_prefix);
void TS_adaptation_print(TS_adaptation_t *adaptation, char *print_prefix);
void TS_PAT_print(TS_PAT_t *pat, char *print_prefix);
void TS_CAT_print(TS_CAT_t *cat, char *print_prefix);
void TS_PMT_print(TS_PMT_t *pmt, char *print_prefix);
void TS_PES_print(TS_PES_t *pes, char *print_prefix);

#endif