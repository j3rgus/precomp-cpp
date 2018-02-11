#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "opennurbs_3dm.h"
#include "opennurbs_crc.h"

#define TDM_TCODE_SIZE				4

#define TDM_FILE_MAGIC				"3D Geometry File Format "
#define TDM_FILE_MAGIC_SIZE			24
#define TDM_FILE_VERSION_SIZE		8
#define TDM_FILE_HEADER_SIZE		TDM_FILE_MAGIC_SIZE + TDM_FILE_VERSION_SIZE

#define TDM_SUCCESS					0
#define TDM_ERROR_FILE_DESC_INPUT	1
#define TDM_ERROR_INVALID_FILE		2
#define TDM_ERROR_INVALID_TCODE		3

#define ERR_STRING_SIZE				128

static char errstring[ERR_STRING_SIZE];
//static int errnum = 0;

void print_hex(char *data, size_t offset, size_t len)
{
	for (size_t i = offset; i < (offset + len); i++) {
		printf("%02x ", (unsigned char) data[i]);
	}
	printf("\n");
}

void skip_n_bytes(FILE *f, size_t n)
{
    while (n > 0) {
        (void) fgetc(f);
        n--;
    }
}

int parse_record(FILE *fr)
{
	return TDM_SUCCESS;
}

int parse_table(FILE *fr)
{
	return TDM_SUCCESS;
}

int parse_3dm(FILE *fr)
{
	char buffer[64];
	size_t readlen = 0;
	int v_major = -1;
	int v_minor = -1;
	int short_chunk = 0;
	int do_crc = 0;
	uint32_t tcode = 0;
	uint32_t size_v1 = 0;
	uint64_t size_v2 = 0;

	if (fr == NULL) {
		strncpy(errstring, "Invalid file descriptor.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_FILE_DESC_INPUT;
	}

	/*Read the MAGIC string from the file.*/
	readlen = fread(buffer, 1, TDM_FILE_HEADER_SIZE, fr);
	if (readlen < TDM_FILE_HEADER_SIZE) {
		strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	/* Check magic. */
	if (strncmp(buffer, TDM_FILE_MAGIC, TDM_FILE_MAGIC_SIZE)) {
		strncpy(errstring, "Invalid header of 3dm file.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	/* Check alignment and version. */
	for (size_t i = TDM_FILE_MAGIC_SIZE; i < (TDM_FILE_HEADER_SIZE - 2); i++) {
		if (buffer[i] != ' ') {
			strncpy(errstring, "File version is not properly aligned.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_FILE;
		}
	}

	if (isdigit(buffer[TDM_FILE_HEADER_SIZE - 1])) {
		if (isdigit(buffer[TDM_FILE_HEADER_SIZE - 2])) {
			v_major = buffer[TDM_FILE_HEADER_SIZE - 2] - '0';
			v_minor = buffer[TDM_FILE_HEADER_SIZE - 1] - '0';
		} else {
			v_major = buffer[TDM_FILE_HEADER_SIZE - 1] - '0';
		}
	}

	if ((v_major > 6) || (v_minor > 0) || (v_major < 5 && v_minor != -1)) {
		strncpy(errstring, "Invalid version of file.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	/* The file must contain TCODE_COMMENTBLOCK as the first chunk */
	/* All values are stored as little endian */
	readlen = fread(&tcode, 1, TDM_TCODE_SIZE, fr);
	if (readlen < TDM_TCODE_SIZE) {
		strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	if (tcode ^ TCODE_COMMENTBLOCK) {
		strncpy(errstring, "First chunk must be comment block.", ERR_STRING_SIZE - 1);
		return TDM_ERROR_INVALID_FILE;
	}

	if (v_major < 5 && v_major > 0) {
		readlen = fread(&size_v1, 1, 4, fr);
		if (readlen < 4) {
			strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_FILE;
		}
		skip_n_bytes(fr, size_v1);
	} else if (v_major >= 5) {
		readlen = fread(&size_v2, 1, 8, fr);
		if (readlen < 8) {
			strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_FILE;
		}
		skip_n_bytes(fr, size_v2);
	}

	/* Read out all chunks from the file */
	for (;;) {
		readlen = fread(&tcode, 1, TDM_TCODE_SIZE, fr);
		if (readlen < TDM_TCODE_SIZE) {
			strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_FILE;
		}

		printf("TCODE: %04x\n", tcode);

		if (tcode & 0x040c8000) {
			strncpy(errstring, "Invalid typecode in 3dm file.", ERR_STRING_SIZE - 1);
			return TDM_ERROR_INVALID_TCODE;
		}

		if (tcode & TCODE_SHORT) {
			short_chunk = 1;
		}

		if (tcode & TCODE_CRC) {
			do_crc = 1;
		}

		if (v_major < 5 && v_major > 0) {
			readlen = fread(&size_v1, 1, 4, fr);
			if (readlen < 4) {
				strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
				return TDM_ERROR_INVALID_FILE;
			}
			skip_n_bytes(fr, size_v1);
		} else if (v_major >= 5) {
			readlen = fread(&size_v2, 1, 8, fr);
			if (readlen < 8) {
				strncpy(errstring, "Not valid 3dm file.", ERR_STRING_SIZE - 1);
				return TDM_ERROR_INVALID_FILE;
			}
			skip_n_bytes(fr, size_v2);
		}

		if (tcode == TCODE_ENDOFFILE) {
			break;
		}
	}

	return TDM_SUCCESS;
}

int main(int argc, char **argv)
{
	FILE *fr = NULL;
	int err;

	if (argc < 2)
		return 1;

	fr = fopen(argv[1], "rb");
	if (fr == NULL) {
		perror("fopen:");
		return 1;
	}
	err = parse_3dm(fr);
	if (err > 0) {
		fprintf(stderr, "parse_3dm returned %d: %s\n", err, errstring);
		return 1;
	}

	fclose(fr);

	return 0;
}
