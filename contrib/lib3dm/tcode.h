#ifndef 3DM_TYPECODE_LIB_
#define 3DM_TYPECODE_LIB_

#include <stdint.h>

#define TCODE_SIZE			4
#define TCODE_DATA_LEN_V1	4
#define TCODE_DATA_LEN_V2	8

typedef struct tcode {
	unsigned int format : 1;
	unsigned int category : 15;
	unsigned int crc_ctrl : 1;
	unsigned int spec_code: 12;
	unsigned int stuff    : 1;
	//unsigned int reserved : 2;
} tcode_t;

typedef struct tcode_short {
    tcode_t typecode;
	char data[TCODE_DATA_LEN_V2];
} tcode_short_t;

typedef struct tcode_big {
	tcode_t typecode;
	union {
		uint32_t sz_v1;
		uint64_t sz_v2;
	} size;
	char *data;
	union {
		uint16_t crc16;
		uint32_t crc32;
	} crc;
} tcode_big_t;

/* Read TCODE chunk from buffer into tcode_*_t */
int read_chunk_short(char *buff, tcode_short_t *tc_chunk);
int read_chunk_big(FILE *buff, tcode_big_t *tc_chunk);

/* Read TCODE chunk from FILE into tcode_*_t */
int fread_chunk_short(FILE *f, tcode_short_t *tc_chunk);
int fread_chunk_big(FILE *f, tcode_big_t *tc_chunk);

#endif
