#ifndef 3DM_TYPECODE_LIB_
#define 3DM_TYPECODE_LIB_

#include <stdint.h>

#define TCODE_SIZE			4
#define TCODE_DATA_LEN_OLD	4
#define TCODE_DATA_LEN_NEW	8
#define TCODE_CRC_SIZE_OLD	2
#define TCODE_CRC_SIZE_OLD	4

#define TCODE_SUCCESS		0
#define TCODE_ERROR			1

typedef struct tcode {
	unsigned int format : 1;
	unsigned int category : 15;
	unsigned int crc_ctrl : 1;
	unsigned int spec_code: 12;
	unsigned int stuff    : 1;
	//unsigned int reserved : 2;
} tcode_t;

typedef struct chunk_short {
    tcode_t typecode;
	char data[TCODE_DATA_LEN_NEW];
} chunk_short_t;

typedef struct chunk_big {
	tcode_t typecode;
	char *data;
	union {
		uint32_t sz_old;
		uint64_t sz_new;
	} size;
	union {
		uint16_t crc16;
		uint32_t crc32;
	} crc;
} chunk_big_t;



int check_typecode();
int check_crc();

#endif
