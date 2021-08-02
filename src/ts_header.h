#ifndef TS_HEADER_H
#define TS_HEADER_H

#include <stdint.h>

#define TS_HEADER_SYNC	0x47
#define TS_HEADER_LEN 4
#define TS_PACKET_MAX_SIZE 204
#define TS_PACKET_DEFAULT_SIZE 188

typedef  struct TS_packet
{
	uint32_t sync_byte                    	 :8;      
	uint32_t transport_error_indicator       :1; 
	uint32_t payload_unit_start_indicator    :1;
	uint32_t transport_priority              :1;
	uint32_t pid                          	 :13;
	uint32_t transport_scrambling_control    :2;
	uint32_t adaption_field_control          :2; 
	uint32_t continuity_counter              :4;
	uint32_t length;
	uint8_t *data;
} TS_packet_t;

typedef struct TS_adaptation {
	uint32_t adaptation_filed_length                :8;
	uint32_t discontinuity_indicator                :1;
	uint32_t random_access_indicator                :1;
	uint32_t elementary_stream_priority_indicator   :1;
	uint32_t PCR_flag                               :1;
	uint32_t OPCR_flag                              :1;
	uint32_t splicing_point_flag                    :1;
	uint32_t transport_private_data_flag            :1;
	uint32_t adaptation_field_extension_flag        :1;

	//PCR_flag == 1
	uint64_t program_clock_reference_base           :33;
	uint32_t pcr_reserved                           :6;
	uint32_t program_clock_reference_extension      :9;

	//OPCR_flag == 1
	uint64_t original_program_clock_reference_base      :33;
	uint64_t opcr_reserved                              :6;
	uint32_t original_program_clock_reference_extension :9;

	//splicing_point_flag == 1
	uint32_t splice_countdown                           :8;

	//transport_private_data_flag == 1
	uint32_t transport_private_data_length              :8;
	uint8_t *transport_private_data;
	
	// adaptation_field_extension_flag == 1
	uint32_t adaptation_field_extension_length          :8;
	uint32_t ltw_flag                                   :1;
	uint32_t piecewise_rate_flag                        :1;
    uint32_t seamless_splice_flag                       :1;
	uint32_t extension_reserved                         :5;

	//ltw_flag == 1
	uint32_t ltw_valid_flag                             :1;
	uint32_t ltw_offset								    :5;

	//piecewise_rate_flag == 1
	uint32_t piecewise_rate_reserved                    :2;
	uint32_t piecewise_rate                             :22;
	
	//seamless_splice_flag == 1
	uint32_t splice_type                                :4;
	uint32_t marker_bit[3];
	uint32_t DTS_next_AU;

	int adaptation_field_extension_reserved_len;
	uint8_t *adaptation_field_extension_reserved;

	uint32_t reserved_len;
	uint8_t *reserved;
}TS_adaptation_t;

typedef enum TS_PID{
	TS_PID_PAT = 0x00,
	TS_PID_CAT = 0x01,
	TS_PID_DESCRIPTION_TABLE = 0x02,
}TS_pid_t;

typedef struct TS_PAT_program
{
	unsigned program_number    :16;   //节目号
	unsigned program_map_PID   :13;   //节目映射表的PID，节目号大于0时对应的PID，每个节目对应一个
	struct TS_PAT_program *next;
}TS_PAT_program_t;

typedef struct TS_PAT
{
	uint32_t table_id                        : 8;  //fix: 0x00
	uint32_t section_syntax_indicator        : 1;  //fix: 1
	uint32_t zero                            : 1;  //0
	uint32_t reserved_1                      : 2;  
	uint32_t section_length                  : 12;
	uint32_t transport_stream_id             : 16; 
	uint32_t reserved_2                      : 2;  
	uint32_t version_number                  : 5;  
	uint32_t current_next_indicator          : 1;  
	uint32_t section_number                  : 8;  
	uint32_t last_section_number             : 8;  
	
	TS_PAT_program_t *programs;
	uint32_t crc32                           : 32;

	uint8_t *section_data;
	uint32_t section_index;
	int section_free_flag;
} TS_PAT_t;

typedef struct TS_CAT {
	uint32_t table_id                        : 8;  //fix: 0x00
	uint32_t section_syntax_indicator        : 1;  //fix: 1
	uint32_t zero                            : 1;  //0
	uint32_t reserved_1                      : 2;  
	uint32_t section_length                  : 12;
	uint32_t reserved_2                      : 18;  
	uint32_t version_number                  : 5;  
	uint32_t current_next_indicator          : 1;  
	uint32_t section_number                  : 8;  
	uint32_t last_section_number             : 8;
	uint32_t descriptor_len;
	uint8_t *descriptor;
	uint32_t crc32							 :32;

	uint8_t *section_data;
	uint32_t section_index;
}TS_CAT_t;

typedef struct TS_PMT_stream{
	uint32_t stream_type                     : 8;
	uint32_t reserved_5                      : 3;
	uint32_t elementary_PID                  : 13;
	uint32_t reserved_6                      : 4;
	uint32_t ES_info_length                  : 12;
	uint8_t *ES_info;
	struct TS_PMT_stream *next;
}TS_PMT_stream_t;

typedef struct TS_PMT
{
  	uint32_t table_id                        : 8;
   	uint32_t section_syntax_indicator        : 1;
	uint32_t zero                            : 1;
	uint32_t reserved_1                      : 2;
	uint32_t section_length                  : 12;
  	uint32_t program_number                  : 16;
 	uint32_t reserved_2                      : 2;
   	uint32_t version_number                  : 5;
   	uint32_t current_next_indicator          : 1;
   	uint32_t section_number                  : 8;
  	uint32_t last_section_number             : 8;
  	uint32_t reserved_3                      : 3;
 	uint32_t PCR_PID                         : 13;
  	uint32_t reserved_4                      : 4;
  	uint32_t program_info_length             : 12;
	uint8_t *program_info;
	TS_PMT_stream_t *streams;
	uint32_t crc32                           : 32;

	uint8_t *section_data;
	uint32_t section_index;
	int section_free_flag;

} TS_PMT_t;

int TS_header_parse(uint8_t *ts_data, int packet_size, TS_packet_t *packet);
int TS_adaptation_parse(TS_packet_t *packet, int adaptation_offset, TS_adaptation_t *adaptation);

int TS_check_is_PMT(TS_PAT_t *pat, int pid);
int TS_PAT_get_network_PID(TS_PAT_t *pat);
void TS_PAT_free(TS_PAT_t *pat);
int TS_PAT_parse(TS_packet_t *packet, int offset, TS_PAT_t *pat);
int TS_CAT_parse(TS_packet_t *packet, int offset, TS_CAT_t *cat);
int TS_check_is_PMT_stream(TS_PMT_t *pmt, int pid);
void TS_PMT_free(TS_PMT_t *pmt);
int TS_PMT_parse(TS_packet_t *packet, int offset, TS_PMT_t *pmt);

#endif