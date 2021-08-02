
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <strings.h>
#include <string.h>

#include "ts_header.h"
#include "ts_pes.h"

#ifndef PREFIX_PRINT
#define PREFIX_PRINT fprintf(stdout, "%s", print_prefix)
#endif

typedef struct TS_stream_description {
	int stream_id;
	char *description;
}TS_stream_description_t;

TS_stream_description_t TS_stream_description[] = {
	{0x00, "Reserved"},
	{0x01, "ISO/IEC 11172-2 (MPEG-1 video)"},
	{0x02, "ITU-T Rec. H.262 and ISO/IEC 13818-2 (MPEG-2 higher rate interlaced video)"},
	{0x03, "ISO/IEC 11172-3 (MPEG-1 audio)"},
	{0x04, "ISO/IEC 13818-3 (MPEG-2 halved sample rate audio)"},
	{0x05, "ITU-T Rec. H.222 and ISO/IEC 13818-1 (MPEG-2 tabled data)"},
	{0x06, "ITU-T Rec. H.222 and ISO/IEC 13818-1 (MPEG-2 packetized data)"},
	{0x07, "ISO/IEC 13522 (MHEG)"},
	{0x08, "ITU-T Rec. H.222 and ISO/IEC 13818-1 DSM CC"},
	{0x09, "ITU-T Rec. H.222 and ISO/IEC 13818-1/11172-1 auxiliary data"},
	{0x0A, "ISO/IEC 13818-6 DSM CC multiprotocol encapsulation"},
	{0x0B, "ISO/IEC 13818-6 DSM CC U-N messages"},
	{0x0C, "ISO/IEC 13818-6 DSM CC stream descriptors"},
	{0x0D, "ISO/IEC 13818-6 DSM CC tabled data"},
	{0x0E, "ISO/IEC 13818-1 auxiliary data"},
	{0x0F, "ISO/IEC 13818-7 ADTS AAC (MPEG-2 lower bit-rate audio)"},
	{0x10, "ISO/IEC 14496-2 (MPEG-4 H.263 based video)"},
	{0x11, "ISO/IEC 14496-3 (MPEG-4 LOAS multi-format framed audio)"},
	{0x12, "ISO/IEC 14496-1 (MPEG-4 FlexMux)"},
	{0x13, "ISO/IEC 14496-1 (MPEG-4 FlexMux)in ISO/IEC 14496 tables"},
	{0x14, "ISO/IEC 13818-6 DSM CC synchronized download protocol"},
	{0x15, "Packetized metadata"},
	{0x16, "Sectioned metadata"},
	{0x17, "ISO/IEC 13818-6 DSM CC Data Carousel metadata"},
	{0x18, "ISO/IEC 13818-6 DSM CC Object Carousel metadata"},
	{0x19, "ISO/IEC 13818-6 Synchronized Download Protocol metadata"},
	{0x1A, "ISO/IEC 13818-11 IPMP"},
	{0x1B, "ITU-T Rec. H.264 and ISO/IEC 14496-10 (lower bit-rate video)"},
	{0x1C, "ISO/IEC 14496-3 (MPEG-4 raw audio)"},
	{0x1D, "ISO/IEC 14496-17 (MPEG-4 text)"},
	{0x1E, "ISO/IEC 23002-3 (MPEG-4 auxiliary video)"},
	{0x1F, "ISO/IEC 14496-10 SVC (MPEG-4 AVC sub-bitstream)"},
	{0x20, "ISO/IEC 14496-10 MVC (MPEG-4 AVC sub-bitstream)"},
	{0x21, "ITU-T Rec. T.800 and ISO/IEC 15444 (JPEG 2000 video)"},
	{0x24, "ITU-T Rec. H.265 and ISO/IEC 23008-2 (Ultra HD video)"},
	{0x42, "Chinese Video Standard"},
	{0x7f, "ISO/IEC 13818-11 IPMP (DRM)"},
	{0x80, "ITU-T Rec. H.262 and ISO/IEC 13818-2 with DES-64-CBC encryption for DigiCipher II or PCM audio for Blu-ray"},
	{0x81, "Dolby Digital (AC-3) up to six channel audio for ATSC and Blu-ray"},
	{0x82, "SCTE subtitle or DTS 6 channel audio for Blu-ray"},
	{0x83, "Dolby TrueHD lossless audio for Blu-ray"},
	{0x84, "Dolby Digital Plus (enhanced AC-3) up to 16 channel audio for Blu-ray"},
	{0x85, "DTS 8 channel audio for Blu-ray"},
	{0x86, "SCTE-35[5] digital program insertion cue message or DTS 8 channel lossless audio for Blu-ray"},
	{0x87, "Dolby Digital Plus (enhanced AC-3) up to 16 channel audio for ATSC"},
	{0x90, "Blu-ray Presentation Graphic Stream (subtitling)"},
	{0x91, "ATSC DSM CC Network Resources table"},
	{0xC0, "DigiCipher II text"},
	{0xC1, "Dolby Digital (AC-3) up to six channel audio with AES-128-CBC data encryption"},
	{0xC2, "ATSC DSM CC synchronous data or Dolby Digital Plus up to 16 channel audio with AES-128-CBC data encryption"},
	{0xCF, "ISO/IEC 13818-7 ADTS AAC with AES-128-CBC frame encryption"},
	{0xD1, "BBC Dirac (Ultra HD video)"},
	{0xDB, "ITU-T Rec. H.264 and ISO/IEC 14496-10 with AES-128-CBC slice encryption"},
	{0xEA, "Microsoft Windows Media Video 9 (lower bit-rate video)"},
};

char *get_stream_description(int stream_id)
{
    int i;
    char *description = "Unknow";
    for(i = 0; i < sizeof(TS_stream_description)/sizeof(TS_stream_description_t); i++) {
        if (TS_stream_description[i].stream_id == stream_id) {
            description = TS_stream_description[i].description;
            break;
        }
    }
    return description;
}

void TS_adaptation_print(TS_adaptation_t *adaptation, char *print_prefix)
{
    int i, j;
    PREFIX_PRINT;fprintf(stdout,"%s", "adaptation:\n");
    PREFIX_PRINT;fprintf(stdout, "\tadaptation_filed_length: 0x%02x\n", adaptation->adaptation_filed_length);
    if (adaptation->adaptation_filed_length == 0) {
        return;
    }
    PREFIX_PRINT;fprintf(stdout, "\tdiscontinuity_indicator: 0x%02x\n", adaptation->discontinuity_indicator);
	PREFIX_PRINT;fprintf(stdout, "\trandom_access_indicator: 0x%02x\n", adaptation->random_access_indicator);
	PREFIX_PRINT;fprintf(stdout, "\telementary_stream_priority_indicator: 0x%02x\n", adaptation->elementary_stream_priority_indicator);

    PREFIX_PRINT;fprintf(stdout, "\tPCR_flag: 0x%02x\n", adaptation->PCR_flag);
    if (adaptation->PCR_flag) {
        //PCR_flag == 1
        //PREFIX_PRINT;fprintf(stdout, "\t\tprogram_clock_reference_base: %"PRIu64"\n", adaptation->program_clock_reference_base);
        PREFIX_PRINT;fprintf(stdout, "\t\tprogram_clock_reference_base: %llu\n", adaptation->program_clock_reference_base);
        //PREFIX_PRINT;fprintf(stdout, "\tpcr_reserved: 0x%02x\n", adaptation->pcr_reserved);
        PREFIX_PRINT;fprintf(stdout, "\t\tprogram_clock_reference_extension: 0x%02x\n", adaptation->program_clock_reference_extension);
    }

    PREFIX_PRINT;fprintf(stdout, "\tOPCR_flag: 0x%02x\n", adaptation->OPCR_flag);
    if (adaptation->OPCR_flag) {
        //OPCR_flag == 1
        PREFIX_PRINT;fprintf(stdout, "\t\toriginal_program_clock_reference_base %"PRIu64"\n", adaptation->original_program_clock_reference_base);
        //uint64_t opcr_reserved                              :6;
        PREFIX_PRINT;fprintf(stdout, "\t\toriginal_program_clock_reference_extension 0x%02x\n",
                                        adaptation->original_program_clock_reference_extension);
    }
    
    PREFIX_PRINT;fprintf(stdout, "\tsplicing_point_flag: 0x%02x\n", adaptation->splicing_point_flag);
    if (adaptation->splicing_point_flag) {
        //splicing_point_flag == 1
        PREFIX_PRINT;fprintf(stdout, "\t\tsplice_countdown: %u\n", adaptation->splice_countdown);
    }

    PREFIX_PRINT;fprintf(stdout, "\ttransport_private_data_flag: 0x%02x\n" , adaptation->transport_private_data_flag);
    if (adaptation->transport_private_data_flag) {
        //transport_private_data_flag == 1
        PREFIX_PRINT;fprintf(stdout, "\t\ttransport_private_data_length: 0x%02x\n", adaptation->transport_private_data_length);
        if (adaptation->transport_private_data_length > 0) {
            PREFIX_PRINT;fprintf(stdout, "\t\ttransport_private_data:\n");
            for (i = 0; i < adaptation->transport_private_data_length; i+=16) {
            //uint8_t *transport_private_data;
                PREFIX_PRINT;fprintf(stdout, "\t\t");
                for(j = 0; j < 16; j++) {
                    fprintf(stdout, "0x%02x ", adaptation->transport_private_data[i+j]);
                }
                fprintf(stdout, "\n");
            }
        }
	}
    PREFIX_PRINT;fprintf(stdout, "\tadaptation_field_extension_flag: 0x%02x\n", adaptation->adaptation_field_extension_flag);
    if (adaptation->adaptation_field_extension_flag) {
	    // adaptation_field_extension_flag == 1
        PREFIX_PRINT;fprintf(stdout, "\t\tadaptation_field_extension_length: 0x%02x\n", adaptation->adaptation_field_extension_length);
        
        PREFIX_PRINT;fprintf(stdout, "\t\tltw_flag: 0x%02x\n", adaptation->ltw_flag);
        if (adaptation->ltw_flag) {
            //ltw_flag == 1
            PREFIX_PRINT;fprintf(stdout, "\t\t\tltw_valid_flag: 0x%02x\n", adaptation->ltw_valid_flag);
            PREFIX_PRINT;fprintf(stdout, "\t\t\tltw_offset: 0x%02x\n", adaptation->ltw_offset);
        }

        PREFIX_PRINT;fprintf(stdout, "\t\tpiecewise_rate_flag: 0x%02x\n", adaptation->piecewise_rate_flag);
        if (adaptation->piecewise_rate_flag) {
            //piecewise_rate_flag == 1
	        //PREFIX_PRINT;fprintf(stdout, "\tpiecewise_rate_reserved                    :2;
	        PREFIX_PRINT;fprintf(stdout, "\t\t\tpiecewise_rate: 0x%02x\n", adaptation->piecewise_rate);
        }

        PREFIX_PRINT;fprintf(stdout, "\t\tseamless_splice_flag: 0x%02x\n", adaptation->seamless_splice_flag);
        //PREFIX_PRINT;fprintf(stdout, "\textension_reserved: 0x%02x\n", adaptation->extension_reserved);

        if (adaptation->seamless_splice_flag) {
            //seamless_splice_flag == 1
            PREFIX_PRINT;fprintf(stdout, "\t\t\tsplice_type: 0x%02x\n", adaptation->splice_type);
            PREFIX_PRINT;fprintf(stdout, "\t\t\tmarker_bit: %d, %d, %d\n", adaptation->marker_bit[0],
                adaptation->marker_bit[1], adaptation->marker_bit[2]);
            PREFIX_PRINT;fprintf(stdout, "\t\t\tDTS_next_AU: %u\n", adaptation->DTS_next_AU);
        }
    }
    fprintf(stdout, "\n");
    return;
}

void TS_PAT_print(TS_PAT_t *pat, char *print_prefix)
{
    TS_PAT_program_t *ptmp = pat->programs;
    PREFIX_PRINT;fprintf(stdout,"%s", "PAT packet:\n");
    PREFIX_PRINT;fprintf(stdout,"\ttable_id: 0x%02x\n", pat->table_id);
	PREFIX_PRINT;fprintf(stdout,"\tsection_syntax_indicator: 0x%02x\n", pat->section_syntax_indicator);
	PREFIX_PRINT;fprintf(stdout,"\tsection_length: %d\n", pat->section_length);
	PREFIX_PRINT;fprintf(stdout,"\ttransport_stream_id: %d\n", pat->transport_stream_id);
	PREFIX_PRINT;fprintf(stdout,"\tversion_number: %d\n", pat->version_number);
	PREFIX_PRINT;fprintf(stdout,"\tcurrent_next_indicator: %d\n", pat->current_next_indicator); 
	PREFIX_PRINT;fprintf(stdout,"\tsection_number: %d\n", pat->section_number);
    PREFIX_PRINT;fprintf(stdout,"\tlast_section_number: %d\n", pat->last_section_number); 
    PREFIX_PRINT;fprintf(stdout,"\tparograms:\n");
    while(ptmp) {
        PREFIX_PRINT;fprintf(stdout,"\t\tprogram number: %d, program map pid: 0x%04x(%d)\n", 
                        ptmp->program_number, ptmp->program_map_PID, ptmp->program_map_PID);
        ptmp = ptmp->next;
    }
    #if 0    
	if (pat->has_network_pid) {
	    PREFIX_PRINT;fprintf(stdout,"\tnetwork_pid: %d\n", pat->network_pid);
    }
    if (pat->has_program_map_pid) {
	    PREFIX_PRINT;fprintf(stdout,"\tprogram_map_pid: %d\n", pat->program_map_pid);
    }
    #endif

	PREFIX_PRINT;fprintf(stdout,"\tcrc32: 0x%08x\n", pat->crc32);
    return;
}

void TS_CAT_print(TS_CAT_t *cat, char *print_prefix)
{
    int i,j;
    PREFIX_PRINT;fprintf(stdout,"%s", "CAT packet:\n");
    PREFIX_PRINT;fprintf(stdout,"\ttable_id: 0x%02x\n", cat->table_id);
	PREFIX_PRINT;fprintf(stdout,"\tsection_syntax_indicator: 0x%02x\n", cat->section_syntax_indicator);
	PREFIX_PRINT;fprintf(stdout,"\tsection_length: %d\n", cat->section_length);
	PREFIX_PRINT;fprintf(stdout,"\tversion_number: %d\n", cat->version_number);
	PREFIX_PRINT;fprintf(stdout,"\tcurrent_next_indicator: %d\n", cat->current_next_indicator); 
	PREFIX_PRINT;fprintf(stdout,"\tlast_section_number: %d\n", cat->section_number);
    PREFIX_PRINT;fprintf(stdout,"\tlast_section_number: %d\n", cat->last_section_number);
    PREFIX_PRINT;fprintf(stdout,"\tdescriptor(%d):\n", cat->descriptor_len);
    for(i = 0; i < cat->descriptor_len; i+=16) {
        PREFIX_PRINT;fprintf(stdout, "\t\t");
        for(j = 0; j < 16; j++) {
            fprintf(stdout, "0x%02x ", cat->descriptor[i+j]);
        }
        fprintf(stdout, "\n");
    }
    
	PREFIX_PRINT;fprintf(stdout,"\tcrc32: 0x%08x\n", cat->crc32);
    return;
}

void TS_PMT_print(TS_PMT_t *pmt, char *print_prefix)
{
    int i,j;
    PREFIX_PRINT;fprintf(stdout,"%s", "PMT packet:\n");
    PREFIX_PRINT;fprintf(stdout,"\ttable_id: 0x%02x\n", pmt->table_id);
	PREFIX_PRINT;fprintf(stdout,"\tsection_syntax_indicator: 0x%02x\n", pmt->section_syntax_indicator);
	PREFIX_PRINT;fprintf(stdout,"\tsection_length: %d\n", pmt->section_length);
    PREFIX_PRINT;fprintf(stdout,"\tprogram number: %d\n", pmt->program_number);
	PREFIX_PRINT;fprintf(stdout,"\tversion_number: %d\n", pmt->version_number);
	PREFIX_PRINT;fprintf(stdout,"\tcurrent_next_indicator: %d\n", pmt->current_next_indicator); 
	PREFIX_PRINT;fprintf(stdout,"\tlast_section_number: %d\n", pmt->section_number);
    PREFIX_PRINT;fprintf(stdout,"\tlast_section_number: %d\n", pmt->last_section_number);
    PREFIX_PRINT;fprintf(stdout,"\tPCR_PID: %d\n", pmt->PCR_PID);
    PREFIX_PRINT;fprintf(stdout,"\tprogram info(%d):\n", pmt->program_info_length);
    for (i = 0; i < pmt->program_info_length; i+= 16) {
        PREFIX_PRINT;fprintf(stdout, "\t\t");
        for(j = 0; j < 16; j++) {
            fprintf(stdout, "0x%02x ", pmt->program_info[i+j]);
        }
        fprintf(stdout, "\n");
    }
    PREFIX_PRINT;fprintf(stdout,"\tstreams:\n");
    TS_PMT_stream_t *stream = pmt->streams;
    int index = 0, i_flag = 0;;
    while(stream) {
        PREFIX_PRINT;fprintf(stdout,"\t\tindex: %d\n", index++);
        PREFIX_PRINT;fprintf(stdout,"\t\t\tstream_type: 0x%02x(", stream->stream_type);
        i_flag = 0;
        for(i = 0; i < sizeof(TS_stream_description)/sizeof(TS_stream_description_t); i++) {
            if (TS_stream_description[i].stream_id == stream->stream_type) {
                fprintf(stdout, "%s", TS_stream_description[i].description);
                i_flag = 1;
                break;
            }
        }
        if (!i_flag) {
            fprintf(stdout, "Privately defined");
        }
        fprintf(stdout, ")\n");
        PREFIX_PRINT;fprintf(stdout,"\t\t\telementary_PID: %d\n", stream->elementary_PID);
        PREFIX_PRINT;fprintf(stdout,"\t\t\tES_info(%d):\n", stream->ES_info_length);
        for (i = 0; i < stream->ES_info_length; i+=8) {
            PREFIX_PRINT;fprintf(stdout, "\t\t\t\t");
            for(j = 0; j < 8; j++) {
                fprintf(stdout, "0x%02x ", stream->ES_info[i+j]);
            }
            fprintf(stdout, "\n");
        }
        stream = stream->next;
    } 

	PREFIX_PRINT;fprintf(stdout,"\tcrc32: 0x%08x\n", pmt->crc32);
    return;
}

void TS_PES_print(TS_PES_t *pes, char *print_prefix) 
{
	int i, j;
	PREFIX_PRINT;fprintf(stdout,"%s", "PES packet:\n");
	PREFIX_PRINT;fprintf(stdout,"\tstart prefix: 0x%02x 0x%02x 0x%02x\n", (pes->packet_start_code_pprefix >> 16) & 0xFF,
																	(pes->packet_start_code_pprefix >> 8) & 0xFF, 
																	pes->packet_start_code_pprefix & 0xFF);
	PREFIX_PRINT;fprintf(stdout,"\tstream_id: 0x%02x%s\n", pes->stream_id, 
						IS_AUDIO_STREAM(pes->stream_id)?"(Audio)":(IS_VIDEO_STREAM(pes->stream_id)?"(Video)":""));
	PREFIX_PRINT;fprintf(stdout,"\tPES_packet_length: %d\n", pes->PES_packet_length);
	if (pes->stream_id != PROGRAM_STREAM_MAP && pes->stream_id != PADDING_STREAM &&
		pes->stream_id != PRIVATE_STRAME_2 && pes->stream_id != ECM_STREAM &&
		pes->stream_id != EMM_STREAM && pes->stream_id != PROGRAM_STREAM_DIRECTORY &&
		pes->stream_id != DSMCC_STREAM && pes->stream_id != ITU_T_H222_1_E) {
		PREFIX_PRINT;fprintf(stdout,"\tPES_scrambling_control: 0x%02x\n", pes->PES_scrambling_control);
		PREFIX_PRINT;fprintf(stdout,"\tPES_priority: %d\n", pes->PES_priority);
		PREFIX_PRINT;fprintf(stdout,"\tdata_alignment_indicator: %d\n", pes->data_alignment_indicator);
		PREFIX_PRINT;fprintf(stdout,"\tcopyright: %d\n", pes->copyright);
		PREFIX_PRINT;fprintf(stdout,"\toriginal_or_copy: %d\n", pes->original_or_copy);
		if (pes->PTS_DTS_flags == 0b10 || pes->PTS_DTS_flags == 0b11) {
			PREFIX_PRINT;fprintf(stdout,"\tPTS: %u\n", pes->PTS);
			if (pes->PTS_DTS_flags == 0b11) {
				PREFIX_PRINT;fprintf(stdout,"\tDTS: %u\n", pes->DTS);
			}
		}
		if (pes->ESCR_flag) {
			PREFIX_PRINT;fprintf(stdout,"\tESCR_base: %u\n", pes->ESCR_base);
			PREFIX_PRINT;fprintf(stdout,"\tESCR_extension: %d\n", pes->ESCR_extension);
		}
		if (pes->ES_rate_flag) {
			PREFIX_PRINT;fprintf(stdout,"\tES_rate: %d\n", pes->ES_rate);
		}
		if (pes->DSM_trick_mode_flag) {
			PREFIX_PRINT;fprintf(stdout,"\ttrick_mode_control: 0x%02x\n", pes->trick_mode_control);
			switch (pes->trick_mode_control)
			{
				case FAST_FORWARD:
				case FAST_REVERSE:
					PREFIX_PRINT;fprintf(stdout, "\t\tfiled_id: %d\n", (pes->trick_data >> 3) & 0xFFFF);
					PREFIX_PRINT;fprintf(stdout, "\t\tintra_slice_refresh: %d\n", (pes->trick_data >> 2) & 0xFF);
					PREFIX_PRINT;fprintf(stdout, "\t\tfrequency_truncation: %d\n", (pes->trick_data & 0xFFFF));
					break;
				case SLOW_MOTION:
				case SLOW_REVERSE:
					PREFIX_PRINT;fprintf(stdout, "\t\trep_cntrl: %d\n", pes->trick_data);
					break;
				case FREEZE_FRAME:
					PREFIX_PRINT;fprintf(stdout, "\t\tfiled_id: %d\n", (pes->trick_data >> 3) & 0xFFFF);
					break;
			}
		}
		if (pes->additional_copy_info_flag) {
			PREFIX_PRINT;fprintf(stdout,"\tadditional_copy_info %d\n", pes->additional_copy_info);
		}
		if (pes->PES_CRC_flag) {
			PREFIX_PRINT;fprintf(stdout,"\t 0x%04x", pes->previous_PES_packet_CRC);
		}

		if (pes->PES_extension_flag) {
			if (pes->PES_private_data_flag) {
				PREFIX_PRINT;fprintf(stdout,"\tPES_private_data: \n");
				PREFIX_PRINT;(stdout,"\t\t");
				for ( i = 0; i < 128; i += 16) {
					for (j = i; j < i+16; j++) {
						fprintf(stdout, "%02x ", pes->PES_private_data[j]);
					}
					fprintf(stdout, "\n");
				}
			}
			if (pes->pack_header_field_flag) {
				PREFIX_PRINT;fprintf(stdout,"\tPack_filed_length: %d\n", pes->pack_field_length);
				PREFIX_PRINT;fprintf(stdout,"\tPack_filed_data:\n");
				PREFIX_PRINT;fprintf(stdout,"\t\t");
				for (i = 0; i < 16; i++) {
					fprintf(stdout, "%02x ", pes->pack_header[i]);
				}
				fprintf(stdout, "\n");
				PREFIX_PRINT;fprintf(stdout,"\t\t... ...\n");
			}
			if (pes->program_packet_sequence_counter_flag) {
				PREFIX_PRINT;fprintf(stdout,"\tprogram_packet_sequence_counter: %d\n", pes->program_packet_sequence_counter);
				PREFIX_PRINT;fprintf(stdout,"\tMPEG1_MPEG2_identifier: %d\n", pes->MPEG1_MPEG2_identifier);
				PREFIX_PRINT;fprintf(stdout,"\toriginal_stuff_length: %d\n", pes->original_stuff_length);
			}
			if (pes->PES_extension_flag_2) {
				PREFIX_PRINT;fprintf(stdout,"\tPES_extension_field_length: %d\n", pes->PES_extension_field_length);
			}
		}
		PREFIX_PRINT;fprintf(stdout, "\tPES_header_data_length: %d\n", pes->PES_header_data_length);

	} else if (pes->stream_id == PROGRAM_STREAM_MAP || pes->stream_id == PRIVATE_STRAME_2 ||
				pes->stream_id == ECM_STREAM || pes->stream_id == EMM_STREAM ||
				pes->stream_id == PROGRAM_STREAM_DIRECTORY || pes->stream_id == DSMCC_STREAM ||
				pes->stream_id == ITU_T_H222_1_E) {
		
	} else if (pes->stream_id == PADDING_STREAM) {
		PREFIX_PRINT;fprintf(stdout,"\tAll data is padding data.\n");
	}
	if (pes->data_len > 0) {
		PREFIX_PRINT;fprintf(stdout, "\tPES data(%d):\n", pes->data_len);
		for ( i = 0; i < pes->data_len; i+=16) {
			PREFIX_PRINT;fprintf(stdout, "\t\t");
			for (j = 0; j < 16 && (i+j < pes->data_len); j++ ) {
				fprintf(stdout, "%02x ", pes->data[j+i]);
			}
			fprintf(stdout, "\n");
		}
	}
}

void TS_packet_print(TS_packet_t *packet, char *print_prefix)
{
    PREFIX_PRINT;fprintf(stdout, "sync_byte                      0x%0x\n", packet->sync_byte); 
    PREFIX_PRINT;fprintf(stdout, "transport_error_indicator      %d\n",  packet->transport_error_indicator); 
    PREFIX_PRINT;fprintf(stdout, "payload_unit_start_indicator   %d\n",  packet->payload_unit_start_indicator); 
    PREFIX_PRINT;fprintf(stdout, "transport_priority             %d\n",  packet->transport_priority); 
    PREFIX_PRINT;fprintf(stdout, "pid                            %d\n",  packet->pid); 
    PREFIX_PRINT;fprintf(stdout, "transport_scrambling_control   0x%0x\n",  packet->transport_scrambling_control); 
    PREFIX_PRINT;fprintf(stdout, "adaption_field_control         0x%0x\n",  packet->adaption_field_control); 
    PREFIX_PRINT;fprintf(stdout, "continuity_counter             %d\n",  packet->continuity_counter);
    return;
}
