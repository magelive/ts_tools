#ifndef TS_PES_H
#define TS_PES_H

#include "ts_header.h"

#define IS_AUDIO_STREAM(stream_id) (((stream_id) >> 5) == 0b110)
#define IS_VIDEO_STREAM(stream_id) (((stream_id) >> 4) == 0b1110)

typedef enum stream_id {
    PROGRAM_STREAM_MAP      = 0b10111100,
    PRIVATE_STREAM_1        = 0b10111101,
    PADDING_STREAM          = 0b10111110,
    PRIVATE_STRAME_2        = 0b10111111,

    AUDIO_STREAM            = 0b11000000,
    VIDEO_STREAM            = 0b11100000,

    ECM_STREAM              = 0b11110000,
    EMM_STREAM              = 0b11110001,
    DSMCC_STREAM            = 0b11110010,
    ISO_IEC_13522_STREAM    = 0b11110011,

	ITU_T_H222_1_A          = 0b11110100,
	ITU_T_H222_1_B          = 0b11110101,
	ITU_T_H222_1_C          = 0b11110110,
	ITU_T_H222_1_D          = 0b11110111,
	ITU_T_H222_1_E          = 0b11111000,

	ancillary_stream        = 0b11111001,
	
	ISO_IEC_14496_1_SL      = 0b11111010,
	ISO_IEC_14496_1_FLEXMUX = 0b11111011,
	
	PROGRAM_STREAM_DIRECTORY = 0b11111111,

}stream_id_t;

typedef enum {
	FAST_FORWARD            = 0b000,
	SLOW_MOTION             = 0b001,
	FREEZE_FRAME            = 0b010,
	FAST_REVERSE            = 0b011,
	SLOW_REVERSE            = 0b100,
}trick_mode_control_t;

typedef struct TS_PES_packet {
	uint32_t packet_start_code_pprefix              :24;
	uint32_t stream_id                              :8;
	uint32_t PES_packet_length                      :16;

	/*
		stream_id != program_stream_map &&
		stream_id != padding_stream &&
		stream_id != private_stream_2 &&
		stream_id != ECM && stream_id != EMM &&
		stream_id != program_stream_directory &&
		stream_id != DSMCC_stream &&
		stream_id !=  ITU-T Rec. H.222.1 type E stream
	*/
	uint32_t fix									:2; //fix '0b10'
	uint32_t PES_scrambling_control					:2;
	uint32_t PES_priority                           :1;
	uint32_t data_alignment_indicator               :1;
	uint32_t copyright                              :1;
	uint32_t original_or_copy                       :1;
	uint32_t PTS_DTS_flags                          :2;
	uint32_t ESCR_flag                              :1;
	uint32_t ES_rate_flag                           :1;
	uint32_t DSM_trick_mode_flag                    :1;
	uint32_t additional_copy_info_flag              :1;
	uint32_t PES_CRC_flag                           :1;
	uint32_t PES_extension_flag                     :1;
	uint32_t PES_header_data_length                 :8;
	
	uint32_t PTS                                    :32;
	uint32_t DTS                                    :32;

	//ESCR_flag == 1
	uint32_t ESCR_base                              :32;
	uint32_t ESCR_extension                         :9;
	
	//ES_rate_flag == 1
	uint32_t ES_rate                                :22;
	
	// DSM_trick_mode_flag == 1;
	uint32_t trick_mode_control                      :3;
	uint32_t trick_data                              :5;

	//additional_copy_info_flag == 1
	uint32_t additional_copy_info                    :7;

	//PES_CRC_flag == 1
	uint32_t previous_PES_packet_CRC                 :16;

	//PES_extension_flag == 1
	uint32_t PES_private_data_flag                  :1;
	uint32_t pack_header_field_flag                 :1;
	uint32_t program_packet_sequence_counter_flag   :1;
	uint32_t PSTD_buffer_flag                       :1;
	uint32_t PES_extension_flag_2                   :1;
	
	//PES_private_data_flag == 1
	uint8_t PES_private_data[16];
	
	//pack_header_field_flag == '1'
	uint32_t pack_field_length                      :8;
	uint8_t *pack_header;
	
	//program_packet_sequence_counter_flag == '1'                                                                               
	uint32_t program_packet_sequence_counter        :7;
	uint32_t MPEG1_MPEG2_identifier                 :1;
	uint32_t original_stuff_length                  :6;
	//PSTD_buffer_flag == '1'
	uint32_t PSTD_buffer_scale                      :1; 
	uint32_t PSTD_buffer_size                       :13;
	//PES_extension_flag_2 == '1'
	uint32_t PES_extension_field_length             :7;

	uint32_t data_len;
	uint8_t *data;
}TS_PES_t;

void TS_PES_print(TS_PES_t *pes, char *print_prefix);

int TS_PES_parse(TS_packet_t *packet, int offset, TS_PES_t *pes);

#endif