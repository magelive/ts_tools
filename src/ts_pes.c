#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ts_header.h"
#include "ts_pes.h"

int TS_PES_parse(TS_packet_t *packet, int offset, TS_PES_t *pes)
{
	uint8_t* PES_buf = packet->data + offset;
    if (PES_buf[0] != 0x00 || PES_buf[1] != 0x00 || PES_buf[2] != 0x01) {
		return -1;
    }
	int poffset = 0;
	pes->packet_start_code_pprefix = PES_buf[poffset++] << 16 | 
									 PES_buf[poffset++] << 8 | 
									 PES_buf[poffset++];
	pes->stream_id = PES_buf[poffset++];
	pes->PES_packet_length = PES_buf[poffset++] << 8 | PES_buf[poffset++];

	if (pes->stream_id != PROGRAM_STREAM_MAP && pes->stream_id != PADDING_STREAM &&
		pes->stream_id != PRIVATE_STRAME_2 && pes->stream_id != ECM_STREAM &&
		pes->stream_id != EMM_STREAM && pes->stream_id != PROGRAM_STREAM_DIRECTORY &&
		pes->stream_id != DSMCC_STREAM && pes->stream_id != ITU_T_H222_1_E) {
		
		pes->PES_scrambling_control = (PES_buf[poffset] >> 4) & 0b11;
		pes->PES_priority = (PES_buf[poffset] >> 3) & 0b1;
		pes->data_alignment_indicator = (PES_buf[poffset] >> 2) & 0b1;
		pes->copyright = (PES_buf[poffset] >> 1) & 0b1;
		pes->original_or_copy = (PES_buf[poffset++]) & 0b1;

		pes->PTS_DTS_flags = (PES_buf[poffset] >> 6) & 0b11;
		pes->ESCR_flag = (PES_buf[poffset] >> 5) & 0b1;
		pes->ES_rate_flag = (PES_buf[poffset] >> 4) & 0b1;
		pes->DSM_trick_mode_flag = (PES_buf[poffset] >> 3) & 0b1;
		pes->additional_copy_info_flag = (PES_buf[poffset] >> 2) & 0b1;
		pes->PES_CRC_flag = (PES_buf[poffset] >> 1) & 0b1;
		pes->PES_extension_flag = PES_buf[poffset++] & 0b1;

		pes->PES_header_data_length = PES_buf[poffset++];
		uint8_t header_start_indicator = poffset;

		if (pes->PTS_DTS_flags == 0b10 || pes->PTS_DTS_flags == 0b11) {

			uint32_t pts_32_30 = (PES_buf[poffset++] >> 1) & 0x07;
			uint32_t pts_29_15 = (PES_buf[poffset++] << 7) |
								((PES_buf[poffset++] >> 1) & 0x7F);
			uint32_t pts_14_0 = (PES_buf[poffset++] << 7) |
								((PES_buf[poffset++] >> 1) & 0x7F);
			pes->PTS = (pts_32_30 << 30 | pts_29_15 << 15 | pts_14_0);

			if (pes->PTS_DTS_flags == 0b11) {
				uint32_t dts_32_30 = (PES_buf[poffset++] >> 1) & 0x07;
				uint32_t dts_29_15 = (PES_buf[poffset++] << 7) |
									((PES_buf[poffset++] >> 1) & 0x7F);
				uint32_t dts_14_0 = (PES_buf[poffset++] << 7) |
									((PES_buf[poffset++] >> 1) & 0x7F);
				pes->DTS = (dts_32_30 << 30 | dts_29_15 << 15 | dts_14_0);
			}
		}
		if (pes->ESCR_flag) {
			uint64_t tmp = (uint64_t)PES_buf[poffset++] << 40 | (uint64_t)PES_buf[poffset++] << 32 |
							(uint64_t)PES_buf[poffset++] << 24 | (uint64_t)PES_buf[poffset++] << 16 |
							(uint64_t)PES_buf[poffset++] << 8 | (uint64_t)PES_buf[poffset++];

			uint32_t ESCR_base_32_30 = (uint32_t)((tmp >> 43) & 0x07);
			uint32_t ESCR_base_29_15 = (uint32_t)((tmp >> 27) & 0x7F);
			uint32_t ESCR_base_14_0 = (uint32_t)((tmp>> 11) & 0x7F);
			pes->ESCR_base = (ESCR_base_32_30 << 30 | ESCR_base_29_15 << 15 | ESCR_base_14_0);

			pes->ESCR_extension = (uint32_t)((tmp >> 1) & 0x1FF);
		}

		if (pes->ES_rate_flag) {
			pes->ES_rate =  (PES_buf[poffset++] & 0x7F) << 15 |
							(PES_buf[poffset++] << 7) |
							(PES_buf[poffset++] >> 1) & 0x7F;
		}

		if (pes->DSM_trick_mode_flag) {
			pes->trick_mode_control = (PES_buf[poffset] >> 5) & 0b111;
			pes->trick_data = PES_buf[poffset++] & 0x1F;
		}

		if (pes->additional_copy_info_flag) {
			pes->additional_copy_info = PES_buf[poffset++] & 0x7F;
		}

		if (pes->PES_CRC_flag) {
			pes->previous_PES_packet_CRC = PES_buf[poffset++] << 8 | PES_buf[poffset++];
		}

		if (pes->PES_extension_flag) {
			pes->PES_private_data_flag = (PES_buf[poffset] >> 7) & 0b1;
			pes->pack_header_field_flag = (PES_buf[poffset] >> 6) & 0b1;
			pes->program_packet_sequence_counter_flag = (PES_buf[poffset] >> 5) & 0b1;
			pes->PSTD_buffer_flag = (PES_buf[poffset] >> 4) & 0b1;
			pes->PES_extension_flag_2 = (PES_buf[poffset++]) & 0x01;
			if (pes->PES_private_data_flag) {
				memcpy(pes->PES_private_data, PES_buf+poffset, 16);
				poffset += 16;
			}
			if (pes->pack_header_field_flag) {
				pes->pack_field_length = PES_buf[poffset++];
				poffset += pes->pack_field_length;
			}
			if (pes->program_packet_sequence_counter_flag) {
				pes->program_packet_sequence_counter = PES_buf[poffset++] & 0x7F;
				pes->MPEG1_MPEG2_identifier = (PES_buf[poffset] >> 6) & 0b1;
				pes->original_stuff_length = PES_buf[poffset++] & 0x3F;
			}
			if (pes->PSTD_buffer_flag) {
				pes->PSTD_buffer_scale = (PES_buf[poffset] >> 5) & 0x1;
				pes->PSTD_buffer_size = (PES_buf[poffset++] & 0x1F) << 8 | PES_buf[poffset++];
			}
			if (pes->PES_extension_flag_2) {
				pes->PES_extension_field_length = PES_buf[poffset++] & 0x7F;
				poffset += pes->PES_extension_field_length;
			}	
		}

		//stuffing
		uint8_t stuffing_size = pes->PES_header_data_length - (poffset - header_start_indicator);
		poffset += stuffing_size;
		pes->data = PES_buf + poffset;
		if (pes->PES_packet_length == 0) {
			pes->data_len = packet->length - (offset + poffset);
		} else {
			pes->data_len = pes->PES_packet_length - poffset;
		}
	} else if (pes->stream_id == PROGRAM_STREAM_MAP || pes->stream_id == PRIVATE_STRAME_2 ||
				pes->stream_id == ECM_STREAM || pes->stream_id == EMM_STREAM ||
				pes->stream_id == PROGRAM_STREAM_DIRECTORY || pes->stream_id == DSMCC_STREAM ||
				pes->stream_id == ITU_T_H222_1_E) {
				if (pes->PES_packet_length == 0) {
					pes->data_len = packet->length - (offset + poffset);
				} else {
					pes->data_len = pes->PES_packet_length - poffset;
				}
	} else if (pes->stream_id == PADDING_STREAM) {
		pes->data_len = 0;
		pes->data = NULL;
	} 
	return poffset;
}