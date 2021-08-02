#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "ts_header.h"
#include "ts_pes.h"
#include "ts_out.h"
#include <getopt.h>

void usage(char *name)
{
	fprintf(stdout, "Usage: %s ACTION [PARAMETERS]... file1 file2 ...\n\n", name);
	fprintf(stdout, "For each TS file specified, perform the specified ACTION.\n");
	fprintf(stdout, "An action must be specified and only once.\n");
	fprintf(stdout, "Some options are not applicable to some actions.\n\n");
	fprintf(stdout, "ACTIONS:\n");
	fprintf(stdout, "\t-r | --resume\t\t\tShort Description(default option)\n");
	fprintf(stdout, "\t-m | --more\t\t\tDetailed description\n");
	fprintf(stdout, "\t-e | --extract <stream_id>\tExtract the stream data\n");
	fprintf(stdout, "\t-a | --extrack-all\t\tExtract the all stream data\n");
	fprintf(stdout, "\t-h | --help\t\t\tShow the help information\n");
}

int check_packet_size(uint8_t *ts_data, size_t length)
{
	size_t i, offset;
	int packet_size = 0, mini_packets = 10;
	if (length < TS_PACKET_MAX_SIZE*mini_packets) {
		mini_packets = length/TS_PACKET_MAX_SIZE;
	}

	for (offset = 0; offset < TS_PACKET_MAX_SIZE; offset ++) {
		if (ts_data[offset] != TS_HEADER_SYNC) {
			continue;
		}
		
		/* try 188 */
		for (i = 0; i < mini_packets; i++) {
			if (ts_data[offset+i*TS_PACKET_DEFAULT_SIZE] !=  TS_HEADER_SYNC) {
				break;
			}
		}
		if (i == mini_packets) {
			packet_size = TS_PACKET_DEFAULT_SIZE;
			break;
		}

		/* try 204 */
		for (i = 0; i < mini_packets; i++) {
			if (ts_data[offset+i*TS_PACKET_MAX_SIZE] !=  TS_HEADER_SYNC) {
				break;
			}
		}
		if (i == mini_packets) {
			packet_size = TS_PACKET_MAX_SIZE;
			break;
		}
	}
	if (offset == TS_PACKET_MAX_SIZE) {
		return 0;
	}
	return offset * 256 + packet_size;
}

int file_resume_information(char *file) 
{
	int i, j, k;
	struct stat statbuf = {0x00};
	if (stat(file, &statbuf) < 0) {
		fprintf(stderr, "file %s error: %s\n", file, strerror(errno));
		return 0;		
	}
	size_t ts_len = statbuf.st_size;
	uint8_t *ts_data = malloc(ts_len);
	if (!ts_data) {
		fprintf(stderr, "OOM length %ld.\n", ts_len);
		return 0;
	}

	fprintf(stdout, "file base info\n");
	fprintf(stdout, "-------------------------\n");
	fprintf(stdout, "\tfile name: %s\n", file);
	fprintf(stdout, "\tfile size: %ld\n", statbuf.st_size);
	fprintf(stdout, "\n\n");

	FILE *fp = fopen(file, "r");
	if (!fp) {
		fprintf(stderr, "open %s file error: %s\n", file, strerror(errno));
		return 0;
	}

	fread(ts_data, 1, ts_len, fp);
	int offset_and_packet_size = check_packet_size(ts_data, ts_len);
	int packet_size = offset_and_packet_size % 256;
	int offset = offset_and_packet_size / 256;
	fprintf(stdout, "packet size and offset:\n");
	fprintf(stdout, "-------------------------\n");
	fprintf(stdout, "\tpacket size: %d\n\tTS offset: %d\n", packet_size, offset);
	fprintf(stdout, "\tpacket number: %ld\n", (statbuf.st_size-offset)/packet_size);
	fprintf(stdout, "\n");

	int packet_nr = (statbuf.st_size-offset)/packet_size, result = 0, poffset = 0; // ts_len/packet_size;
	TS_packet_t packet = {0x00};
	TS_adaptation_t adaptation = {0x00};
	TS_PAT_t pat = {0x00};
	TS_CAT_t cat = {0x00};
	TS_PMT_t pmt = {0x00};

	typedef struct pid_info {
		int pid;
		int count;
		char description[128];
		struct pid_info *next;
	}pid_t;

	pid_t *pid_header = NULL, *pid_n = NULL, *pid_p = NULL;

	#define pid_find(PID)  \
		do {  \
			pid_p = pid_n = pid_header;  \
			while(pid_n) {   \								
				if (pid_n->pid == PID) {  \
					break;  \
				} \
				pid_p = pid_n; \
				pid_n = pid_n->next; \
			} \
		}while(0)

	#define pid_append(PID, describe) \
			do { \ 
				pid_find(PID); \
				if (!pid_n) { \
					pid_t *p = malloc(sizeof(pid_t)); \
					if (!p) { \
						fprintf(stdout, "OOM at pid append."); \
						exit(0); \
					} \
					p->pid = PID; \
					p->count = 1; \
					int l = strlen(describe); \
					l = (sizeof(p->description) > l)? l: sizeof(p->description) - 1; \
					strncpy(p->description, describe, l); \
					p->description[l] = '\0'; \
					p->next = NULL; \
					if (!pid_header) { \
						pid_n = pid_p = pid_header = p; \
					} else {  \
						pid_p->next = p; \
						pid_n = p; \
					}	\				
				} else { \
					pid_n->count ++; \
				} \
			} while(0)

	for (i = 0; i < packet_nr; i++) {		
		poffset = 0;
		bzero(&packet, sizeof(packet));
		bzero(&adaptation, sizeof(adaptation));
		result = TS_header_parse(ts_data + offset + i*packet_size, packet_size, &packet);
		if (result < 0 || packet.adaption_field_control == 0x00) {
        	//invalid packet
        	continue;
    	}

		//has adaptation
		if (packet.adaption_field_control == 0b10 ||
			packet.adaption_field_control == 0b11) {
			poffset += TS_adaptation_parse(&packet, poffset, &adaptation);
		}

		if (poffset == packet.length) {
			continue;
		}

		if (packet.payload_unit_start_indicator == 1) {
			//has PSI or PES header	
			int pointer_field;
			if (packet.pid == TS_PID_PAT) {
				pointer_field = packet.data[poffset];
				poffset += 1;
				poffset += TS_PAT_parse(&packet, poffset, &pat);
				pid_append(packet.pid, "PAT\0");
			} else if (packet.pid == TS_PID_CAT) {
				//CAT
				pointer_field = packet.data[poffset];
				poffset += 1;
				poffset += TS_CAT_parse(&packet, poffset, &cat);
				pid_append(packet.pid, "CAT\0");
			} else if (TS_check_is_PMT(&pat, packet.pid)) {
				//PMT
				pointer_field = packet.data[poffset];
				poffset += 1;
				bzero(&pmt, sizeof(pmt));
				poffset += TS_PMT_parse(&packet, poffset, &pmt);
				pid_append(packet.pid, "PMT\0");
			} else if (packet.pid != -1 && packet.pid == TS_PAT_get_network_PID(&pat)) {
				//Network PID
			} else if (TS_check_is_PMT_stream(&pmt, packet.pid)) {
				TS_PMT_stream_t *tmp = pmt.streams;
				while(tmp != NULL) {
					if (packet.pid == tmp->elementary_PID) {
						break;
					}
					tmp = tmp->next;
				}
				pid_append(packet.pid, get_stream_description(tmp->stream_type));
			}
		} else {
			if (packet.pid == TS_PID_PAT) {
				pid_append(packet.pid, "PAT\0");
			} else if (packet.pid == TS_PID_CAT) {
				pid_append(packet.pid, "CAT\0");
			} else if (TS_check_is_PMT(&pat, packet.pid)) {
				pid_append(packet.pid, "PMT\0");
			} else if (packet.pid != -1 && packet.pid == TS_PAT_get_network_PID(&pat)) {
				//Network PID
			} else if (TS_check_is_PMT_stream(&pmt, packet.pid)) {
				TS_PMT_stream_t *tmp = pmt.streams;
				while(tmp != NULL) {
					if (packet.pid == tmp->elementary_PID) {
						break;
					}
					tmp = tmp->next;
				}
				pid_append(packet.pid, get_stream_description(tmp->stream_type));
			}
		}
	}

	fprintf(stdout, "pid info\n");
	fprintf(stdout, "-------------------\n");
	pid_n = pid_header; 
	while(pid_n) { 								
		fprintf(stdout, "\tpid 0x%04x(%4d), %8d packets, %5.2f% ==> %s\n", pid_n->pid, pid_n->pid, 
			pid_n->count, pid_n->count * 100 / (float)packet_nr , pid_n->description);
		pid_n = pid_n->next; 
	}
	fprintf(stdout, "\n=================================================\n\n");
}

int file_more_infomation(char *file)
{
	int i, j, k;
	struct stat statbuf = {0x00};
	if (stat(file, &statbuf) < 0) {
		fprintf(stderr, "file %s error: %s\n", file, strerror(errno));
		return 0;		
	}
	size_t ts_len = statbuf.st_size;
	uint8_t *ts_data = malloc(ts_len);
	if (!ts_data) {
		fprintf(stderr, "OOM length %ld.\n", ts_len);
		return 0;
	}

	fprintf(stdout, "file base info\n");
	fprintf(stdout, "-------------------------\n");
	fprintf(stdout, "\tfile name: %s\n", file);
	fprintf(stdout, "\tfile size: %ld\n", statbuf.st_size);
	fprintf(stdout, "\n\n");

	FILE *fp = fopen(file, "r");
	if (!fp) {
		fprintf(stderr, "open %s file error: %s\n", file, strerror(errno));
		return 0;
	}

	fread(ts_data, 1, ts_len, fp);
	int offset_and_packet_size = check_packet_size(ts_data, ts_len);
	int packet_size = offset_and_packet_size % 256;
	int offset = offset_and_packet_size / 256;
	fprintf(stdout, "packet size and offset:\n");
	fprintf(stdout, "-------------------------\n");
	fprintf(stdout, "\tpacket size: %d\n\tTS offset: %d\n", packet_size, offset);
	fprintf(stdout, "\tpacket number: %ld\n", (statbuf.st_size-offset)/packet_size);
	fprintf(stdout, "\n");

	int packet_nr = (statbuf.st_size-offset)/packet_size, result = 0, poffset = 0; // ts_len/packet_size;
	TS_packet_t packet = {0x00};
	TS_adaptation_t adaptation = {0x00};
	TS_PAT_t pat = {0x00};
	TS_CAT_t cat = {0x00};
	TS_PMT_t pmt = {0x00};
	fprintf(stdout, "TS packet infamation: \n");
	fprintf(stdout, "-----------------------------------\n");
	for (i = 0; i < packet_nr; i++) {
		poffset = 0;
		bzero(&packet, sizeof(packet));
		bzero(&adaptation, sizeof(adaptation));
		result = TS_header_parse(ts_data + offset + i*packet_size, packet_size, &packet);
		fprintf(stdout, "\t packet index: %d\n", i);
		TS_packet_print(&packet, "\t\t");
		fprintf(stdout, "\n");

		if (packet.adaption_field_control == 0x00) {
        	//invalid packet
        	continue;
    	}

		//has adaptation
		if (packet.adaption_field_control == 0b10 ||
			packet.adaption_field_control == 0b11) {
			poffset += TS_adaptation_parse(&packet, poffset, &adaptation);
			TS_adaptation_print(&adaptation, "\t\t");
		}

		if (poffset == packet.length) {
			continue;
		}

		fprintf(stdout, "\t\tPayload(start at %d, length %d):\n", poffset, packet.length);
		for (j = 0; (poffset+j) < packet.length; j+=16) {
			fprintf(stdout, "\t\t\t");
			for (k = 0; k < 16 && ((poffset+j+k) < packet.length); k++) {
				fprintf(stdout, "0x%02x ", packet.data[poffset+j+k]);
			}
			fprintf(stdout, "\n");
		}
		fprintf(stdout, "\n");

		if (packet.payload_unit_start_indicator == 1) {
			//has PSI or PES header
			int pointer_field;
			if (packet.pid == TS_PID_PAT) {
				pointer_field = packet.data[poffset];
				poffset += 1;
				poffset += TS_PAT_parse(&packet, poffset, &pat);
				TS_PAT_print(&pat, "\t\t");
			} else if (packet.pid == TS_PID_CAT) {
				//CAT
				pointer_field = packet.data[poffset];
				poffset += 1;
				poffset += TS_CAT_parse(&packet, poffset, &cat);
				TS_CAT_print(&cat, "\t\t");
			} else if (TS_check_is_PMT(&pat, packet.pid)) {
				//PMT
				pointer_field = packet.data[poffset];
				poffset += 1;
				bzero(&pmt, sizeof(pmt));
				poffset += TS_PMT_parse(&packet, poffset, &pmt);
				TS_PMT_print(&pmt, "\t\t");
			} else if (packet.pid != -1 && packet.pid == TS_PAT_get_network_PID(&pat)) {
				//Network PID
			} else if (TS_check_is_PMT_stream(&pmt, packet.pid)){
				//PES header
				TS_PES_t pes = {0x00};
				result = TS_PES_parse(&packet, poffset, &pes);
				if (result == -1) { 
					fprintf(stderr, "\t\tThis packet isn't PES.\n");
					continue;
				}
				TS_PES_print(&pes, "\t\t");
			}
		} else {
			//Payload data
			if (packet.pid == TS_PID_PAT) {
			} else if (packet.pid == TS_PID_CAT) {
			}
			if (TS_check_is_PMT_stream(&pmt, packet.pid)) {
				//PES data
			}
		}
		fprintf(stdout, "\n\n");
	}
	fclose(fp);

	return 0;	
}

typedef struct frame {
	int index;
	int data_size;
	int data_len;
	unsigned char *data;
	struct frame *next;
}frame_t;

#define frame_malloc(P) \
	do { \
		(P) = calloc(1, sizeof(frame_t)); \
		if (!(P)) {fprintf(stderr, "OOM exit.\n"); exit(0);} \
		(P)->data_size = 1024*1024*4; \
		(P)->data = calloc(1, (P)->data_size); \
		if (!(P)->data) {fprintf(stderr, "OOM exit.\n"); exit(0);} \
	}while(0)
	
#define frame_copy(P, S, L) \
	do {\
		while (((L) + (P)->data_len) > (P)->data_size) {\
			(P)->data_size *= 2; \
			(P)->data = realloc((P)->data, (P)->data_size); \
			if (!(P)->data) {fprintf(stderr, "OOM exit.\n"); exit(0);} \				
		} \
		memcpy((P)->data + (P)->data_len, (S), (L)); \
		(P)->data_len += (L); \
	}while(0)

#define frame_free(P) \
	do { \
		free((P)->data); \
		free(P); \
		(P) = NULL; \
	}while(0)

int extract_all(char *file)
{
	int i, j, k;
	struct stat statbuf = {0x00};
	if (stat(file, &statbuf) < 0) {
		fprintf(stderr, "file %s error: %s\n", file, strerror(errno));
		return 0;		
	}
	size_t ts_len = statbuf.st_size;
	uint8_t *ts_data = malloc(ts_len);
	if (!ts_data) {
		fprintf(stderr, "OOM length %ld.\n", ts_len);
		return 0;
	}

	FILE *fp = fopen(file, "r");
	if (!fp) {
		fprintf(stderr, "open %s file error: %s\n", file, strerror(errno));
		return 0;
	}

	fread(ts_data, 1, ts_len, fp);
	int offset_and_packet_size = check_packet_size(ts_data, ts_len);
	int packet_size = offset_and_packet_size % 256;
	int offset = offset_and_packet_size / 256;
	int packet_nr = (statbuf.st_size-offset)/packet_size, result = 0, poffset = 0; // ts_len/packet_size;

	TS_packet_t packet = {0x00};
	TS_adaptation_t adaptation = {0x00};

	TS_PAT_t pat = {0x00};
	TS_CAT_t cat = {0x00};
	TS_PMT_t pmt = {0x00};

	typedef struct pid_frame {
		int pid;
		int count;
		frame_t *frame_head;
		frame_t *frame_end;
		char description[128];
		struct pid_frame *next;
	}pid_frame_t;

	pid_frame_t *header = NULL, *pid_n, *pid_p;

	#define pid_frame_find(PID) \
		do { \
			pid_p = pid_n = header; \
			while(pid_n != NULL) { \
				if (pid_n->pid == (PID)) { \
					break; \
				} \
				pid_p = pid_n; \
				pid_n = pid_n->next; \
			} \
		}while(0)

	#define pid_frame_append_pid(PID, describe) \
		do { \
			pid_frame_find(PID); \
			if (!pid_n) { \
				pid_frame_t *p = malloc(sizeof(pid_frame_t)); \
				if (!p) { \
					fprintf(stdout, "OOM at pid_frame append."); \
					exit(0); \
				} \
				p->pid = PID; \
				p->count = 0; \
				int l = strlen(describe); \
				l = (sizeof(p->description) > l)? l: sizeof(p->description) - 1; \
				strncpy(p->description, describe, l); \
				p->description[l] = '\0'; \
				p->next = NULL; \
				if (!header) { \
					pid_n = pid_p = header = p; \
				} else {  \
					pid_p->next = p; \
					pid_n = p; \
				}	\				
			} \
		}while(0)

	#define pid_frame_append(P, FD, FL, F) \
		do { \
			frame_t *f = NULL; \
			if (F) { \
				frame_malloc(f); \
				if (!(P)->frame_head) { \
					(P)->frame_head = (P)->frame_end = f; \
				} else { \
					(P)->frame_end->next = f; \
					(P)->frame_end = f; \
				} \
				(P)->count ++; \
			} else { \
				f = (P)->frame_end; \
			}\
			frame_copy(f, FD, FL); \			
		}while (0);
		

	for (i = 0; i < packet_nr; i++) {
		poffset = 0;
		bzero(&packet, sizeof(packet));
		bzero(&adaptation, sizeof(adaptation));
		result = TS_header_parse(ts_data + offset + i*packet_size, packet_size, &packet);
		if (packet.adaption_field_control == 0x00) {
        	//invalid packet
        	continue;
    	}

		//has adaptation
		if (packet.adaption_field_control == 0b10 ||
			packet.adaption_field_control == 0b11) {
			poffset += TS_adaptation_parse(&packet, poffset, &adaptation);
		}

		if (poffset == packet.length) {
			continue;
		}

		if (packet.payload_unit_start_indicator == 1) {
			//has PSI or PES header
			int pointer_field;
			if (packet.pid == TS_PID_PAT) {
				pointer_field = packet.data[poffset];
				poffset += 1;
				poffset += TS_PAT_parse(&packet, poffset, &pat);
			} else if (packet.pid == TS_PID_CAT) {
				//CAT
				pointer_field = packet.data[poffset];
				poffset += 1;
				poffset += TS_CAT_parse(&packet, poffset, &cat);
			} else if (TS_check_is_PMT(&pat, packet.pid)) {
				//PMT
				pointer_field = packet.data[poffset];
				poffset += 1;
				bzero(&pmt, sizeof(pmt));
				poffset += TS_PMT_parse(&packet, poffset, &pmt);
			} else if (TS_check_is_PMT_stream(&pmt, packet.pid)) {
				
				TS_PMT_stream_t *tmp = pmt.streams;
				while(tmp != NULL) {
					if (packet.pid == tmp->elementary_PID) {
						break;
					}
					tmp = tmp->next;
				}
				pid_frame_append_pid(packet.pid, get_stream_description(tmp->stream_type));
				//PES header
				TS_PES_t pes = {0x00};
				result = TS_PES_parse(&packet, poffset, &pes);
				if (result == -1) { 
					fprintf(stderr, "\t\tThis packet isn't PES.\n");
					continue;
				}
				pid_frame_append(pid_n, pes.data, pes.data_len, 1);
			}
		} else {
			if (TS_check_is_PMT_stream(&pmt, packet.pid)) {
				//PES data
				pid_frame_find(packet.pid);				
				pid_frame_append(pid_n, packet.data + offset, packet.length - offset, 0);	
			}
		}
	}

	fprintf(stdout, "All stream:\n\n");
	frame_t *f_tmp;
	pid_n = header;
	int index;
	while(pid_n != NULL) {
		fprintf(stdout, "\tstream id: %d\n", pid_n->pid);
		fprintf(stdout, "\tdescription: %s\n", pid_n->description);
		fprintf(stdout, "\tframe count: %d\n", pid_n->count);
		fprintf(stdout, "\t-------------------------------\n\n");
		f_tmp = pid_n->frame_head;
		index = 0;
		while(f_tmp) {
			fprintf(stdout, "\t\tframe %d\n", ++index);
			fprintf(stdout, "\t\tdata(%d):\n", f_tmp->data_len);

			for(i = 0; i < f_tmp->data_len; i+=16) {
				fprintf(stdout, "\t\t\t");
				for(j = 0; j < 16 && (j+i) < f_tmp->data_len; j++) {
					fprintf(stdout, "0x%02x ", f_tmp->data[i+j]);
				}
				fprintf(stdout, "\n");
			}
			fprintf(stdout, "\n");
			f_tmp = f_tmp->next;
		}

		pid_n = pid_n->next;
	}


	fclose(fp);

	return 0;	
}

int extract_pid(char *file, int pid)
{
	int i, j, k;
	struct stat statbuf = {0x00};
	if (stat(file, &statbuf) < 0) {
		fprintf(stderr, "file %s error: %s\n", file, strerror(errno));
		return 0;		
	}
	size_t ts_len = statbuf.st_size;
	uint8_t *ts_data = malloc(ts_len);
	if (!ts_data) {
		fprintf(stderr, "OOM length %ld.\n", ts_len);
		return 0;
	}

	FILE *fp = fopen(file, "r");
	if (!fp) {
		fprintf(stderr, "open %s file error: %s\n", file, strerror(errno));
		return 0;
	}

	fread(ts_data, 1, ts_len, fp);
	int offset_and_packet_size = check_packet_size(ts_data, ts_len);
	int packet_size = offset_and_packet_size % 256;
	int offset = offset_and_packet_size / 256;
	int packet_nr = (statbuf.st_size-offset)/packet_size, result = 0, poffset = 0; // ts_len/packet_size;

	TS_packet_t packet = {0x00};
	TS_adaptation_t adaptation = {0x00};

	TS_PAT_t pat = {0x00};
	TS_CAT_t cat = {0x00};
	TS_PMT_t pmt = {0x00};
	frame_t *header = NULL, *end, *f_tmp;

	for (i = 0; i < packet_nr; i++) {
		poffset = 0;
		bzero(&packet, sizeof(packet));
		bzero(&adaptation, sizeof(adaptation));
		result = TS_header_parse(ts_data + offset + i*packet_size, packet_size, &packet);
		if (packet.adaption_field_control == 0x00) {
        	//invalid packet
        	continue;
    	}

		//has adaptation
		if (packet.adaption_field_control == 0b10 ||
			packet.adaption_field_control == 0b11) {
			poffset += TS_adaptation_parse(&packet, poffset, &adaptation);
		}

		if (poffset == packet.length) {
			continue;
		}

		if (packet.pid != pid) {
			continue;
		}

		if (packet.payload_unit_start_indicator == 1) {
			//has PSI or PES header
			TS_PES_t pes = {0x00};
			result = TS_PES_parse(&packet, poffset, &pes);
			if (result == -1) { 
				fprintf(stderr, "\t\tThis packet isn't PES.\n");
				continue;
			}
			frame_malloc(f_tmp);
			frame_copy(f_tmp, pes.data, pes.data_len);
			if (!header) {
				end = header = f_tmp;
			} else {
				end->next = f_tmp;
				end = f_tmp;
			}
		} else {		
			frame_copy(end, packet.data + offset, packet.length - offset);
		}
	}

	fprintf(stdout, "stream %d:\n\n", pid);
	f_tmp = header;
	int index;
	while(f_tmp) {
		fprintf(stdout, "\tframe %d\n", ++index);
		fprintf(stdout, "\tdata(%d):\n", f_tmp->data_len);

		for(i = 0; i < f_tmp->data_len; i+=16) {
			fprintf(stdout, "\t\t");
			for(j = 0; j < 16 && (j+i) < f_tmp->data_len; j++) {
				fprintf(stdout, "0x%02x ", f_tmp->data[i+j]);
			}
			fprintf(stdout, "\n");
		}
		fprintf(stdout, "\n");
		f_tmp = f_tmp->next;
	}
	fclose(fp);

	return 0;	
}

typedef enum option_cmd {
	CMD_UNKNOW,
	CMD_RESUME,
	CMD_MORE,
	CMD_EXTRACK,
	CMD_EXTRACK_ALL
}option_cmd_t;

int main(int argc, char *argv[])
{
	int c;
	int digit_optind = 0;
	int option_index = 0;
	option_cmd_t cmd = CMD_UNKNOW;
	int stream_id = -1;

	for (;;) {		
		static const struct option long_options [] = {
			{ "resume",       no_argument,       NULL, 'r'},
			{ "more",         no_argument,       NULL, 'm' },
			{ "extract",      required_argument, NULL, 'e' },
			{ "extrack-all",  no_argument,       NULL, 'a' },
			{ "help",         no_argument,       NULL, 'h' },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "rme:ah", long_options, &option_index);

		if (c == -1) {
			break;
		}
#define OPT_ERR_OUT do{fprintf(stderr, "Using the wrong ACTION option parameters.\n\n"); usage(argv[0]); return 0;}while(0)
		switch (c) {
		case 'r':
			if (cmd != CMD_UNKNOW) {
				OPT_ERR_OUT;
			} else {
				cmd = CMD_RESUME;
			}
			break;
		case 'm':
			if (cmd != CMD_UNKNOW) {
				OPT_ERR_OUT;
			} else {
				cmd = CMD_MORE;
			}
			break;
		case 'e':
			if (cmd != CMD_UNKNOW) {
				OPT_ERR_OUT;
			} else {
				cmd = CMD_EXTRACK;
				stream_id = atoi(optarg);
				//fprintf(stdout, "extract stream id %d\n", stream_id);
			}			
			break;
		case 'a':
			if (cmd != CMD_UNKNOW) {
				OPT_ERR_OUT;
			} else {
				cmd = CMD_EXTRACK_ALL;
			}
			break;
		case '?':
			printf("??? out: %s-%s\n", argv[option_index], optarg);
		default:
			usage(argv[0]);
			return 0;
			break;
		}
	}
	printf("argc = %d, optind = %d\n", argc, optind);
	if (optind >= argc) {
		usage(argv[0]);
		return 0;
	}
	if (cmd == CMD_UNKNOW) {
		cmd = CMD_RESUME;
	}

	while (optind < argc) {
		printf("%s \n", argv[optind]);
		switch(cmd) {
			case CMD_MORE:
				file_more_infomation(argv[optind]);
				break;
			case CMD_RESUME:
				file_resume_information(argv[optind]);
				break;
			case CMD_EXTRACK:
				extract_pid(argv[optind], stream_id);
				break;
			case CMD_EXTRACK_ALL:
				extract_all(argv[optind]);
				break;
			default:
				break;
		}
		optind ++;
  	}
	return 0;
}

#if 0

int TS_packet_feed(TS_t *ts, TS_packet_t *packet)
{
	if (packet.adaption_field_control == 0x00) {
		return -1;
	}

	TS_adaptation_t adaptation = {0x00};
	//has adaptation
	if (packet.adaption_field_control == 0b10 ||
		packet.adaption_field_control == 0b11) {
		poffset += TS_adaptation_parse(&packet, poffset, &adaptation);
	}

	if (poffset == packet.length) {
		continue;
	}

	fprintf(stdout, "\t\tPayload(start at %d, length %d):\n", poffset, packet.length);
	for (j = 0; (poffset+j) < packet.length; j+=16) {
		fprintf(stdout, "\t\t\t");
		for (k = 0; k < 16 && ((poffset+j+k) < packet.length); k++) {
			fprintf(stdout, "0x%02x ", packet.data[poffset+j+k]);
		}
		fprintf(stdout, "\n");
	}
	fprintf(stdout, "\n");

	if (packet.payload_unit_start_indicator == 1) {
		//has PSI or PES header
		int pointer_field;
		if (packet.pid == TS_PID_PAT) {
			pointer_field = packet.data[poffset];
			poffset += 1;
			poffset += TS_PAT_parse(&packet, poffset, &pat);
			TS_PAT_print(&pat, "\t\t");
		} else if (packet.pid == TS_PID_CAT) {
			//CAT
			pointer_field = packet.data[poffset];
			poffset += 1;
			poffset += TS_CAT_parse(&packet, poffset, &cat);
			TS_CAT_print(&cat, "\t\t");
		} else if (TS_check_is_PMT(&pat, packet.pid)) {
			//PMT
			pointer_field = packet.data[poffset];
			poffset += 1;
			bzero(&pmt, sizeof(pmt));
			poffset += TS_PMT_parse(&packet, poffset, &pmt);
			TS_PMT_print(&pmt, "\t\t");
		} else if (packet.pid != -1 && packet.pid == TS_PAT_get_network_PID(&pat)) {
			//Network PID
		} else if (TS_check_is_PMT_stream(&pmt, packet.pid)){
			//PES header
			TS_PES_t pes = {0x00};
			result = TS_PES_parse(&packet, poffset, &pes);
			if (result == -1) { 
				fprintf(stderr, "\t\tThis packet isn't PES.\n");
				continue;
			}
			TS_PES_print(&pes, "\t\t");
		}
	} else {
		//Payload data
		if (packet.pid == TS_PID_PAT) {
		} else if (packet.pid == TS_PID_CAT) {
		}
		if (TS_check_is_PMT_stream(&pmt, packet.pid)) {
			//PES data
		}
	}

}

#endif