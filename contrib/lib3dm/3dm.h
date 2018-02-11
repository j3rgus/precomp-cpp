#ifndef TDM_PARSER_LIB
#define TDM_PARSER_LIB

#include <stdint.h>

#define TDM_FILE_MAGIC				"3D Geometry File Format "
#define TDM_FILE_MAGIC_SIZE			24
#define TDM_FILE_HEADER_SIZE		TDM_FILE_MAGIC_SIZE + TDM_FILE_VERSION_SIZE
#define TDM_FILE_VERSION_SIZE_OLD	4
#define TDM_FILE_VERSION_SIZE		8
#define TDM_FILE_READ				0
#define TDM_FILE_PEEK				1

#define TDM_TCODE_SIZE				4
#define TDM_TCODE_CRC_SIZE_OLD		2
#define TDM_TCODE_CRC_SIZE			4

#define TDM_SUCCESS					0
#define TDM_ERROR_FUNC_INPUT		1
#define TDM_ERROR_INVALID_FILE		2
#define TDM_ERROR_INVALID_TCODE		3
#define TDM_ERROR_OUT_OF_MEMORY		4
#define TDM_ERROR_INVALID_CRC		5
#define TDM_INVALID_FILE_VER		6

#define TDM_FUNC_SUCCESS			0
#define TDM_FUNC_ERROR				1

#define ERR_STRING_SIZE				128


/* Structures for TYPECODE and chunks. */
////////////////////////////////////////////
typedef struct version {
	int v_maj;
	int v_min;
} version_t;

/*
 *  TYPECODE bit structure (32 bit long)
 */
typedef struct tcode {
	unsigned int format : 1;
	unsigned int category : 15;
	unsigned int crc_ctrl : 1;
	unsigned int spec_code: 12;
	unsigned int stuff    : 1;
	unsigned int reserved : 2;
	uint32_t value;
} tcode_t;

/*
 *  Short chunk structure 
*/
typedef struct chunk_short {
    tcode_t typecode;
	version_t ver;
	char data[TDM_FILE_VERSION_SIZE];
} chunk_short_t;

/*
 *  Big chunk structure
 */
typedef struct chunk_big {
	tcode_t typecode;
	version_t ver;
	uint64_t size;
	char *data;
	uint32_t crc;
} chunk_big_t;

/*
 *  Structure for bitmap header info
*/
typedef struct bitmap_header {
	uint32_t biSize;
	int32_t biWidth;
	int32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t bitCompression;
	uint32_t biSizeImage;
	int32_t biXPelsPerMeter;
	int32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} bitmap_header_t;
////////////////////////////////////////////

/* Function prototypes */
////////////////////////////////////////////
/* Functions for error handling */
void print_tdm_error(char *str);
void clear_tdm_error(void);

/* Functions for typecode and chunk processing */
int read_tcode_fd(FILE *fd_tdm, tcode_t *tcode, int peek);
int read_schunk_fd(FILE *fd_tdm, version_t *ver, chunk_short_t *s_chunk, int peek);
int read_bchunk_fd(FILE *fd_tdm, version_t *ver, chunk_big_t *b_chunk, int peek);
void free_bchunk(chunk_big_t *b_chunk);

/* Functions for checking data. */
int check_chunk_crc(chunk_big_t *b_chunk);

/* Main parsing functions */
int parse_tdm_file(const char *filename);
int parse_tdm_file_crc(FILE *fd_tdm);

////////////////////////////////////////////

#endif
