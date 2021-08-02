#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <strings.h>
#include <string.h>

#include "ts_header.h"
#include "ts_pes.h"

int TS_header_parse(uint8_t *TS_buf, int packet_size, TS_packet_t *packet)
{
	packet->sync_byte                                     = TS_buf[0];
	if  (packet->sync_byte != 0x47)
		return  -1;
	packet->transport_error_indicator       = TS_buf[1] >> 7;
	packet->payload_unit_start_indicator    = TS_buf[1] >> 6 & 0x01;
	packet->transport_priority             = TS_buf[1] >> 5 & 0x01;
	packet->pid                         = (TS_buf[1] & 0x1F) << 8 | TS_buf[2];
	packet->transport_scrambling_control   = TS_buf[3] >> 6;
	packet->adaption_field_control         = TS_buf[3] >> 4 & 0x03;
	packet->continuity_counter            = TS_buf[3] & 0x0F;
	packet->length = packet_size - TS_HEADER_LEN;
    packet->data = TS_buf + TS_HEADER_LEN;
	return  0;
}

//return data offset
int TS_adaptation_parse(TS_packet_t *packet, int adaptation_offset, TS_adaptation_t *adaptation)
{
    int offset = 0, tmp_offset;
    uint8_t *adaptation_data = packet->data+adaptation_offset;
    adaptation->adaptation_filed_length = adaptation_data[offset];
    offset += 1;
    if (adaptation->adaptation_filed_length <= 0) {
        return offset;
    }

    adaptation->discontinuity_indicator = adaptation_data[offset] >> 7;
    adaptation->random_access_indicator = (adaptation_data[offset] >> 6) & 0x01;
    adaptation->elementary_stream_priority_indicator = (adaptation_data[offset] >> 5) & 0x01;
    adaptation->PCR_flag = (adaptation_data[offset] >> 4) & 0x01;
    adaptation->OPCR_flag = (adaptation_data[offset] >> 3) & 0x01;
    adaptation->splicing_point_flag = (adaptation_data[offset] >> 2) & 0x01;
    adaptation->transport_private_data_flag = (adaptation_data[offset] >> 1) & 0x01;
    adaptation->adaptation_field_extension_flag = adaptation_data[offset] & 0x01;

    offset += 1;

    if (adaptation->PCR_flag) {
        adaptation->program_clock_reference_base = (adaptation_data[offset] << 25) |
                                                    adaptation_data[offset+1] << 17 |
                                                    adaptation_data[offset+2] << 9 |
                                                    adaptation_data[offset+3] << 1 |
                                                    (adaptation_data[offset+4] >> 7) & 0x01;
        //adaptation->pcr_reserved = adaptation_data[offset+4] >> 1 & 0x3F;
        adaptation->program_clock_reference_extension = (adaptation_data[offset+4] & 0x01) << 8 | adaptation_data[offset+5];
        offset += 5;
    }

    if (adaptation->OPCR_flag) {
        adaptation->original_program_clock_reference_base = (adaptation_data[offset] << 25) |
                                                    adaptation_data[offset+1] << 17 |
                                                    adaptation_data[offset+2] << 9 |
                                                    adaptation_data[offset+3] << 1 |
                                                    (adaptation_data[offset+4] >> 7) & 0x01;
        //adaptation->opcr_reserved = adaptation_data[offset+4] >> 1 & 0x3F;
        adaptation->original_program_clock_reference_extension = (adaptation_data[offset+4] & 0x01) << 8 | adaptation_data[offset+5];
        offset += 5;
    }

    if (adaptation->splicing_point_flag) {
        adaptation->splice_countdown = adaptation_data[offset];
        offset += 1;
    }

    if (adaptation->transport_private_data_flag) {
        adaptation->transport_private_data_length = adaptation_data[offset];
        offset += 1;
        if (adaptation->transport_private_data_length > 0) {
            adaptation->transport_private_data = adaptation_data + offset;
            offset += adaptation->transport_private_data_length;
        }
    }

    if (adaptation->adaptation_field_extension_flag) {
        tmp_offset = offset;
        adaptation->adaptation_field_extension_length = adaptation_data[tmp_offset];
        adaptation->ltw_flag = (adaptation_data[tmp_offset+1] >> 7) & 0x01;
        adaptation->piecewise_rate_flag = (adaptation_data[tmp_offset+1] >> 6) & 0x01;
        adaptation->seamless_splice_flag = (adaptation_data[tmp_offset+1] >> 6) & 0x01;
        adaptation->extension_reserved = (adaptation_data[tmp_offset+1]) & 0x1F;
        tmp_offset += 2;

        if (adaptation->ltw_flag) {
            adaptation->ltw_valid_flag = (adaptation_data[tmp_offset] >> 7) & 0x01;
            adaptation->ltw_offset = (adaptation_data[tmp_offset] & 0x7F) << 8 | adaptation_data[tmp_offset+1];
            tmp_offset += 2;
        }

        if (adaptation->piecewise_rate_flag) {
            adaptation->piecewise_rate_reserved = (adaptation_data[tmp_offset] >> 6) & 0x03;
            adaptation->piecewise_rate = (adaptation_data[tmp_offset] & 0x3F) << 16 |
                                            adaptation_data[tmp_offset+1] << 8 |
                                            adaptation_data[tmp_offset+2];
            tmp_offset += 3;
        }

        if (adaptation->seamless_splice_flag) {
            adaptation->splice_type = (adaptation_data[tmp_offset] >> 4) & 0x0F;
            adaptation->DTS_next_AU = ((adaptation_data[tmp_offset] >> 1) & 0x03) << 30 |
                                        adaptation_data[tmp_offset+1] << 23 |
                                        (adaptation_data[tmp_offset+2] >> 1) << 15 |
                                        (adaptation_data[tmp_offset+3] & 0x7F) << 7 |
                                        adaptation_data[tmp_offset+4] >> 1;

            adaptation->marker_bit[0] = adaptation_data[tmp_offset] & 0x01;
            adaptation->marker_bit[1] = adaptation_data[tmp_offset+2] & 0x01;
            adaptation->marker_bit[2] = adaptation_data[tmp_offset+4] & 0x01;
            tmp_offset += 5;
        }

        adaptation->adaptation_field_extension_reserved_len = 
            adaptation->adaptation_field_extension_length - (tmp_offset - offset);
        adaptation->adaptation_field_extension_reserved = adaptation_data + tmp_offset;
        offset += adaptation->adaptation_field_extension_length;
    }
    
    if (packet->adaption_field_control != 0x03) {
        offset = packet->length;
    } else {
        offset = adaptation->adaptation_filed_length + 1;
    }

    return offset;
}

int TS_check_is_PMT(TS_PAT_t *pat, int pid)
{
    TS_PAT_program_t *ptmp = pat->programs;
    while(ptmp) {
        if (ptmp->program_map_PID == pid) {
            return 1;
        }
        ptmp = ptmp->next;
    }
    return 0;
}

/*****************PAT*****************************/
int TS_PAT_get_network_PID(TS_PAT_t *pat)
{
    int find = 0;
    TS_PAT_program_t *ptmp = pat->programs;
    while(ptmp) {
        if (ptmp->program_number == 0x0000) {
            find = 1;
            break;
        }
        ptmp = ptmp->next;
    }
    if (find) {
        return ptmp->program_map_PID;
    }
    return -1;
}

void TS_PAT_free(TS_PAT_t *pat)
{
    TS_PAT_program_t *ptmp = pat->programs, *next;
    while(ptmp) {
        next = ptmp->next;
        free(ptmp);
        ptmp = next;
    }
    if (pat->section_free_flag && pat->section_data) {
        free(pat->section_data);
    }
    bzero(pat, sizeof(TS_PAT_t));
    return;
}

int TS_PAT_parse(TS_packet_t *packet, int offset, TS_PAT_t *pat)
{
    uint8_t* PAT_buf = packet->data + offset;
    int rlen = 0; //Process offset
    if (packet->payload_unit_start_indicator == 1) {
        TS_PAT_free(pat);
        pat->table_id = PAT_buf[0];
        pat->section_syntax_indicator = PAT_buf[1] >> 7;
        pat->zero = PAT_buf[1] >> 6 & 0x1;
        //pat->reserved_1 = PAT_buf[1] >> 4 & 0x3;
        pat->section_length = (PAT_buf[1] & 0x0F) << 8 | PAT_buf[2];

        if (pat->section_length > (packet->length - (offset+3)) ) {
            pat->section_data = malloc(pat->section_length);
            if (!pat->section_data) {
                return -1;
            }
            pat->section_free_flag = 1;
            memcpy(pat->section_data, packet->data+offset + 3, packet->length - (offset+3));
            pat->section_index = packet->length - (offset+3);
            return (packet->length - (offset+3));
        } else {
            pat->section_data = PAT_buf + 3;
            pat->section_index = pat->section_length;
            rlen = pat->section_length;
        }
    } else {
        int __len = pat->section_length - pat->section_index;
        __len = __len > packet->length?packet->length:__len;
        memcpy(pat->section_data + pat->section_index, packet->data, __len);
        pat->section_index += __len;
        rlen = __len; 
    }

    if (pat->section_index == pat->section_index) {
        PAT_buf = pat->section_data;
        pat->transport_stream_id = PAT_buf[0] << 8 | PAT_buf[1];
        pat->reserved_2 = PAT_buf[2] >> 6;
        pat->version_number = PAT_buf[2] >> 1 & 0x1F;
        pat->current_next_indicator = (PAT_buf[2] << 7) >> 7;
        pat->section_number = PAT_buf[3];
        pat->last_section_number = PAT_buf[4];
        pat->crc32 = (PAT_buf[pat->section_length - 4] & 0xFF) << 24
            | (PAT_buf[pat->section_length - 3] & 0xFF) << 16
            | (PAT_buf[pat->section_length - 2] & 0xFF) << 8
            | (PAT_buf[pat->section_length - 1] & 0xFF);

        int n = 0, find = 0;;
        uint32_t program_number, reserved, pid;
        TS_PAT_program_t *ptmp, *pnext, *pprev;
        for (n = 0; n < (pat->section_length - 12); n += 4) {
            program_number = PAT_buf[5 + n] << 8 | PAT_buf[6 + n];
            //reserved = PAT_buf[7 + n] >> 5;
            pid = (PAT_buf[7 + n] & 0x1F) << 8 | PAT_buf[8 + n];
            
            if (pid == 0x1FFF) { //Null packet
                continue;
            }

            find = 0;
            pnext = pat->programs;
            while(pnext && pnext->program_number <= program_number) {
                if (pnext->program_number == program_number  ) {
                    find = 1;
                    break;
                }
                pnext = pnext->next;
            }
            if (find) {
                continue;
            }

            ptmp = malloc(sizeof(TS_PAT_program_t));
            if (!ptmp) {
                fprintf(stderr, "OOM at %s:%d", __func__, __LINE__);
                return -1;
            }

            ptmp->program_number = program_number;
            ptmp->program_map_PID = pid;
            ptmp->next = NULL;
        
            pprev = NULL;
            pnext = pat->programs;
            while(pnext) {
                if (ptmp->program_number < pnext->program_number) {
                    break;
                }
                pprev = pnext;
                pnext = pnext->next;
            }
            ptmp->next = pnext;
            if (!pnext && pprev) {
                pprev->next = ptmp;
            } 
            if (!pprev) {
                pat->programs = ptmp;
            }
        }
    }
    return rlen;
}

/*******************CAT***************************/
int TS_CAT_parse(TS_packet_t *packet, int offset, TS_CAT_t *cat)
{
    if (packet->pid != TS_PID_CAT) {
        return -1;
    }
    int rlen = 0;
    uint8_t* CAT_buf = packet->data + offset;
    
    if (packet->payload_unit_start_indicator) {
        if (cat->section_data) {
            free(cat->section_data);
        }
        bzero(cat, sizeof(TS_CAT_t));

        cat->table_id = CAT_buf[0];
        cat->section_syntax_indicator = CAT_buf[1] >> 7;
        //cat->zero = CAT_buf[1] >> 6 & 0x1;
        //cat->reserved_1 = CAT_buf[1] >> 4 & 0x3;
        cat->section_length = (CAT_buf[1] & 0x0F) << 8 | CAT_buf[2];
        cat->section_data = malloc(cat->section_length);
        if (!cat->section_data) {
            return -1;
        }

        if (cat->section_length > (packet->length - (offset + 3))) {
            memcpy(cat->section_data, packet->data+offset + 3, packet->length - (offset+3));
            cat->section_index = packet->length - (offset+3);
            return (packet->length - (offset+3));
        } else {
            memcpy(cat->section_data, packet->data+offset + 3, cat->section_length);
            cat->section_index = cat->section_length;
        }
    } else {
        int __len = cat->section_length - cat->section_index;
        __len = __len > packet->length?packet->length:__len;
        memcpy(cat->section_data + cat->section_index, packet->data, __len);
        cat->section_index += __len;
        rlen = __len; 
    }

    if (cat->section_index == cat->section_length) {
        CAT_buf = cat->section_data;
        cat->version_number = CAT_buf[2] >> 1 & 0x1F;
        cat->current_next_indicator = (CAT_buf[2] << 7) >> 7;
        cat->section_number = CAT_buf[3];
        cat->last_section_number = CAT_buf[4];
        cat->crc32 = (CAT_buf[cat->section_length - 4] & 0x000000FF) << 24
            | (CAT_buf[cat->section_length - 3] & 0x000000FF) << 16
            | (CAT_buf[cat->section_length - 2] & 0x000000FF) << 8
            | (CAT_buf[cat->section_length - 1] & 0x000000FF);

        cat->descriptor_len = cat->section_length - 12;
        cat->descriptor = CAT_buf+5;
    }
    return rlen;
}

/******************PMT****************************/
int TS_check_is_PMT_stream(TS_PMT_t *pmt, int stream_id)
{
    //printf("%s %d\n", __func__, stream_id);
    TS_PMT_stream_t *tmp = pmt->streams;
    while(tmp != NULL) {
        //printf("\t list pid: %d\n", tmp->elementary_PID);
        if (stream_id == tmp->elementary_PID) {
            return 1;
        }
        tmp = tmp->next;
    }
    return 0;
}

void TS_PMT_free(TS_PMT_t *pmt)
{
    TS_PMT_stream_t *stream = pmt->streams, *next;
    while(stream) {
        next = stream->next;
        free(stream);
        stream = next;
    }
    if (pmt->section_free_flag && pmt->section_data) {
        free(pmt->section_data);
    }
    bzero(pmt, sizeof(TS_PMT_t));
    return;
}

int TS_PMT_parse(TS_packet_t *packet, int offset, TS_PMT_t *pmt)
{
    uint8_t* PMT_buf = packet->data + offset;
    int rlen = 0;

    if (packet->payload_unit_start_indicator) {
        pmt->table_id = PMT_buf[0];
        pmt->section_syntax_indicator = PMT_buf[1] >> 7;
        //pmt->zero = PMT_buf[1] >> 6 & 0x1;
        //pmt->reserved_1 = PMT_buf[1] >> 4 & 0x3;
        pmt->section_length = (PMT_buf[1] & 0x0F) << 8 | PMT_buf[2];
        if (pmt->section_length > (packet->length - (offset+3)) ) {
            pmt->section_data = malloc(pmt->section_length);
            if (!pmt->section_data) {
                return -1;
            }
            pmt->section_free_flag = 1;
            memcpy(pmt->section_data, packet->data+offset + 3, packet->length - (offset+3));
            pmt->section_index = packet->length - (offset+3);
            return (packet->length - (offset+3));
        } else {
            pmt->section_data = PMT_buf + 3;
            pmt->section_index = pmt->section_length;
            rlen = pmt->section_length;
        }
    } else {
        int __len = pmt->section_length - pmt->section_index;
        __len = __len > packet->length?packet->length:__len;
        memcpy(pmt->section_data + pmt->section_index, packet->data, __len);
        pmt->section_index += __len;
        rlen = __len; 
    }

    if (pmt->section_index == pmt->section_length) {
        PMT_buf = pmt->section_data;
        pmt->program_number = PMT_buf[0] << 8 | PMT_buf[1];
        pmt->version_number = PMT_buf[2] >> 1 & 0x1F;
        pmt->current_next_indicator = (PMT_buf[2] << 7) >> 7;
        pmt->section_number = PMT_buf[3];
        pmt->last_section_number = PMT_buf[4];
        pmt->PCR_PID = (PMT_buf[5] & 0x1F) << 8 | PMT_buf[6];
        pmt->program_info_length = (PMT_buf[7] & 0x0F ) << 8 | PMT_buf[8];
        pmt->program_info = &PMT_buf[9];
        pmt->crc32 = (PMT_buf[pmt->section_length - 4] & 0x000000FF) << 24
            | (PMT_buf[pmt->section_length - 3] & 0x000000FF) << 16
            | (PMT_buf[pmt->section_length - 2] & 0x000000FF) << 8
            | (PMT_buf[pmt->section_length - 1] & 0x000000FF);

        int pos = pmt->program_info_length + 9;
        TS_PMT_stream_t *stream = NULL;
        while (pos < (pmt->section_length - 4)) {
            stream = malloc(sizeof(TS_PMT_stream_t));
            if (!stream) {
                fprintf(stderr, "OOM at PMT parse.\n");
                return -1;
            }
            bzero(stream, sizeof(TS_PMT_stream_t));
            stream->stream_type = PMT_buf[pos];
            stream->elementary_PID = (PMT_buf[pos+1] & 0x1F) << 8 | PMT_buf[pos+2];
            stream->ES_info_length = (PMT_buf[pos+3] & 0x0F) << 8 | PMT_buf[pos+4];
            stream->ES_info = &PMT_buf[pos+5];
            pos += stream->ES_info_length + 5;

            stream->next = pmt->streams;
            pmt->streams = stream;
        }
    }

    return rlen;
}
